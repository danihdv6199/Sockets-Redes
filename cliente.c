/*

** Fichero: cliente.c
** Autores:
** Daniel Hernandez de Vega DNI 70915236A
** Alberto Corredera Romero DNI 70911473N
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern int errno;
	int alarma=0;

#define ADDRNOTFOUND	0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */
#define BUFFERSIZE 1024	/* maximum size of packets to be received */
#define BUFFERSIZE2 512
#define PUERTO 17278
#define TIMEOUT 6
#define MAXHOST 512

void handler()
{
	//printf("Alarma recibida, pongo la alarma a 1\n");
	alarma=1;
 
}

void UDP(int, char **,FILE *);
void TCP(int, char **,FILE *);

int main (int argc, char *argv[]){

FILE *f;

	if(argc != 4 || (strcmp(argv[2],"UDP") && strcmp(argv[2],"TCP" ))){
		fprintf(stderr, "\nArgumentos invalidos\n");
		exit(0);
	}
	
	//Comprobación fichero
	if((f = (fopen(argv[3], "r")))== NULL){
                fprintf(stderr, "\nError al abrir el fichero %s\n", argv[3]);
                exit(-1);
        }
	
	if(strcmp(argv[2],"UDP")==0){
		//printf("\nUDP\n");
		UDP(argc, argv,f);
	}
	else{
		//printf("\nTCP\n");
		TCP(argc, argv,f);
		
	}
	fclose(f);
	return 0;

}

void UDP(int argc, char *argv[], FILE *f){

	//int alarma=0;

	int i, errcode;	
	int s;				/* socket descriptor */
	long timevar;                       /* contains time returned by time() */
	struct sockaddr_in myaddr_in;	/* for local socket address */
	struct sockaddr_in servaddr_in;	/* for server socket address */
	struct in_addr reqaddr;		/* for returned internet address */
	int	addrlen, n_retry;
	struct sigaction vec;
   	char hostname[MAXHOST];
   	struct addrinfo hints, *res;

	char cadenaLeidaOrdenes[BUFFERSIZE2];
	char vectCadenaLeidaTroceada[3][50];
	int y=0;
	char *trozoCadena;
	char pagina[BUFFERSIZE];
	
	char cadenaEnvio[BUFFERSIZE];
	char http[]="HTTP/1.1";
	char host[]="Host: ";
	char retorno[]="\r\n";
	char respuesta[BUFFERSIZE2];
	int salir = 0;
	char port[10];
	FILE *fp;
	
	
			/* Create the socket. */
	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}
	
    /* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
	
			/* Bind socket to some local address so that the
		 * server can send the reply back.  A port number
		 * of zero will be used so that the system will
		 * assign any available port number.  An address
		 * of INADDR_ANY will be used so we do not have to
		 * look up the internet address of the local host.
		 */
	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_port = 0;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	
	
	
	if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
		exit(1);
	   }
        
	addrlen = sizeof(struct sockaddr_in);
	
	//guarda la direccion asociada al socket
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	}

	
	/* Set up the server address. */
	servaddr_in.sin_family = AF_INET;
		/* Get the host information for the server's hostname that the
		 * user passed in.
		 */
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
 	/* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
	errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
	if (errcode != 0){
			/* Name was not found.  Return a
			 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				argv[0], argv[1]);
		exit(1);
	}
	else {
			/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}
	freeaddrinfo(res);
	/* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PUERTO);
	
	 /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
	vec.sa_handler = (void *) handler;
	vec.sa_flags = 0;
	if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
		perror(" sigaction(SIGALRM)");
		fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
		exit(1);
	}

	
	//leemos la primera linea de ordenes
	fgets(cadenaLeidaOrdenes, BUFFERSIZE2, f);

	
	while(!feof(f)){
		
		int p;
		/*Vaciamos el vector que contendra cada linea de ordenes*/
                for(p=0; p<3; p++){
                        strcpy(vectCadenaLeidaTroceada[p], "");
                }
		
		//troceamos la cadena leida, para ver si ha introducido la "k"
		
		y = 0;
		trozoCadena = strtok (cadenaLeidaOrdenes, " ");
		while(trozoCadena != NULL){
			strcpy(vectCadenaLeidaTroceada[y], trozoCadena);
			trozoCadena = strtok (NULL, " ");
			y++; 
		}	
		
			
		strcpy(cadenaEnvio,vectCadenaLeidaTroceada[0]);
		strcat(cadenaEnvio," ");
		strcat(cadenaEnvio,vectCadenaLeidaTroceada[1]);
		strcat(cadenaEnvio," ");
		strcat(cadenaEnvio, http);
		strcat(cadenaEnvio,retorno);
		strcat(cadenaEnvio,host); 
		strcat(cadenaEnvio, argv[1]);//servidor que indique por parametro
		strcat(cadenaEnvio,retorno);
		
		//comprobamos lleva la k, si la lleva añadimos el keep-alive
		//si no, una linea en blanco		
		if(vectCadenaLeidaTroceada[2][0] == 'k'){
			strcat(cadenaEnvio, "Connection: keep-alive");
			strcat(cadenaEnvio,retorno);
		}
				
		strcat(cadenaEnvio,retorno);
								
		//printf("\nLa cadena que enviamos al servidor es:\n%s\n",cadenaEnvio);
		
		//Envio cadenaEnvio al servidor					
		if (sendto (s, cadenaEnvio, strlen(cadenaEnvio), 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) != strlen(cadenaEnvio)) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to send request\n", argv[0]);
			exit(1);
		}
		
		
	 	//Como recibimos varios mensajes seguidos del servidor cuando lee "index.html",
		// y no sabemos cuando dejamos de recibir
		//Configuramos un tiempo de espera
		alarm(TIMEOUT);
	
			while(alarma == 0){
			
			if (recvfrom (s, respuesta, BUFFERSIZE2, 0 , (struct sockaddr *)&servaddr_in, &addrlen) == -1){
				if(alarma == 0){
					perror("\nError al recibir\n");
				}	
			}
			else{
			
				if(alarma == 0){
					//printf("\nRecibido: \n%s\n",respuesta);
					
					strcat(respuesta, retorno);
					/*Convertimos a string el puerto efimero*/
					snprintf(port, sizeof(port), "%d", myaddr_in.sin_port);
					strcat(port, ".txt");
					if((fp = (fopen(port, "a")))== NULL){
						fprintf(stderr, "No se ha podido abrir el fichero");
					}
					
					fputs(respuesta, fp); 
					fclose(fp); 
					
					//cancelamos la alarma
					alarm(0);
					//y ponemos una nueva
					alarm(TIMEOUT);
				}	
			
			}
	 	
	 	}
	 	alarma=0;

			
		//leemos la siguiente linea
		fgets(cadenaLeidaOrdenes, 512, f);
	}
	
	
	
	//cerrar el socket
}

