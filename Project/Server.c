#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#include <dirent.h>
#define true 1

/*Struttura di una richiesta*/
/********************************************************/
typedef struct{
  char nomefile[256];
  char parola[256];
}Request; 
/*******************************************************/

/*Faccio le funzioni ausiliarie ************************/

/*Gestore di SIGCHLD ****************************/
void gestore(int signo){
  int stato;
  printf("esecuzione gestore di SIGCHLD\n");
  wait(&stato);
}

/*Funzione eliminaOcc che mi restituisce il num di Occ trovate ed elimina le occorenze */

int eliminaOcc(int fd, char *filename, char* parola){
	int len, i, result, nread, fdTemp;
	char c, buf[256], filenameTemp[256];
	
	strcpy(filenameTemp, filename);
	strcat(filenameTemp, "temp");
	
	if((fdTemp = creat(filenameTemp, 777))<0){
		perror("Errore apertura file temporaneo");
		return -2;
	}
	
	len = strlen(parola);
	result=0;
	i=0;
	while((nread=read(fd,&c,sizeof(char)))>0){
		if(c==parola[i]){
			buf[i++]=c;
			
			if(len==i){
				result++;
				i=0;
			}
		}else{
			if(i>0)
				write(fdTemp,buf,sizeof(char)*i);
			
			write(fdTemp,&c,sizeof(char));
			i=0;
		}
	}
	close(fd);
	close(fdTemp);
	
	/* Ho fatto in modo che se c'è stato un problema annullo tutto infatti ritorno -2
	 * in questo modo potrò segnalare al client che qualcosa è andato storto******/
	if(nread<0){
		printf("Sono %d qualcosa è andato storto durante la lettura del file\n",getpid());
		result=-2;
	}
	rename(filenameTemp, filename);
	
	return result;
}

/* Funzione che mi invia tutti i nomi dei file delle sotto directory al cliente in caso di errore mi ritorna -2 --------*/
int mandaNomiFile(int connsd, DIR* dir){
	struct dirent * dire;
    DIR * indir;
    errno=0;//readdir restituisce null anche in caso di errore, unico modo per verificare
    //successo e' settare errno e vedere se rimane a 0 (da manuale). Oppure si fa finta che vada tutto bene sempre
    
    while((dire=readdir(dir)) != NULL){//lettura dir 
        
        //d_type della struct dirent non e' standardizzato. Io lo uso comunque :) 
        //DT_DIR      This is a directory.
        if(dire->d_type==DT_DIR){//se sotto-directory la ciclo
            indir=opendir(dire->d_name);
            if(indir == NULL) return (-2);
            
            while((dire=readdir(indir)) != NULL){
                if(dire->d_type == DT_REG) write(connsd,dire->d_name,strlen(dire->d_name)+1);
            }
            
            if(errno != 0) return (-2);
        }
    }
    
    if(errno!=0) return (-2);
	
	return 0;
}

