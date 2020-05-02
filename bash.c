
#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <readline/readline.h> 
#include <readline/history.h> 
#include <fcntl.h>
  
#define MAXCOM 1000 // numero de letras 
#define MAXLIST 100 // numero de comandos
#define TAM 60
// limpiar la consola con simbolos de escape 
#define clear() printf("\033[H\033[J")   
char ** cmdGlobal;
char * username;

void init_shell() 
{ 
    clear(); 
    username = getenv("USER"); 
    printf("\n\n\nUSER is: @%s", username); 
    printf("\n"); 
    sleep(1); 
    clear(); 
} 
void redireccionaSalida(char cad[TAM]){
    char *cadPtr;
    cadPtr=cad;
    close(1);
    open(cadPtr,O_CREAT | O_WRONLY,0777);
}
void entradaArchivo(char cad[TAM]){
    char *cadPtr;
    int f;
    cadPtr = cad;
    f=open(cadPtr,O_RDONLY);
    close(0);
    dup(f);
}

int obtenEntrada(char* str){ 
    char* buf; 
    buf = readline(">>> "); 
    if (strlen(buf) != 0) { 
        add_history(buf); 
        strcpy(str, buf); 
        return 0; 
    } else { 
        return 1; 
    } 
} 

void imprimeDir(){ 
    char cwd[1024]; 
    getcwd(cwd, sizeof(cwd)); 
    printf("\nDir: @%s %s", username, cwd); 
} 

void execArgs(char** parsed){ 
    pid_t pid = fork();  
    if (pid == -1) { 
        printf("\nFalla creando proceso hijo.."); 
        return; 
    } else if (pid == 0) { 
        if (execvp(parsed[0], parsed) < 0) { 
            printf("\nNo se pudo ejecutar el comando.."); 
        } 
        exit(0); 
    } else { 
        wait(NULL);  
        return; 
    } 
} 
void pipeline(char ***cmd){
    int descArchivo[2];
    pid_t pid;
    int descArchivoD = 0;

    while (*cmd != NULL) {
        pipe(descArchivo);
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(1);
        }
        else if (pid == 0) {
            dup2(descArchivoD, 0);
            if (*(cmd + 1) != NULL) {
                dup2(descArchivo[1], 1);
            }
            close(descArchivo[0]);
            execvp((*cmd)[0], *cmd);
            exit(1);
        }
        else {
            wait(NULL);
            close(descArchivo[1]);
            descArchivoD = descArchivo[0];
            cmd++;
        }
    }
}

void Aiuda(){ 
    puts("\nHas pedido aiuda:\n"
        "\nComandos soportados:"
        "\n>cd"
        "\n>ls"
        "\n>exit"
        "\n>todos los comandos que soporta UNIX shell"
        "\n>uso de tuberias"); 
    return; 
} 

int esComandoPropio(char** parsed){ 
    int numComandosPropios = 4, i, switchComando = 0; 
    char* listaComandosPropios[numComandosPropios]; 
    char* username; 
    listaComandosPropios[0] = "exit"; 
    listaComandosPropios[1] = "cd"; 
    listaComandosPropios[2] = "aiuda"; 
    listaComandosPropios[3] = "hola"; 
    for (i = 0; i < numComandosPropios; i++) { 
        if (strcmp(parsed[0], listaComandosPropios[i]) == 0) { 
            switchComando = i + 1; 
            break; 
        } 
    } 
    switch (switchComando) { 
    case 1: 
        printf("\nHasta luego\n"); 
        exit(0); 
    case 2: 
        chdir(parsed[1]); 
        return 1; 
    case 3: 
        Aiuda(); 
        return 1; 
    case 4: 
        username = getenv("USER"); 
        printf("\nHola! %s.\nMe presento "
            "soy el proyecto que te hará pasar la materia de sistemas operativos."
            "\nUsa aiuda para mas informacion..\n", 
            username); 
        return 1; 
    default: 
        break; 
    } 
    return 0; 
} 

int separaTuberia(char* str, char** separada){ 
    int i = 0;

    while( (separada[i] = strsep(&str,"|")) != NULL ){
        i++;
    }

    if (separada[1] == NULL) 
        return 0; 
    else { 
        return i; 
    } 
} 

void separarEspacios(char* str, char** parsed) { 
    int i;
    char *aux; 
    for (i = 0; i < MAXLIST; i++) { 
        aux = strsep(&str, " "); 
        if (aux == NULL){ 
            parsed[i] = aux;
            break;
        }
        else if (strlen(aux) == 0)
            i--; 
        else{
            parsed[i] = aux;
        }
    }
} 

