/* cliTCPIt.c - Exemplu de client TCP
   Trimite un numar la server; primeste de la server numarul incrementat.
         
   Autor: Lenuta Alboaie  <adria@infoiasi.ro> (c)2009
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

void scrie(int sd, char msg[200]);		///functie de scriere catre server
void citeste(int sd, char msg[200]);	///functie de citire de la server

int main (int argc, char *argv[])
{
	int sd;			// descriptorul de socket
	struct sockaddr_in server;	// structura folosita pentru conectare 
	// mesajul trimis
	int nr=0;
	char buf[10];
	
	char msg_read[200];
	char msg_write[200];

	/* exista toate argumentele in linia de comanda? */
	if (argc != 3)
	{
		printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
		return -1;
	}

	/* stabilim portul */
	port = atoi (argv[2]);

	/* cream socketul */
	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("Eroare la socket().\n");
		return errno;
	}

	/* umplem structura folosita pentru realizarea conexiunii cu serverul */
	/* familia socket-ului */
	server.sin_family = AF_INET;
	/* adresa IP a serverului */
	server.sin_addr.s_addr = inet_addr(argv[1]);
	/* portul de conectare */
	server.sin_port = htons (port);

	/* ne conectam la server */
	if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
	{
		perror ("[client]Eroare la connect().\n");
		return errno;
	}
	
	printf("[client]=========================\n");
	printf("[client]=====CLIENT CONECTAT=====\n");
	printf("[client]=========================\n");

	while(1)
	{
	
		citeste(sd, msg_read);
		
		printf ("\n\n[client]%s\n", msg_read);
		/* citirea mesajului */
		printf ("[client]Introduceti comanda: ");
		fflush (stdout);
		
		bzero(msg_write, 200);
		read (0, msg_write, 200);
		
		for(int i = 0; i < strlen(msg_write); i++)
		{
			if( !( (msg_write[i] >= 'a' && msg_write[i] <= 'z') || (msg_write[i] >= '0' && msg_write[i] <= '9') || msg_write[i] == ' ' || msg_write[i] == '.' || msg_write[i] == '@' ) )
			{
				strcpy(msg_write + i, msg_write + i + 1);
				i--;
			}
		}

		/* trimiterea mesajului la server */
		scrie(sd, msg_write);

		/* citirea raspunsului dat de server 
		(apel blocant pina cind serverul raspunde) */
		
		citeste(sd, msg_read);
		
		/* afisam mesajul primit */
		printf ("[client]%s\n", msg_read);
		if(strcmp(msg_read, "Aplicatia a fost inchisa") == 0)
			break;
		if(strcmp(msg_read, "Comanda corecta, vei primi informatiile in scurt timp") == 0)
		{
			int n;
			if (read (sd, &n, sizeof(int)) < 0)
			{
				perror ("[client]Eroare la read() de la server.\n");
				return errno;
			}
			printf("[client]In total sunt %d campionate:\n", n);
			for(int i = 0; i < n; i++)
			{
				citeste(sd,msg_read);
				printf("\n[client]Nume campionat: %s\n", msg_read);
				citeste(sd,msg_read);
				printf("[client]Nume joc: %s\n", msg_read);
				int nr;
				if (read (sd, &nr, sizeof(int)) <= 0)
				{
					perror ("[client]Eroare la read() de la server.\n");
				}
				printf("[client]Numar jucatori: %d\n", nr);
				citeste(sd,msg_read);
				printf("[client]Regula: %s\n", msg_read);
				citeste(sd,msg_read);
				printf("[client]Partide: %s\n", msg_read);
				citeste(sd,msg_read);
				printf("[client]Ora inceperii: %s\n", msg_read);
			}
		}
		if(strcmp(msg_read, "Ai drepturi depline, uite scorurile:") == 0)
		{
			citeste(sd,msg_read);
			printf("\n\n[client]Nume Campionat: %s\n", msg_read);
			do
			{
				citeste(sd,msg_read);
				printf("[client]Nume Player1: %s\n", msg_read);
				citeste(sd,msg_read);
				printf("[client]Scor Player1: %s\n", msg_read);
				citeste(sd,msg_read);
				printf("[client]Nume Player2: %s\n", msg_read);
				citeste(sd,msg_read);
				printf("[client]Scor Player2: %s\n", msg_read);
				citeste(sd,msg_read);
				if(strcmp(msg_read, "Gata") != 0)
					printf("\n\n[client]Nume Campionat: %s\n", msg_read);
			}while(strcmp(msg_read, "Gata") != 0);
		}
	}

	/* inchidem conexiunea, am terminat */
	close (sd);
}
void scrie (int sd, char msg[200])	///functie de scriere a unui char
{
	if (write (sd, msg, 200) <= 0)
	{
		perror ("[client]Eroare la write() catre server.\n");
	}
}

void citeste (int sd, char msg[200])	///functie de citire a unui char
{
	
	if (read (sd, msg, 200) <= 0)
	{
		perror ("[client]Eroare la read() de la server.\n");
	}
}
///gcc ChampionshipClient.c -o ChampionshipClient
