#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#define true 1

/*Struttura di una richiesta*/
/********************************************************/
typedef struct{
	char nomefile[256];
	char parola[256];
}Request;
/********************************************************/


int main(int argc, char **argv){
	/*Dichiaro variabili---------------------------*/
	
	struct hostent *host;
	struct sockaddr_in clientaddr, servaddr;
	int  port, sd, num1, len, ris;
	char namef[256], occ[256];
	Request * req = (Request*) malloc (sizeof(Request));
	
	/* CONTROLLO ARGOMENTI ---------------------------------- */
	if(argc!=3){
		printf("Error:%s serverAddress serverPort\n", argv[0]);
		exit(1);
	}
	
	/* INIZIALIZZAZIONE INDIRIZZO CLIENT E SERVER --------------------- */
	
	memset((char *)&clientaddr, 0, sizeof(struct sockaddr_in));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = INADDR_ANY;

	/* Passando 0 ci leghiamo ad un qualsiasi indirizzo libero,
	* ma cio' non funziona in tutti i sistemi.
	* Se nel nostro sistema cio' non funziona come si puo' fare?
	*/
	
	clientaddr.sin_port = 0;

	memset((char *)&servaddr, 0, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	host = gethostbyname (argv[1]);
	
	/* VERIFICA INTERO */
	num1=0;
	while( argv[2][num1]!= '\0' ){
		if( (argv[2][num1] < '0') || (argv[2][num1] > '9') ){
			printf("Secondo argomento non intero\n");
			printf("Error:%s serverAddress serverPort\n", argv[0]);
			exit(2);
		}
		num1++;
	}
	port = atoi(argv[2]);

	/* VERIFICA PORT e HOST */
	if (port < 1024 || port > 65535){
		printf("%s = porta scorretta...\n", argv[2]);
		exit(2);
	}
	if (host == NULL){
		printf("%s not found in /etc/hosts\n", argv[1]);
		exit(2);
	}else{
		servaddr.sin_addr.s_addr=((struct in_addr *)(host->h_addr))->s_addr;
		servaddr.sin_port = htons(port);
	}
	/* CREAZIONE SOCKET ---------------------------------- */
	
	sd=socket(AF_INET, SOCK_DGRAM, 0);
	if(sd<0) {perror("apertura socket"); exit(1);}
	printf("Client: creata la socket sd=%d\n", sd);

	/* BIND SOCKET, a una porta scelta dal sistema --------------- */
	
	if(bind(sd,(struct sockaddr *) &clientaddr, sizeof(clientaddr))<0)
	{perror("bind socket "); exit(1);}
	printf("Client: bind socket ok, alla porta %i\n", clientaddr.sin_port);

	/* CORPO DEL CLIENT: ciclo di accettazione di richieste da utente */
	
	printf("Scrivi il nome del file e premi invio\n");
	
	while((scanf("%s", req->nomefile))!=EOF){
		
		printf("Scrivi la parola da elminare e premi invio\n");
		scanf("%s", req->parola);
		
		/*Invio la mia Request al server-------------*/
		
		len=sizeof(servaddr);
		if(sendto(sd, req, sizeof(Request), 0, (struct sockaddr *)&servaddr, len)<0){
			perror("sendto");continue;}
		
		/* Attendo il risultato del server--------------------*/
		
		printf("Attesa del risultato...\n");
		if (recvfrom(sd, &ris, sizeof(ris), 0, (struct sockaddr *)&servaddr, &len)<0){
			perror("recvfrom"); continue;}
		
		/*Controllo il risultato dato che se è 
		* ==-1 vuoldire che il file non esiste
		* ==-2 vuoldire che c'è stato un errore in lettura del file---------*/
		 
		if(ris>=0){ printf("Il server ha risposto: numero di occorenze eliminate dal file sono %d\n",ris);}
		else if(ris==-1){printf("Il server ha risposto: ERRORE file non esistente\n");}
		else if (ris==-2){printf("Il server ha risposto: ERRORE durante l'operazione\n");}
		
		printf("Scrivi il nome del file e premi invio\n");

	}
	
	close(sd);
	printf("\nDatagramClient: termino...\n");  
	exit(0);
}