char ** separarEspaciosPipes(char* str, char** parsed){ 
    char *aux;
    char **parsed2;
    parsed2 = (char**)malloc(sizeof(char*) * 100); 
    for (int i = 0; i < MAXLIST; i++) { 
        aux = strsep(&str, " "); 
        if (aux == NULL){ 
            parsed[i] = aux;
            parsed2[i] = aux;
            return parsed2; 
        }
        else if (strlen(aux) == 0)
            i--; 
        else{
            parsed[i] = aux;
            parsed2[i] = aux;
        }
    }
} 



int procesaCadena(char* str, char** parsed, char** parsedpipe){ 
    char * separada[100]; 
    int piped = 0, i = 0, y, k = 0, ejecutar = 0; 
    piped = separaTuberia(str, separada); 
    char entrada[MAXLIST], salida[TAM];
    char ** cmd[(piped+1)];

    if (piped) { 
        for(int j = 0; j < piped; j++){
         i = 0;
            while(i <= strlen(separada[j])){
                if(separada[j][i] =='<'){ 
                        separada[j][i] = ' ';
                        i++;
                        if(separada[j][i] !=' '){ 
                            ejecutar=1;
                        }else{
                            i++; 
                                for(y = 0; separada[j][i] !='\0' && separada[j][i] !=' ' && separada[j][i] !='|' && separada[j][i] !='>'; y++){
                                    entrada[y] = separada[j][i]; 
                                    separada[j][i] = ' ';
                                    i++;
                                }
                                entrada[y]='\0'; 
                                if(separada[j][i]!='\0') i++; 
                                entradaArchivo(entrada); 
                            }
                }

                if (separada[j][i] == '>') { 
                        separada[j][i] = ' ';
                        i++;
                        if (separada[j][i] != ' '){
                            ejecutar=1; 
                        }else{
                                i++; 
                                    for(y = 0; separada[j][i] != '\0';y++){
                                        salida[y] = separada[j][i]; 
                                        separada[j][i] = ' ';
                                        i++;
                                    }
                            salida[y] = '\0'; 
                            redireccionaSalida(salida); 
                        }
                }

                i++;
            }
        }    

    if(ejecutar!=0) printf("Error en la sintáxis\n");

    separarEspacios(separada[0], parsed);

    for(i = 0; i < piped; i++){
        cmd[i] = separarEspaciosPipes(separada[i], parsedpipe);   
    }
        cmd[i] = NULL;

    } else { 

        i = 0;
        while(i <= strlen(str)){
            if(str[i] =='<'){ 
                    str[i] = ' ';
                    i++;
                    if(str[i] !=' '){ 
                        ejecutar=1;
                    }else{
                        i++; 
                            for(y = 0; str[i] !='\0' && str[i] !=' ' && str[i] !='|' && str[i] !='>'; y++){
                                entrada[y] = str[i]; 
                                str[i] = ' ';
                                i++;
                            }
                            entrada[y]='\0'; 
                            if(str[i]!='\0') i++; 
                            entradaArchivo(entrada); 
                        }
            }

            if (str[i] == '>') { 
                    str[i] = ' ';
                    i++;
                    if (str[i] != ' '){
                        ejecutar=1; 
                    }else{
                            i++; 
                                for(y = 0; str[i] != '\0';y++){
                                    salida[y] = str[i];
                                    str[i] = ' ';
                                    i++;
                                }
                        salida[y] = '\0'; 
                        redireccionaSalida(salida);
                    }
            }

            i++;

        }

         if(ejecutar!=0) printf("Error en la sintáxis\n");


        separarEspacios(str, parsed);

    } 
    if (esComandoPropio(parsed)) 
        return 0;
    else{
        pipeline(cmd);
        return 1 + piped; 
    }
} 

int main(){ 
    char cadenaEntrada[MAXCOM], * argsParsed[MAXLIST]; 
    char * argsParsedPipe[MAXLIST]; 
    int bandera = 0; 
    int stdout = dup(1), stdin = dup(0);
    
    init_shell();

    while (1) { 
        close(1); // Se cierra la salida que tenga 
        dup(stdout);
        close(0); 
        dup(stdin);
        imprimeDir();
        if (obtenEntrada(cadenaEntrada)) 
            continue; 
        bandera = procesaCadena(cadenaEntrada, argsParsed, argsParsedPipe); 
        if (bandera == 1) 
            execArgs(argsParsed);
    } 
    return 0; 
}
