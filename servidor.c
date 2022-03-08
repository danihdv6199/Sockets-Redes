/*

** Fichero: servidor.c
** Autores:
** Daniel Hernandez de Vega DNI 70915236A
** Alberto Corredera Romero DNI 70911473N
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include<stdbool.h>



#define PUERTO 17278
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE 1024	/* maximum size of packets to be received */
#define BUFFERSIZE2 512
#define TAM_BUFFER 10
#define MAXHOST 128


extern int errno;

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */
 
void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in);
void errout(char *);		/* declare error out routine */

int FIN = 0;             /* Para el cierre ordenado */
char ejecutable[100];
void finalizar(){ FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
{

	int s_TCP, s_UDP;		/* connected socket descriptor */
	int ls_TCP;				/* listen socket descriptor */
	int cc;				    /* contains the number of bytes read */
     
	struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */
    
	struct sockaddr_in myaddr_in;		/* for local socket address */
	struct sockaddr_in clientaddr_in;	/* for peer socket address */
	int addrlen;
	
	fd_set readmask;
	int numfds,s_mayor;
    
	char buffer[BUFFERSIZE];	/* buffer for packets to be read into */
    
	struct sigaction vec;
	strcpy(ejecutable, "");
	strcpy(ejecutable,argv[0]);
	

	/* Create the listen socket. */
	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

	addrlen = sizeof(struct sockaddr_in);

	/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
		/* The server should listen on the wildcard address,
		 * rather than its own internet address.  This is
		 * generally good practice for servers, because on
		 * systems which are connected to more than one
		 * network at once will be able to have one server
		 * listening on all networks at once.  Even when the
		 * host is connected to only one network, this is good
		 * practice, because it makes the server program more
		 * portable.
		 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}
		/* Initiate the listen on the socket so remote users
		 * can connect.  The listen backlog is set to 5, which
		 * is the largest currently supported.
		 */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}
	
	
	/* Create the socket UDP. */
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	}
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	}

		/* Now, all the initialization of the server is
		 * complete, and any user errors will have already
		 * been detected.  Now we can fork the daemon and
		 * return to the user.  We need to do a setpgrp
		 * so that the daemon will no longer be associated
		 * with the user's control terminal.  This is done
		 * before the fork, so that the child will not be
		 * a process group leader.  Otherwise, if the child
		 * were to open a terminal, it would become associated
		 * with that terminal as its control terminal.  It is
		 * always best for the parent to do the setpgrp.
		 */
		 
	//Independizamos este procesos del grupo de procesos de la consola
	
	setpgrp();
	
	//Creamos un proceso demonio
	
	switch (fork()) {
	case -1:		/* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0:	/* The child process (daemon) comes here. */

			/* Close stdin and stderr so that they will not
			 * be kept open.  Stdout is assumed to have been
			 * redirected to some logging file, or /dev/null.
			 * From now on, the daemon will not report any
			 * error messages.  This daemon will loop forever,
			 * waiting for connections and forking a child
			 * server to handle each one.
			 */
		fclose(stdin);
		fclose(stderr);

			/* Set SIGCLD to SIG_IGN, in order to prevent
			 * the accumulation of zombies as each child
			 * terminates.  This means the daemon does not
			 * have to make wait calls to clean them up.
			 */
		if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
			perror(" sigaction(SIGCHLD)");
			fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
			exit(1);
		}
            
		/* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
		vec.sa_handler = (void *) finalizar;
		vec.sa_flags = 0;
		if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
			perror(" sigaction(SIGTERM)");
			fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
			exit(1);
		}
		
		while (!FIN) {
			/* Meter en el conjunto de sockets los sockets UDP y TCP */
			FD_ZERO(&readmask);
			FD_SET(ls_TCP, &readmask);
			FD_SET(s_UDP, &readmask);
			/* 
			Seleccionar el descriptor del socket que ha cambiado. Deja una marca en 
			el conjunto de sockets (readmask)
			*/ 
			if (ls_TCP > s_UDP) s_mayor=ls_TCP;
			else s_mayor=s_UDP;

			if ( (numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
				if (errno == EINTR) {
					FIN=1;
					close (ls_TCP);
					close (s_UDP);
					perror("\nFinalizando el servidor. SeÃal recibida en elect\n "); 
				}
			}
			else { 

				/* Comprobamos si el socket seleccionado es el socket TCP */
				if (FD_ISSET(ls_TCP, &readmask)) {
				/* Note that addrlen is passed as a pointer
				 * so that the accept call can return the
				 * size of the returned address.
				 */
				/* This call will block until a new
				 * connection arrives.  Then, it will
	 			 * return the address of the connecting
				 * peer, and a new socket descriptor, s,
				 * for that connection.
				 */
				s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
				if (s_TCP == -1) exit(1);
				switch (fork()) {
					case -1:	/* Can't fork, just exit. */
						exit(1);
					case 0:		/* Child process comes here. */
						close(ls_TCP); /* Close the listen socket inherited from the daemon. */
						serverTCP(s_TCP, clientaddr_in);
						exit(0);
					default:	/* Daemon process comes here. */
							/* The daemon needs to remember
							 * to close the new accept socket
							 * after forking the child.  This
							 * prevents the daemon from running
							 * out of file descriptor space.  It
							 * also means that when the server
							 * closes the socket, that it will
							 * allow the socket to be destroyed
							 * since it will be the last close.
							 */
						close(s_TCP);
					}
				} /* De TCP*/
 /* Comprobamos si el socket seleccionado es el socket UDP */
          if (FD_ISSET(s_UDP, &readmask)) {
                /* This call will block until a new
                * request arrives.  Then, it will
                * return the address of the client,
                * and a buffer containing its request.
                * BUFFERSIZE - 1 bytes are read so that
                * room is left at the end of the buffer
                * for a null character.
                */
                
                
                //struct in_addr reqaddr;	/* for requested host's address */
		//struct hostent *hp;		/* pointer to host info for requested host */
		//int nc, errcode;
		//struct addrinfo hints, *res;

		int addrlen;
		addrlen = sizeof(struct sockaddr_in);
		
		long timevar;
		
		
		char *trozoCadenaLinea, *trozoCadenaEspacios;
	    	int q;
	    	char vecCadenaRecibidaTroceadaPorLinea[4][200]; //Vector que guarda los trocos del mensaje del cliente separados por \r\n
		char retorno[]="\r\n";
		char http[]="HTTP/1.1";
		char cabeceraCorrecta[]="200 OK";
		char cabeceraIncorrectaGet[]="501 NOT IMPLEMENTED";
		char cabeceraIncorrectaFichero[]="404 Not Found";
		char server[]="Server: ";
		char connection[]="Connection: keep-alive";
		char content[]="content-Length: 54";
		char linkHtml[]="<html><body><h1>Universad de salamanca</html></body></h1>";
		char linkHtmlFicheroIncorrecto[]="<html><body><h1>404 Not Found</h1></body></html>";
		char linkHtmlGetIncorrecto[]="<html><body><h1>501 Not Implemented</h1></body></html>";
		char vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[3][100]; //Vector que guarda la cabecera separada
		char rutaDeTrabajo[BUFFERSIZE];
		char rutaFichero[BUFFERSIZE];
		char cadenaRecibida[BUFFERSIZE];
		char cadenaEnvio[BUFFERSIZE2];

		FILE *ficheroHtml;
		FILE *log;
		char cadenaFicheroHtml[BUFFERSIZE2]; //leer fichero de 512 en 512 
		char cadenaLog[512];
		char hostname[MAXHOST];
		char puerto[100];
		
		//directorio de trabajo
		if (getcwd(rutaDeTrabajo, sizeof(rutaDeTrabajo)) == NULL){
		        perror("Error directorio");
		}
		strcat(rutaDeTrabajo, "/www");   
		
		
		time (&timevar);	
		 strcpy(cadenaLog,"");
		
		//Faltaria para el fichero .log
		/*En caso que la cabecera del fichero log se tenga que poner solo una vez:*/
		/*Limpiamos la cadena*/
		if(getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in), hostname,MAXHOST,NULL,0,0) != 0){
			//no hemos encontrado nombreHost, ponemos IP
			strcat(cadenaLog, inet_ntoa(clientaddr_in.sin_addr)); //Metemos la IP
		}
		else{
			strcat(cadenaLog, hostname);
		}
		
		strcat(cadenaLog, " ");
		strcat(cadenaLog, inet_ntoa(clientaddr_in.sin_addr));
		strcat(cadenaLog, " ");
		strcat(cadenaLog, "UDP"); //Metemos protocolo de transporte
		strcat(cadenaLog, " ");
		snprintf(puerto, sizeof(puerto), "%d", clientaddr_in.sin_port);
		strcat(cadenaLog, puerto); //Metemos puerto efimero
		strcat(cadenaLog, " ");
		strcat(cadenaLog, ejecutable);
		strcat(cadenaLog, " ");
		strcat(cadenaLog, ctime(&timevar)); //Metemos la fecha y hora
		
		
		//crear o añadir a ficheroLog
		if(NULL == (log = (fopen("ficheroLog.log", "a")))){

		        fprintf(stderr, "No se ha podido abrir el fichero");

		} 
		
		fputs(cadenaLog, log);
		fclose(log); 
		
		strcpy(cadenaLog,"");
		
		//do{
		
		//Copiamos la cadena recibida del cliente
		
		 cc = recvfrom(s_UDP, buffer, BUFFERSIZE, 0,
                   (struct sockaddr *)&clientaddr_in, &addrlen);
                if ( cc == -1) {
                    perror(argv[0]);
                    printf("%s: recvfrom error\n", argv[0]);
                    exit (1);
                    }
                /* Make sure the message received is
                * null terminated.
                */
                buffer[cc]='\0';
		
		strcpy(cadenaRecibida,buffer);
		//printf("\nLa cadena recibida del cliente udp es:\n%s\n",cadenaRecibida);
			
		/*Troceamos el mensaje por \r\n*/
	 	q=0;
		trozoCadenaLinea = strtok(cadenaRecibida, retorno);
	  	while(trozoCadenaLinea != NULL){

	 		strcpy(vecCadenaRecibidaTroceadaPorLinea[q], trozoCadenaLinea);
			trozoCadenaLinea = strtok(NULL, retorno);
			q++;

		}


		/*Separamos el envio de la cabecera por espacios*/
		q=0;
		trozoCadenaEspacios = strtok(vecCadenaRecibidaTroceadaPorLinea[0], " ");
		while(trozoCadenaEspacios != NULL){
		        
			strcpy(vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[q], trozoCadenaEspacios);
			/*Seguimos con el siguiente token*/
	 		trozoCadenaEspacios = strtok(NULL, " ");
			q++;

		}
		
		//log
		strcat(cadenaLog,http);
		strcat(cadenaLog," ");
		
		
		//cadena envio vacia
		strcpy(cadenaEnvio,"");        
		strcat(cadenaEnvio,http);
		strcat(cadenaEnvio," ");  
		        
		if(strcmp(vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[0], "GET")){
		//501 Not Implemented
			//printf("no es GET\n");
			
			strcat(cadenaLog,cabeceraIncorrectaGet);
			strcat(cadenaLog,retorno);
			
			strcat(cadenaEnvio,cabeceraIncorrectaGet);
			strcat(cadenaEnvio,retorno);
			
			strcat(cadenaEnvio,server);
			strcat(cadenaEnvio,hostname);
			strcat(cadenaEnvio,retorno);
				
			if(strcmp(vecCadenaRecibidaTroceadaPorLinea[2],connection)){
				//printf("No lleva el keep alive \n");	
			}
			else{
				//printf("Si lleva keepalive\n");
				strcat(cadenaEnvio,connection);
				strcat(cadenaEnvio,retorno);
			}
				
			strcat(cadenaEnvio,content);
			strcat(cadenaEnvio,retorno);
			strcat(cadenaEnvio,linkHtmlGetIncorrecto);
				
			//printf("\nLa cadena que enviamos al cliente es:\n%s\n",cadenaEnvio);
			 if (sendto(s_UDP, cadenaEnvio, BUFFERSIZE2, 0, (struct sockaddr*) &clientaddr_in, addrlen) != BUFFERSIZE2) printf("ERROR envio cadena\n");
			 
			  //crear o añadir a ficheroLog
			if(NULL == (log = (fopen("ficheroLog.log", "a")))){

				fprintf(stderr, "No se ha podido abrir el fichero");

			} 
		
			fputs(cadenaLog, log); //Ponemos en el fichero la cabecera
			fclose(log); //Cerramos el fichero
					
		}
		else{
			//printf("\nSi es GET\n");
			//strcpy(nombreFichero, vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[1] + 1);
			strcpy(rutaFichero, rutaDeTrabajo);
			strcat(rutaFichero, vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[1]);
			//printf("Abro; %s\n",rutaFichero);
			if((ficheroHtml = (fopen(rutaFichero, "r")))== NULL){
			//404 Not Found
				//printf( "\nError al abrir el fichero %s\n",vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[1]);				
				
				//log
			       strcat(cadenaLog,cabeceraIncorrectaFichero);
				strcat(cadenaLog,retorno);
				
				strcat(cadenaEnvio,cabeceraIncorrectaFichero);
				strcat(cadenaEnvio,retorno);
				
				strcat(cadenaEnvio,server);
				strcat(cadenaEnvio,hostname);//hostname
				strcat(cadenaEnvio,retorno);
				
				if(strcmp(vecCadenaRecibidaTroceadaPorLinea[2],connection)){
					//printf("No lleva el keep alive \n");
				
				
				}
				else{
					//printf("Si lleva keepalive\n");
					strcat(cadenaEnvio,connection);
					strcat(cadenaEnvio,retorno);
				}
				
				strcat(cadenaEnvio,content);
				strcat(cadenaEnvio,retorno);
				strcat(cadenaEnvio,linkHtmlFicheroIncorrecto);
				
				//printf("\nLa cadena que enviamos al cliente es:\n%s\n",cadenaEnvio);
				 if (sendto(s_UDP, cadenaEnvio, BUFFERSIZE2, 0, (struct sockaddr*) &clientaddr_in, addrlen) != BUFFERSIZE2) printf("ERROR envio cadena\n");
				 
					 //crear o añadir a ficheroLog
				if(NULL == (log = (fopen("ficheroLog.log", "a")))){

					fprintf(stderr, "No se ha podido abrir el fichero");

				} 
			
				fputs(cadenaLog, log); //Ponemos en el fichero la cabecera
				fclose(log); //Cerramos el fichero
				 
			}
			else{
			//Todo correcto
				//printf("\nTodo correcto\n");
				
				//log
			       strcat(cadenaLog,cabeceraCorrecta);
				strcat(cadenaLog,retorno);
				
				strcat(cadenaEnvio,cabeceraCorrecta);
				strcat(cadenaEnvio,retorno);
				
				strcat(cadenaEnvio,server);
				strcat(cadenaEnvio,hostname);//hostname
				strcat(cadenaEnvio,retorno);
				
				if(strcmp(vecCadenaRecibidaTroceadaPorLinea[2],connection)){
				//printf("No lleva el keep alive \n");
				
				
				}
				else{
					//printf("Si lleva keepalive\n");
					strcat(cadenaEnvio,connection);
					strcat(cadenaEnvio,retorno);
				}
				
				strcat(cadenaEnvio,content);
				strcat(cadenaEnvio,retorno);
				
				//Enviamos hasta aqui
				//printf("\nLa cadena que enviamos al cliente es:\n%s\n",cadenaEnvio);
				 if (sendto(s_UDP, cadenaEnvio, BUFFERSIZE2, 0, (struct sockaddr*) &clientaddr_in, addrlen) != BUFFERSIZE2) printf("ERROR envio cadena\n");  
				
				 	 //crear o añadir a ficheroLog
				if(NULL == (log = (fopen("ficheroLog.log", "a")))){

					fprintf(stderr, "No se ha podido abrir el fichero");

				} 
			
				fputs(cadenaLog, log); //Ponemos en el fichero la cabecera
				fclose(log); //Cerramos el fichero
				
				//y ahora vamos enviando lo del fichero de 512 en 512
				while(!feof(ficheroHtml)){
					//Vaciamos la cadena
					strcpy(cadenaFicheroHtml,"");			
					fgets(cadenaFicheroHtml, 512, ficheroHtml);
					//printf("\nY vamos enviando el fichero: %s", cadenaFicheroHtml);
					if (sendto(s_UDP, cadenaFicheroHtml, BUFFERSIZE2, 0, (struct sockaddr*) &clientaddr_in, addrlen) != BUFFERSIZE2) printf("ERROR envio cadenaFicheroHtml\n");   
					//strcat(cadenaEnvio, cadenaFicheroHtml);
				
					}	
				} 	
			}
		
		}//UDP
	}
}


 	close(ls_TCP);
        close(s_UDP);
    
        //printf("\nFin de programa servidor!\n");
        
	default:		/* Parent process comes here. */
		exit(0);
}        
}


