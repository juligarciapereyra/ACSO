#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv){

	int n, c, s; //número de procesos, valor inicial, índice del proceso inicial
	int pid; 
	int buffer[1]; //almacenará el valor actual del mensaje

	if (argc != 4){ printf("Uso: anillo <n> <c> <s> \n"); exit(0);}
    
    //Parseo de argumentos
    n = atoi(argv[1]); 
    c = atoi(argv[2]); 
    s = atoi(argv[3]); 
    buffer[0] = c; // inicializamos el buffer con el valor inicial del mensaje

    printf("Se crearán %i procesos, se enviará el caracter %i desde proceso %i \n", n, buffer[0], s);

    //Crear los pipes
    int pipes[n+1][2]; 
    for(int i=0; i<=n; i++){
    	if(pipe(pipes[i]) == -1){
    		perror("Error al crear pipe");
			exit(EXIT_FAILURE);
    	}
    }
    
 	// Crear procesos hijos y formar el anillo
 	for(int i=0; i<n; i++){
 		pid = fork();

		if (pid == -1) {
			perror("Fork fallido");
			exit(EXIT_FAILURE);

		} else if (pid == 0) { // Código del hijo

			// Cerrar extremos no usados
			for (int j = 0; j <n; j++) {
				if (j != i) close(pipes[j][0]);
				if (j != (i + 1) % n) close(pipes[j][1]);
			}

			close(pipes[n][0]); // El padre leerá de este pipe

			read(pipes[i][0], &buffer, sizeof(int));
			close(pipes[i][0]);
			
            printf("soy el hijo %d, recibi un %d del ", i, buffer[0]);
             if (i == s) {
                printf("padre, ");
            } else if (i == 0) {
                printf("hijo %d, ", n - 1);
            } else {
                printf("hijo %d, ", i - 1);
            }

            buffer[0]++;
            printf("lo incremento a %d y se lo mando al ", buffer[0]);
            if (i == (s - 1 + n) % n){
                printf("padre\n");
                write(pipes[n][1], &buffer, sizeof(int)); // Mandar al padre
                close(pipes[n][1]);
             
            } else {
                printf("hijo %d\n", (i + 1) % n);
                close(pipes[n][1]); // Cerrar el pipe extra del padre si no soy el último
                write(pipes[(i + 1) % n][1], &buffer, sizeof(int));
                close(pipes[(i + 1) % n][1]);
            }

            exit(0);
 	
 		}
 
 	}

    for (int j = 0; j <= n; j++) {
        if (j != n) 
            close(pipes[j][0]); // Cierra todos los descriptores de lectura excepto el último
       
        if (j != s)
            close(pipes[j][1]); // Cierra todos los descriptores de escritura excepto el del primer hijo
    }	

 	// El padre envía el mensaje inicial al hijo 's'
    write(pipes[s][1], &buffer, sizeof(int));
    close(pipes[s][1]);

    // Esperar a que todos los hijos terminen
    for (int i = 0; i < n; i++) {
        if (wait(NULL) == -1) {
            perror("Error en el wait");
            exit(EXIT_FAILURE);
        }
    }

    // El padre recibe el mensaje final
    read(pipes[n][0], &buffer, sizeof(int));
    close(pipes[n][0]);
    printf("soy el padre, recibí un %d.\n", buffer[0]);

    return 0;
}
