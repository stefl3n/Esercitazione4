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

int main(int argc, char *argv[])
{
	int sd, port, fd_sorg, fd_dest, nread, i=0, j=0;
	char buff[256], req[256];
	// FILENAME_MAX: lunghezza massima nome file. Costante di sistema.
	char c;
	struct hostent *host;
	struct sockaddr_in servaddr;


	/* CONTROLLO ARGOMENTI ---------------------------------- */
	if(argc!=3){
		printf("Error:%s serverAddress serverPort\n", argv[0]);
		exit(1);
	}

	/* INIZIALIZZAZIONE INDIRIZZO SERVER -------------------------- */
	memset((char *)&servaddr, 0, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	host = gethostbyname(argv[1]);

	/*VERIFICA INTERO*/
	nread=0;
	while( argv[2][nread]!= '\0' ){
		if( (argv[2][nread] < '0') || (argv[2][nread] > '9') ){
			printf("Secondo argomento non intero\n");
			exit(2);
		}
		nread++;
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

	/* CORPO DEL CLIENT:
	ciclo di accettazione di richieste da utente ------- */
	/* ATTENZIONE!!
	* Cosa accade se la riga e' piu' lunga di FILENAME_MAX-1?
	* Stesso dicasi per le altre gets...
	* Come si potrebbe risolvere il problema?
	*/
	while(true){
		/*Chiedo il nome di cartella e lo salvo nella variabile req-------------*/
		
		printf("Scrivi nome cartella\n");
		scanf("%s",req);
		//Forse devo aggiungere qualche controllo ----------------*/
		
		/*Creo la connessione ------------------*/    
		
		sd=socket(AF_INET, SOCK_STREAM, 0);
		if(sd<0) {perror("apertura socket"); exit(1);}
		printf("Client: creata la socket sd=%d\n", sd);

		/* Operazione di BIND implicita nella connect */
		
		if(connect(sd,(struct sockaddr *) &servaddr, sizeof(struct sockaddr))<0)
		{perror("connect"); exit(1);}
		printf("Client: connect ok\n");

		/* Invio il nome di cartella al server --------------*/
		if((write(sd,req,sizeof(req)))<0)
			{perror("invio"); exit(1);}
		printf("Client: invio ok\n");
		
		/*Creo un figlio che mi stia in ascolto per evitare di pienare il buffere della connessione in ricezione
		 * e in questo modo posso continuare a fare richieste in caso di bisogno--------------------*/
		 
		 if(fork()==0){
			 while((check=read(sd,&c,sizeof(char)))>0){
				 buff[i]=c;
				 i++;
				 /*CREDO VADANO CONTROLLATI SOLO QUESTI--------------***-*-*-*-*-*-*-*-*-*/
				 if(c==' ' || c=='\0' || c=='\n'){
					 for(j=0;j<=i;i++){
						 printf(buff[j]);
					 }
					 /*Faccio in modo che ogni nome di file a video sia separato da \n-----------------*/
					 
					 if(c!='\n'){
						printf("\n");}
					
					/*"Svuoto la variabile buff"-------------------*/
					
					i=0;
				}
			}
			
			/*Controllo se è uscito per EOF o per qualche errore---------------*/
			
			if(check<0){
				perror("ClientStream: ricezione...");
				
			}
			
		}
	}
}