/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;		/* keeps count of number of requests */
	char buf[BUFFERSIZE];		/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST];		/* remote host's name string */

	int len;
	struct hostent *hp;		/* pointer to host info for remote host */
	long timevar;			/* contains time returned by time() */
    
	struct linger linger;		/* allow a lingering, graceful close; */
    				            /* used when setting SO_LINGER */
	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	 
	 
	int addrlen;
	addrlen = sizeof(struct sockaddr_in);
	
	
	char *trozoCadenaLinea, *trozoCadenaEspacios;
    	int q;
    	char vecCadenaRecibidaTroceadaPorLinea[4][200]; //Vector que guarda los trocos del mensaje del cliente separados por \r\n
	char retorno[]="\r\n";
	char http[]="HTTP/1.1";
	char cabeceraCorrecta[]="200 OK";
	char cabeceraIncorrectaGet[]="501 NOT IMPLEMENTED";
	char cabeceraIncorrectaFichero[]="404 Not Found";
	char server[]="Server: ";
	char connection[]="Connection: keep-alive";
	char content[]="content-Length: 54";
	char linkHtml[]="<html><body><h1>Universad de salamanca</html></body></h1>";
	char linkHtmlFicheroIncorrecto[]="<html><body><h1>404 Not Found</h1></body></html>";
	char linkHtmlGetIncorrecto[]="<html><body><h1>501 Not Implemented</h1></body></html>";
	char vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[3][100]; //Vector que guarda la cabecera separada
	char rutaDeTrabajo[BUFFERSIZE];
	char rutaFichero[BUFFERSIZE];
	char cadenaRecibida[BUFFERSIZE];
	char cadenaEnvio[BUFFERSIZE2];

	FILE *ficheroHtml;
	FILE *log;
	char cadenaFicheroHtml[BUFFERSIZE2]; //leer fichero de 512 en 512 
	char cadenaLog[512];
	char puerto[100];
	
	//directorio de trabajo
	if (getcwd(rutaDeTrabajo, sizeof(rutaDeTrabajo)) == NULL){
                perror("Error directorio");
        }
        strcat(rutaDeTrabajo, "/www");   
	
	
	time (&timevar);	
	 strcpy(cadenaLog,"");
        
        //Faltaria para el fichero .log
        
        if(getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in), hostname,MAXHOST,NULL,0,0) != 0){
        	//no hemos encontrado nombreHost, ponemos IP
        	strcat(cadenaLog, inet_ntoa(clientaddr_in.sin_addr)); //Metemos la IP
        }
        else{
        	strcat(cadenaLog, hostname);
        }
        
        strcat(cadenaLog, " ");
        strcat(cadenaLog, inet_ntoa(clientaddr_in.sin_addr));
        strcat(cadenaLog, " ");
        strcat(cadenaLog, "TCP"); //Metemos protocolo de transporte
        strcat(cadenaLog, " ");
        snprintf(puerto, sizeof(puerto), "%d", clientaddr_in.sin_port);
        strcat(cadenaLog, puerto); //Metemos puerto efimero
        strcat(cadenaLog, " ");
        strcat(cadenaLog, ejecutable);
        strcat(cadenaLog, " ");
        strcat(cadenaLog, ctime(&timevar)); //Metemos la fecha y hora
        
        
        //crear o añadir a ficheroLog
        if(NULL == (log = (fopen("ficheroLog.log", "a")))){

                fprintf(stderr, "No se ha podido abrir el fichero");

        } 
        
        fputs(cadenaLog, log);
        fclose(log); 
        
        strcpy(cadenaLog,"");

	while (len = recv(s, buf, BUFFERSIZE, 0)) {
		if (len == -1) errout(hostname); /* error from recv */
	
		strcpy(cadenaRecibida,buf);
	
		//printf("\nLa cadena recibida del cliente TCP es:\n%s\n",cadenaRecibida);
		
		/*Troceamos el mensaje por \r\n*/
	 	q=0;
		trozoCadenaLinea = strtok(cadenaRecibida, retorno);
	  	while(trozoCadenaLinea != NULL){

	 		strcpy(vecCadenaRecibidaTroceadaPorLinea[q], trozoCadenaLinea);
			trozoCadenaLinea = strtok(NULL, retorno);
			q++;

		}


		/*Separamos el envio de la cabecera por espacios*/
		q=0;
		trozoCadenaEspacios = strtok(vecCadenaRecibidaTroceadaPorLinea[0], " ");
		while(trozoCadenaEspacios != NULL){
		        
			strcpy(vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[q], trozoCadenaEspacios);
			/*Seguimos con el siguiente token*/
	 		trozoCadenaEspacios = strtok(NULL, " ");
			q++;

		}
		
		//log
		strcat(cadenaLog,http);
		strcat(cadenaLog," ");
		
		
		//cadena envio vacia
		strcpy(cadenaEnvio,"");        
		strcat(cadenaEnvio,http);
		strcat(cadenaEnvio," ");  
		        
		if(strcmp(vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[0], "GET")){
		//501 Not Implemented
			//printf("no es GET\n");
			
			strcat(cadenaLog,cabeceraIncorrectaGet);
			strcat(cadenaLog,retorno);
			
			strcat(cadenaEnvio,cabeceraIncorrectaGet);
			strcat(cadenaEnvio,retorno);
			
			strcat(cadenaEnvio,server);
			strcat(cadenaEnvio,hostname);
			strcat(cadenaEnvio,retorno);
				
			if(strcmp(vecCadenaRecibidaTroceadaPorLinea[2],connection)){
				//printf("No lleva el keep alive \n");	
			}
			else{
				//printf("Si lleva keepalive\n");
				strcat(cadenaEnvio,connection);
				strcat(cadenaEnvio,retorno);
			}
				
			strcat(cadenaEnvio,content);
			strcat(cadenaEnvio,retorno);
			strcat(cadenaEnvio,linkHtmlGetIncorrecto);
				
			//printf("\nLa cadena que enviamos al cliente es:\n%s\n",cadenaEnvio);
			 if (send(s, cadenaEnvio, BUFFERSIZE2, 0) != BUFFERSIZE2) errout(hostname);  
			 
			  //crear o añadir a ficheroLog
			if(NULL == (log = (fopen("ficheroLog.log", "a")))){

				fprintf(stderr, "No se ha podido abrir el fichero");

			} 
		
			fputs(cadenaLog, log); //Ponemos en el fichero la cabecera
			fclose(log); //Cerramos el fichero
					
		}
		else{
			//printf("\nSi es GET\n");
			//strcpy(nombreFichero, vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[1] + 1);
			strcpy(rutaFichero, rutaDeTrabajo);
			strcat(rutaFichero, vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[1]);
			//printf("Abro; %s\n",rutaFichero);
			if((ficheroHtml = (fopen(rutaFichero, "r")))== NULL){
			//404 Not Found
				printf( "\nError al abrir el fichero %s\n",vecCadenaRecibidaPrimeraLineaTroceadaPorEspacios[1]);
				//exit(-1);
				
				//log
			       strcat(cadenaLog,cabeceraIncorrectaFichero);
				strcat(cadenaLog,retorno);
				
				strcat(cadenaEnvio,cabeceraIncorrectaFichero);
				strcat(cadenaEnvio,retorno);
				
				strcat(cadenaEnvio,server);
				strcat(cadenaEnvio,hostname);//hostname
				strcat(cadenaEnvio,retorno);
				
				if(strcmp(vecCadenaRecibidaTroceadaPorLinea[2],connection)){
					//printf("No lleva el keep alive \n");
				
				
				}
				else{
					//printf("Si lleva keepalive\n");
					strcat(cadenaEnvio,connection);
					strcat(cadenaEnvio,retorno);
				}
				
				strcat(cadenaEnvio,content);
				strcat(cadenaEnvio,retorno);
				strcat(cadenaEnvio,linkHtmlFicheroIncorrecto);
				
				//printf("\nLa cadena que enviamos al cliente es:\n%s\n",cadenaEnvio);
				 if (send(s, cadenaEnvio, BUFFERSIZE2, 0) != BUFFERSIZE2) errout(hostname);  
				 
					 //crear o añadir a ficheroLog
				if(NULL == (log = (fopen("ficheroLog.log", "a")))){

					fprintf(stderr, "No se ha podido abrir el fichero");

				} 
			
				fputs(cadenaLog, log); //Ponemos en el fichero la cabecera
				fclose(log); //Cerramos el fichero
				 
			}
			else{
			//Todo correcto
				//printf("\nTodo correcto\n");
				
				//log
			       strcat(cadenaLog,cabeceraCorrecta);
				strcat(cadenaLog,retorno);
				
				strcat(cadenaEnvio,cabeceraCorrecta);
				strcat(cadenaEnvio,retorno);
				
				strcat(cadenaEnvio,server);
				strcat(cadenaEnvio,hostname);//hostname
				strcat(cadenaEnvio,retorno);
				
				if(strcmp(vecCadenaRecibidaTroceadaPorLinea[2],connection)){
				//printf("No lleva el keep alive \n");
				
				
				}
				else{
					//printf("Si lleva keepalive\n");
					strcat(cadenaEnvio,connection);
					strcat(cadenaEnvio,retorno);
				}
				
				strcat(cadenaEnvio,content);
				strcat(cadenaEnvio,retorno);
				
				//Enviamos hasta aqui
				//printf("\nLa cadena que enviamos al cliente es:\n%s\n",cadenaEnvio);
				 if (send(s, cadenaEnvio, BUFFERSIZE2, 0) != BUFFERSIZE2) errout(hostname);  
				
				 	 //crear o añadir a ficheroLog
				if(NULL == (log = (fopen("ficheroLog.log", "a")))){

					fprintf(stderr, "No se ha podido abrir el fichero");

				} 
			
				fputs(cadenaLog, log); //Ponemos en el fichero la cabecera
				fclose(log); //Cerramos el fichero
				
				//y ahora vamos enviando lo del fichero de 512 en 512
				while(!feof(ficheroHtml)){
					//Vaciamos la cadena
					strcpy(cadenaFicheroHtml,"");			
					fgets(cadenaFicheroHtml, 512, ficheroHtml);
					//printf("\nY vamos enviando el fichero: %s", cadenaFicheroHtml);
					if (send(s, cadenaFicheroHtml,  BUFFERSIZE2, 0) !=  BUFFERSIZE2) errout(hostname);   
					//strcat(cadenaEnvio, cadenaFicheroHtml);
				
					}	
				} 	
			}
		}
	
	//close(s);
	 

}	




/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);     
}

	         