void TCP(int argc, char *argv[],FILE *f){


	int s;				/* connected socket descriptor */
   	struct addrinfo hints, *res;
	long timevar;			/* contains time returned by time() */
	struct sockaddr_in myaddr_in;	/* for local socket address */
	struct sockaddr_in servaddr_in;	/* for server socket address */
	int addrlen, i, j, errcode;
	struct sigaction vec;


   	char hostname[MAXHOST];

	char cadenaLeidaOrdenes[BUFFERSIZE2];
	char vectCadenaLeidaTroceada[3][50];
	int y=0;
	char *trozoCadena;
	char pagina[BUFFERSIZE];
	
	char cadenaEnvio[BUFFERSIZE];
	char http[]="HTTP/1.1";
	char host[]="Host: ";
	char retorno[]="\r\n";
	char respuesta[BUFFERSIZE2];
	int salir = 0;
	char port[10];
	FILE *fp;


        /* Create the socket. */
        s = socket (AF_INET, SOCK_STREAM, 0);
        if (s == -1) {
                perror(argv[0]);
                fprintf(stderr, "%s: unable to create socket\n", argv[0]);
                exit(1);
        }
        
        /* clear out address structures */
        memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
        memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

        /* Set up the peer address to which we will connect. */
        servaddr_in.sin_family = AF_INET;
        
        /* Get the host information for the hostname that the
         * user passed in. */
        memset (&hints, 0, sizeof (hints));
        hints.ai_family = AF_INET;
 	/* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
	errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
	if (errcode != 0){
                /* Name was not found.  Return a
                 * special value signifying the error. */
                fprintf(stderr, "%s: No es posible resolver la IP de %s\n", argv[0], argv[1]);
                exit(1);
	}
	else {
                /* Copy address of host */
                servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}
	freeaddrinfo(res);

	/* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PUERTO);

        /* Try to connect to the remote server at the address
         * which was just built into peeraddr.
         */
	if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
 		fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
  		exit(1);
        }
        /* Since the connect call assigns a free address
         * to the local end of this connection, let's use
         * getsockname to see what it assigned.  Note that
         * addrlen needs to be passed in as a pointer,
         * because getsockname returns the actual length
         * of the address.
         */
        addrlen = sizeof(struct sockaddr_in);
        
        if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
                perror(argv[0]);
                fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
                exit(1);
         }
         
         
         	servaddr_in.sin_port = htons(PUERTO);
	
	 /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
	vec.sa_handler = (void *) handler;
	vec.sa_flags = 0;
	if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
		perror(" sigaction(SIGALRM)");
		fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
		exit(1);
	}

	//leemos la primera linea de ordenes
	fgets(cadenaLeidaOrdenes, BUFFERSIZE2, f);

	
	while(!feof(f)){
		
		int p;
		/*Vaciamos el vector que contendra cada linea de ordenes*/
                for(p=0; p<3; p++){
                        strcpy(vectCadenaLeidaTroceada[p], "");
                }
		
		//troceamos la cadena leida, para ver si ha introducido la "k"
		
		y = 0;
		trozoCadena = strtok (cadenaLeidaOrdenes, " ");
		while(trozoCadena != NULL){
			strcpy(vectCadenaLeidaTroceada[y], trozoCadena);
			trozoCadena = strtok (NULL, " ");
			y++; 
		}	
		
			
		strcpy(cadenaEnvio,vectCadenaLeidaTroceada[0]);
		strcat(cadenaEnvio," ");
		strcat(cadenaEnvio,vectCadenaLeidaTroceada[1]);
		strcat(cadenaEnvio," ");
		strcat(cadenaEnvio, http);
		strcat(cadenaEnvio,retorno);
		strcat(cadenaEnvio,host); 
		strcat(cadenaEnvio, argv[1]);//servidor que indique por parametro
		strcat(cadenaEnvio,retorno);
		
		//comprobamos lleva la k, si la lleva añadimos el keep-alive
		//si no, una linea en blanco		
		if(vectCadenaLeidaTroceada[2][0] == 'k'){
			strcat(cadenaEnvio, "Connection: keep-alive");
			strcat(cadenaEnvio,retorno);
		}
				
		strcat(cadenaEnvio,retorno);
								
		//printf("\nLa cadena que enviamos al servidor es:\n%s\n",cadenaEnvio);
		
		//Envio cadenaEnvio al servidor
		if(send(s, cadenaEnvio, strlen(cadenaEnvio), 0) != strlen(cadenaEnvio)){ 
                        fprintf(stderr, "%s: Connection aborted on error ",argv[0]);
                        exit(1);
                }



//Como recibimos varios mensajes seguidos del servidor cuando lee "index.html",
		// y no sabemos cuando dejamos de recibir
		//Configuramos un tiempo de espera
		alarm(TIMEOUT);
	
			while(alarma == 0){
			//printf("\nSigo buclen");
			
			if(-1 == (recv(s, respuesta, BUFFERSIZE2, 0))){
			perror(argv[0]);
			fprintf(stderr, "%s: error reading result\n", argv[0]);
				if(alarma == 0){
						perror("\nError al recibir\n");
					}
			//exit(1);
		        }
		        else{
			
				if(alarma == 0){
					//printf("\nRecibido: \n%s\n",respuesta);
					
					strcat(respuesta, retorno);
					/*Convertimos a string el puerto efimero*/
					snprintf(port, sizeof(port), "%d", myaddr_in.sin_port);
					strcat(port, ".txt");
					if((fp = (fopen(port, "a")))== NULL){
						fprintf(stderr, "No se ha podido abrir el fichero");
					}
					
					fputs(respuesta, fp); 
					fclose(fp); 
					
					//cancelamos la alarma
					alarm(0);
					//y ponemos una nueva
					alarm(TIMEOUT);
				}	
			
			}
	 	
	 	}
	 	alarma=0;
//leemos la siguiente linea
		fgets(cadenaLeidaOrdenes, BUFFERSIZE2, f);
	}
	
	
	
	//cerrar el socket
}