int main(int argc, char **argv){
	int udpsd, listen_sd, connsd, port, len, nready, check, check1, ris1, maxfd, fd, num1;
	fd_set fdset;
	DIR* dir;
	const int on = 1;
	char req2[256];
	struct sockaddr_in cliaddr, servaddr;
	struct hostent *clienthost;
	Request* req1 =(Request *) malloc (sizeof(Request));
	
	
	/* CONTROLLO ARGOMENTI ---------------------------------- */
	
	if(argc!=2){
		printf("Error: %s port\n", argv[0]);
		exit(1);
	}
	else{
		num1=0;
		while( argv[1][num1]!= '\0' ){
			if((argv[1][num1] < '0') || (argv[1][num1] > '9')){
				printf("Secondo argomento non intero\n");
				printf("Error: %s port\n", argv[0]);
				exit(2);
			}
			num1++;
		}  	
	  	port = atoi(argv[1]);
  		if (port < 1024 || port > 65535){
		      printf("Error: %s port\n", argv[0]);
		      printf("1024 <= port <= 65535\n");
		      exit(2);  	
  		}
	}
	
	/* INIZIALIZZAZIONE INDIRIZZO SERVER ----------------------------------------- */
	
	memset ((char *)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(port);
	
	
	/* CREAZIONE E SETTAGGI SOCKET D'ASCOLTO --------------------------------------- */
	listen_sd=socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sd <0)
	{perror("creazione socket "); exit(1);}
	printf("Server: creata la socket d'ascolto per le richieste di ordinamento, fd=%d\n", listen_sd);
	
	udpsd=socket(AF_INET, SOCK_DGRAM, 0);
	if(udpsd <0){perror("creazione socket "); exit(1);}
	printf("Server: creata la socket, sd=%d\n", udpsd);

	if(setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))<0)
	{perror("set opzioni socket d'ascolto"); exit(1);}
	printf("Server: set opzioni socket d'ascolto ok\n");
	
	if(setsockopt(udpsd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))<0)
	{perror("set opzioni socket "); exit(1);}
	printf("Server: set opzioni socket ok\n");

	if(bind(listen_sd,(struct sockaddr *) &servaddr, sizeof(servaddr))<0)
	{perror("bind socket d'ascolto"); exit(1);}
	printf("Server: bind socket d'ascolto ok\n");
	
	if(bind(udpsd,(struct sockaddr *) &servaddr, sizeof(servaddr))<0)
	{perror("bind socket "); exit(1);}
	printf("Server: bind socket ok\n");

	if (listen(listen_sd, 5)<0) //creazione coda d'ascolto
	{perror("listen"); exit(1);}
	printf("Server: listen ok\n");
	
	/*Inizzializzo il set di fd a zero e mi trovo il primo paramentro della select----------------*/
	
	signal(SIGCHLD, gestore);
	FD_ZERO(&fdset);
	maxfd=(listen_sd>udpsd ? listen_sd : udpsd)+1;
	
	/*Entro nel ciclo e aspetto che qualcuno mi contatti -----------------------------------*/
	
	while(true){
		
		FD_SET(listen_sd,&fdset);
		FD_SET(udpsd,&fdset);
		
		if ((nready=select(maxfd, &fdset, NULL, NULL, NULL))<0){
			if(errno==EINTR) continue; 
			else { perror("select"); exit(8);}
		}
		
		
		if(FD_ISSET(udpsd,&fdset)){
			/*Gestisco servizio senza connessione------*/
			
			len=sizeof(struct sockaddr);
			
			if (recvfrom(udpsd, req1, sizeof(Request), 0, (struct sockaddr *)&cliaddr, &len)<0){
				perror("recvfrom");
				continue;
			}
			printf("%s %s\n", req1->nomefile, req1->parola);	
			/* Messaggio ricevuto e salvato in req1
			 * 	Vado a controllare se esiste il file -------------*/
			 
			if((fd=open(req1->nomefile,O_RDONLY))<0){
				perror("Maiala ");
				
				/*Il file non esiste -----------*/
				
				ris1=-1;
				if (sendto(udpsd, &ris1, sizeof(ris1), 0, (struct sockaddr *)&cliaddr, len)<0)
					{perror("sendto"); continue;}
			}else{
				
				/*Il file esiste --------------------------
				 * Vado ad eliminare le occorenze e salvo il dato in ris--------------*/
				 
				ris1=eliminaOcc(fd, req1->nomefile, req1->parola);
				
				/*Invio il dato al client in caso di errore durante l'eliminazione di 
				 * occorenze la funzione eliminaOcc() mi restituisce -2*/
				 
				if (sendto(udpsd, &ris1, sizeof(ris1), 0, (struct sockaddr *)&cliaddr, len)<0)
				{perror("sendto"); continue;}
			}
		}
 
		if(FD_ISSET(listen_sd,&fdset)){
			/*Gestisco servizio con connessiione-------------------*/
			
			/* Provo a stabilire la connessione nel padre e non nel figlio perchè 
			 * potrebbe sucedere che due processi tentano di aprire la stessa connessione*/
			 
			len = sizeof(struct sockaddr);
			
			if((connsd = accept(listen_sd,(struct sockaddr *)&cliaddr,&len))<0){
				if (errno==EINTR) continue;
				else {perror("accept"); exit(9);}
			}		
			
			/* Creo il figlio----------------*/
			if((fork())==0){
				close(listen_sd); 
				printf("Ciao sono il figlio %d\n",getpid());
				
				/*Leggo il nome della directory --------------*/
				
				if ((check1=read(connsd, &req2, sizeof(req2)))<=0)
					{perror("read"); break;}
				
				/* Se mi manda EOF chiudo e termino ----------------------*/	
				
				if(check1==0){
					close(connsd);
					exit(3);
				}
				
				/*Vado a vedere se il nome della cartella è valido----------------------*/
				
				if((dir=opendir(req2))==NULL){
					check=-1;
				}
				
				while(check<0 || check1<0){	
					 /* Dentro ad un ciclo almeno looppo finchè non mi da un nome valido-----
					  * Notifico l'errore*/
					  
					write(connsd, &check, sizeof(check));
					
					/*Attendo il reinvio-------------*/
				
					if ((check1=read(connsd, &req2, sizeof(req2)))<=0)
					{perror("read"); break;}
					
					/* Se mi manda EOF chiudo e termino ----------------------*/	
					
					if(check1==0){
						close(connsd);
						exit(3);
					}
					
					if((dir=opendir(req2))==NULL){
					check=-1;
					}
					
					if(check>=0){
						/*Adesso ho il nome di directory vaido e vado a compiere il mio servizio
						* Ma resto dentro al ciclio che se qualcosa va storto almeno lo notifico e mi metto in attesa
						* 
						* Per differenziare i due tipi di errore faccio che mandaNomiFile() in caso 
						* di errore mi restituisce -2-----*/
						check=mandaNomiFile(connsd,dir);
					}
				}//chiusura del figlio
			}// chiusura del padre
		}
	}
 }
