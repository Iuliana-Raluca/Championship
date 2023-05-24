/* servTCPConcTh2.c - Exemplu de server TCP concurent care deserveste clientii
   prin crearea unui thread pentru fiecare client.
   Asteapta un numar de la clienti si intoarce clientilor numarul incrementat.
	Intoarce corect identificatorul din program al thread-ului.
  
   
   Autor: Lenuta Alboaie  <adria@infoiasi.ro> (c)2009
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>

/* portul folosit */
#define PORT 2930

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
}thData;

struct Campionat
{
	int id;
	char nume_c[200];
	char nume_joc[200];
	int nr_jucatori;
	char regula[200];
	char mod_partide[200];
	char ora[200];
}c[100];
int nr_c;

struct Utilizator
{
	int id;
	char username[200];
	char password[200];
	char mail[200];
	char tip[200];
}u[100];
int nr_u;

struct scor
{
	char nume_c[200];
	char player1[200];
	char scor1[200];
	char player2[200];
	char scor2[200];
}s;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);
void scrie(void *, char msg[200]);		///functie de scriere catre client
void citeste(void *, char msg[200]);	///functie de citire de la client

int valid(char msg[200]);
int verif(char msg[200]);

void add_u(char msg[200]);				///adauga user in baza de date
void add_c(char msg[200]);				///adauga campionat in baza de date
void add_user_on_log_array(char msg[200], int index); 

void jucare_partide(int id_c, int id_u1, int id_u2);

int sqlite_data_create();
int sqlite_data_add_u();				///adauga user in baza de date
int sqlite_data_add_c();				///adauga campionat in baza de date
int sqlite_data_add_s(int scor_1, int scor_2, int id_c, int id_u1, int id_u2);

int sqlite_data_afisare_scor();
int descriptor;

char insert_into[256];

int callbackC(void *NotUsed, int argc, char **argv, char **azColName);
int callbackU(void *NotUsed, int argc, char **argv, char **azColName);
int callbackS(void *NotUsed, int argc, char **argv, char **azColName);

struct logat
{
	int login;	///vectorul de logati (0 - nelogat    1 - user    2-admin)
	char username[200];
	int campionate[100];
	int nr_campionate;
}l[100];



int main ()
{
	struct sockaddr_in server;	// structura folosita de server
	struct sockaddr_in from;	
	int sd;		//descriptorul de socket 
	int pid;
	pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	int i=0;
	
	sqlite_data_create(); /// se creeaza bazele de date
	
		///afisare structura campionat (extrasa din baza de date)
	for(int j = 0; j < nr_c; j++)
	{
		printf("%d : %s : %s : %d : %s : %s : %s\n", c[j].id, c[j].nume_c, c[j].nume_joc, c[j].nr_jucatori, c[j].regula, c[j].mod_partide, c[j].ora);
	}
	
	printf("\n");
	for(int j = 0; j < nr_u; j++)
	{
		printf("%d : %s : %s : %s : %s \n", u[j].id, u[j].username, u[j].password, u[j].tip, u[j].mail);
	}
	


	/* crearea unui socket */
	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("[server]Eroare la socket().\n");
		return errno;
	}
	/* utilizarea optiunii SO_REUSEADDR */
	int on=1;
	setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

	/* pregatirea structurilor de date */
	bzero (&server, sizeof (server));
	bzero (&from, sizeof (from));

	/* umplem structura folosita de server */
	/* stabilirea familiei de socket-uri */
	server.sin_family = AF_INET;	
	/* acceptam orice adresa */
	server.sin_addr.s_addr = htonl (INADDR_ANY);
	/* utilizam un port utilizator */
	server.sin_port = htons (PORT);

	/* atasam socketul */
	if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
	{
		perror ("[server]Eroare la bind().\n");
		return errno;
	}

	/* punem serverul sa asculte daca vin clienti sa se conecteze */
	if (listen (sd, 2) == -1)
	{
		perror ("[server]Eroare la listen().\n");
		return errno;
	}
	
	printf ("\n[server]Asteptam la portul %d...\n",PORT);
	fflush (stdout);
		
	/* servim in mod concurent clientii...folosind thread-uri */
	while (1)
	{
		int client;
		thData * td; //parametru functia executata de thread     
		int length = sizeof (from);

		// client= malloc(sizeof(int));
		/* acceptam un client (stare blocanta pina la realizarea conexiunii) */
		if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
		{
			perror ("[server]Eroare la accept().\n");
			continue;
		}

		/* s-a realizat conexiunea, se astepta mesajul */

		// int idThread; //id-ul threadului
		// int cl; //descriptorul intors de accept

		td=(struct thData*)malloc(sizeof(struct thData));	
		td->idThread=i++;
		td->cl=client;

		pthread_create(&th[i], NULL, &treat, td);	      
			
	}//while    
};				
static void *treat(void * arg)
{	
		
	struct thData tdL; 
	tdL= *((struct thData*)arg);	
	printf ("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
	fflush (stdout);		 
	pthread_detach(pthread_self());
	
	char msg_read[200];
	char msg_write[200];

	
	while(1)
	{	
		bzero(msg_write, 200);
		strcpy(msg_write, "Comenzi disponibile: \"inregistrare\"|\"logare\"|\"quit\"");
		
		scrie((struct thData*)arg,msg_write);
		citeste((struct thData*)arg,msg_read);
		
		if(strcmp(msg_read, "inregistrare") == 0)	///inregistrare a unui utilizator
		{	
			bzero(msg_write, 200);
			strcpy(msg_write, "Comanda introdusa este una corecta");
			
			scrie((struct thData*)arg,msg_write);
			
			bzero(msg_write, 200);
			strcpy(msg_write, "Format:<tip_utilizator> <username> <password> <mail>");
			
			scrie((struct thData*)arg,msg_write);
			citeste((struct thData*)arg,msg_read);
		
			if(valid(msg_read) == 3 && verif(msg_read) == 1)		///daca s-a inregistrat corect
			{
				add_u(msg_read);
				sqlite_data_add_u();
				
				printf("\n");
				for(int j = 0; j < nr_u; j++)
				{
					printf("%d : %s : %s : %s : %s \n", u[j].id, u[j].username, u[j].password, u[j].tip, u[j].mail);
				}
				
				bzero(msg_write, 200);
				strcpy(msg_write, "Inregistrat cu succes");
			}
			else		///altfel xD
			{
				bzero(msg_write, 200);
				strcpy(msg_write, "Inregistrat esuata");
			}
			
			scrie((struct thData*)arg,msg_write);
		} 
		else
		{
			if(strcmp(msg_read, "logare") == 0)		///logare a unui utilizator
			{
				bzero(msg_write, 200);
				strcpy(msg_write, "Comanda corecta");
				
				scrie((struct thData*)arg,msg_write);
				
				bzero(msg_write, 200);
				strcpy(msg_write, "Format:<tip_utilizator> <username> <password>");
				
				scrie((struct thData*)arg,msg_write);
				citeste((struct thData*)arg,msg_read);
				
				if(valid(msg_read) == 2 && verif(msg_read) == 0)		///daca s-a logat corect
				{
					///adauaga userul in vectorul de logati (1 - user    2-admin)
					add_user_on_log_array(msg_read, tdL.idThread); 
					
					bzero(msg_write, 200);
					strcpy(msg_write, "Logare reusita");
					
					scrie((struct thData*)arg,msg_write);
		
					while(1)		///while pt actiuni specifice unui utilizator logat
					{
						
						bzero(msg_write, 200);
						strcpy(msg_write, "Comenzi disponibile: \"info\"|\"inregistrare campionat\"|\"start\"|\"scor\"|\"delogare\"");
						
						scrie((struct thData*)arg,msg_write);
						citeste((struct thData*)arg,msg_read);
						
						if(strcmp(msg_read, "info") == 0)	///informatii campionate
						{
							bzero(msg_write, 200);
							strcpy(msg_write, "Comanda corecta, vei primi informatiile in scurt timp");
							
							scrie((struct thData*)arg,msg_write);	
							
							
							if (write (tdL.cl, &nr_c, sizeof(int)) <= 0)
							{
								perror ("[Thread]Eroare la write() catre client.\n");
							}
							
							for(int i = 0; i < nr_c; i++)
							{
								scrie((struct thData*)arg,c[i].nume_c);
								scrie((struct thData*)arg,c[i].nume_joc);
								if (write (tdL.cl, &c[i].nr_jucatori, sizeof(int)) <= 0)
								{
									perror ("[Thread]Eroare la write() catre client.\n");
								}
								scrie((struct thData*)arg,c[i].regula);
								scrie((struct thData*)arg,c[i].mod_partide);
								scrie((struct thData*)arg,c[i].ora);
							}
							
							bzero(msg_write, 200);
							strcpy(msg_write, "Doriti sa va inregistrati la un campionat?");
							
							scrie((struct thData*)arg,msg_write);
							citeste((struct thData*)arg,msg_read);
							
							
							if(strcmp(msg_read,"da") == 0)		///daca dorim sa ne inreg. la un campionat
							{
								strcpy(msg_write, "Am inregistrat raspunsul dumneavoastra");
								
								scrie((struct thData*)arg,msg_write);
								
								bzero(msg_write, 200);
								strcpy(msg_write, "Format conectare:<nume_campionat>");
								
								scrie((struct thData*)arg,msg_write);
								citeste((struct thData*)arg,msg_read);
								
								
								///parcurgem toate turneele existente in baza de date
								int i;
								for(i = 0; i < nr_c; i++)
								{
									///facem matching pe turneul la care vrea sa se inregistreze
									if(strcmp(c[i].nume_c, msg_read) == 0)
									{	
										
										///adaugam id.ul camp. la care ne-am inregistrat
										///in vectorul de logati, pe id.ul clientului actual logat
										///in vectorul de campionate specific acestuia
										l[tdL.idThread].campionate[l[tdL.idThread].nr_campionate] = i;
										l[tdL.idThread].nr_campionate++;
										
										
										///formam mesajul care urmeaza sa fie trimis catre client
										bzero(msg_write, 200);
										strcpy(msg_write, "Ai fost imregistrat. Ora inceperii: ");
										strcat(msg_write, c[i].ora);
										strcat(msg_write,". Adversarii tai sunt: ");
										for(int j = 0; j < nr_u; j++)///parcurgem toti userii
										{
											if(l[j].nr_campionate != 0 && j != tdL.idThread)
											///pt fiecare user parcurgem camp. la care e inregistrat
											for(int k = 0; k < l[j].nr_campionate; k++)
											{
												///daca gasim matching (nu cu el insusi), inseamna ca i-am gasit un adversar
												if( l[j].campionate[k] ==  i)
												{
												
													///caruia ii concatenam numele in mesajul mai sus format
													strcat(msg_write, l[j].username);		
													strcat(msg_write, " ");
												}
											}
										}
										scrie((struct thData*)arg,msg_write);
										break;
									}
								}
								///daca numele campionatului e gresit
								if(i == nr_c)
								{
									bzero(msg_write, 200);
									strcpy(msg_write, "Campionatul nu a fost gasit");
									
									scrie((struct thData*)arg,msg_write);
								}
							}
							else
							{
								bzero(msg_write, 200);
								strcpy(msg_write, "Va redirectionam catre meniul principal");
								
								scrie((struct thData*)arg,msg_write);
							}
						}
						else
						{
							if(strcmp(msg_read, "inregistrare campionat") == 0)
							{
								if(l[tdL.idThread].login == 2)
								{
									bzero(msg_write, 200);
									strcpy(msg_write, "Esti admin, ai permisiunea sa faci aceasta actiune");
									
									scrie((struct thData*)arg,msg_write);
									
									bzero(msg_write, 200);
									strcpy(msg_write, "Format inregistrare:<nume_campionat> <nume_joc> <nr_jucatori> <regula> <mod_partide> <ora>");
									
									scrie((struct thData*)arg,msg_write);
									
									citeste((struct thData*)arg,msg_read);
									
									
									add_c(msg_read);
									
									sqlite_data_add_c();
									
									for(int j = 0; j < nr_c; j++)
									{
										printf("%d : %s : %s : %d : %s : %s : %s\n", c[j].id, c[j].nume_c, c[j].nume_joc, c[j].nr_jucatori, c[j].regula, c[j].mod_partide, c[j].ora);
									}
									
									bzero(msg_write, 200);
									strcpy(msg_write, "Inregistrare reusita");
									scrie((struct thData*)arg,msg_write);
									
								}
								else
								{
									bzero(msg_write, 200);
									strcpy(msg_write, "Nu esti admin, nu ai permisiunea sa faci aceasta actiune");
									
									scrie((struct thData*)arg,msg_write);
								}
							}
							else
							{
								if(strcmp(msg_read, "start") == 0) 	///start partide
								{
									bzero(msg_write, 200);
									strcpy(msg_write, "Comanda preluata, esti curajos");
									
									scrie((struct thData*)arg,msg_write);
									
									bzero(msg_write, 200);
									strcpy(msg_write, "Introdu numele campionatului pentru care vrei sa incepi partidele de joc:");
									
									scrie((struct thData*)arg,msg_write);
									
									citeste((struct thData*)arg,msg_read);
									
									int ok = -1;
									
									for(int i = 0; i < nr_c; i++)
									{
										///facem matching pe turneul la care vrea sa joace
										if(strcmp(c[i].nume_c, msg_read) == 0)
										{
										 	ok = i;
										}
										
									}
									if(ok > -1)
									{	
										int ok1 = 0;
										for(int k = 0; k < nr_u; k++)
										{
											if(l[k].nr_campionate != 0 && k != tdL.idThread)
											{
												for(int j = 0; j < l[k].nr_campionate; j++)
												{
													if(l[k].campionate[j] == ok)
													{	
														ok1 = 1;
														jucare_partide(ok, tdL.idThread, k);
													}
												}
											}
											
										}
										if(ok1 == 0)
										{
											bzero(msg_write, 200);
											strcpy(msg_write, "Nu exista alti useri inregistrati la acest campionat");
											scrie((struct thData*)arg,msg_write);
										}
										else
										{
											bzero(msg_write, 200);
											strcpy(msg_write, "Partidele au fost jucate. Rezultatul se afla in baza de date accesibila doar adminului");
											scrie((struct thData*)arg,msg_write);
										}
									}
									else
									{
										bzero(msg_write, 200);
										strcpy(msg_write, "Nu esti inregistrat la acest campionat sau campionatul nu exista");
										scrie((struct thData*)arg,msg_write);
									}
										
								}
								else
								{
									if(strcmp(msg_read, "scor") == 0)	///interogare baza de date cu scor
									{
										if(l[tdL.idThread].login == 2)
										{
											bzero(msg_write, 200);
											strcpy(msg_write, "Ai drepturi depline, uite scorurile:");
											
											scrie((struct thData*)arg,msg_write);
											
											descriptor = tdL.cl;
											sqlite_data_afisare_scor();
											
											bzero(msg_write, 200);
											strcpy(msg_write, "Gata");
											
											scrie((struct thData*)arg,msg_write);
											
											
											
										}
										else
										{
											bzero(msg_write, 200);
											strcpy(msg_write, "Doar un admin poate vizualiza scorurile :(\n");
											
											scrie((struct thData*)arg,msg_write);
										}		
									}
									else
									{
										if(strcmp(msg_read, "delogare") == 0)	///delogare
										{
											l[tdL.idThread].login = 0; ///daca s-a delogat, modificam vectorul de logari
											
											bzero(msg_write, 200);
											strcpy(msg_write, "Comanda corecta");
											
											scrie((struct thData*)arg,msg_write);
											break;		
										}
										else
										{
											bzero(msg_write, 200);
											strcpy(msg_write, "Comanda gresita");
											
											scrie((struct thData*)arg,msg_write);
										}
									}
									
								}
							}
						}
					}	///while actiuni
						
					bzero(msg_write, 200);
					strcpy(msg_write, "Inchideti aplicatia?");
					
					scrie((struct thData*)arg,msg_write);
					citeste((struct thData*)arg,msg_read);
					
					if(strcmp(msg_read, "da") == 0)
					{
						bzero(msg_write, 200);
						strcpy(msg_write, "Aplicatia a fost inchisa");
						
						scrie((struct thData*)arg,msg_write);
						break;		///parasim while.ul de actiuni
					}
					else
					{
						bzero(msg_write, 200);
						strcpy(msg_write, "Vei fi redirectionat la meniul principal");
						
						scrie((struct thData*)arg,msg_write);
					}
				}
				else		///daca nu s-a logat corect
				{
					bzero(msg_write, 200);
					strcpy(msg_write, "Logare esuata");
					
					scrie((struct thData*)arg,msg_write);
				}
			}	///if logare
			else
			{
				if(strcmp(msg_read, "quit") == 0) ///quit
				{
					bzero(msg_write, 200);
					strcpy(msg_write, "Aplicatia a fost inchisa");
					
					scrie((struct thData*)arg,msg_write);
					break;
				}
				else
				{
				
					bzero(msg_write, 200);
					strcpy(msg_write, "Comanda gresita!");
					
					scrie((struct thData*)arg,msg_write);
				}
			}
		}
		
		
	} ///while
	
	///raspunde((struct thData*)arg);
	/* am terminat cu acest client, inchidem conexiunea */
	close ((intptr_t)arg);
	return(NULL);	

}

void scrie (void *arg, char msg[200])
{
	struct thData tdL; 
	tdL= *((struct thData*)arg);
	
	if (write (tdL.cl, msg, 200) <= 0)
	{
		printf("[Thread %d] ",tdL.idThread);
		perror ("[Thread]Eroare la write() catre client.\n");
	}
}

void citeste (void *arg, char msg[200])
{
	struct thData tdL; 
	tdL= *((struct thData*)arg);
	
	if (read (tdL.cl, msg, 200) <= 0)
	{
		printf("[Thread %d] ",tdL.idThread);
		perror ("[Thread]Eroare la read() de la client.\n");
	}
}

int valid(char msg[200])
{
	int cnt = 0;
	for(int i = 0; i < strlen(msg); i++)
	{
		if(!(msg[i] >= 'a' && msg[i] <= 'z' || msg[i] == ' ' || msg[i] == '.' || msg[i] == '@'))
			return 0;
		if(msg[i] == ' ')
		{
			cnt++;
			if(!(msg[i+1] >= 'a' && msg[i+1] <= 'z'))
				return 0;
		}
	}	
	
	char *cuv;
	char copie[200];
	strcpy(copie, msg);
	cuv = strtok(copie, " ");
	if(strcmp(cuv, "user") != 0 && strcmp(cuv, "admin") != 0)
		return 0;
	
	if(cnt == 2)
		return 2;
	if(cnt == 3 )
		return 3;
	
	return 0;
}

int verif(char msg[200])
{
	char *cuv1, *cuv2, *cuv3;
	char copie[200];
	strcpy(copie, msg);
	cuv1 = strtok(copie, " ");
	cuv2 = strtok(NULL, " ");
	cuv3 = strtok(NULL, " ");
	for(int i = 0; i < nr_u; i++)
	{
		if(strcmp(cuv2, u[i].username) == 0 && strcmp(cuv1, u[i].tip) == 0 && strcmp(cuv3, u[i].password) == 0)
			return 0;
	}
	return 1;
}

void add_u(char msg[200])
{
	char *cuv1, *cuv2, *cuv3, *cuv4;
	char copie[200];
	strcpy(copie, msg);
	cuv1 = strtok(copie, " ");
	cuv2 = strtok(NULL, " ");
	cuv3 = strtok(NULL, " ");
	cuv4 = strtok(NULL, " ");
	u[nr_u].id = nr_u;
	strcpy(u[nr_u].tip, cuv1);
	strcpy(u[nr_u].username, cuv2);
	strcpy(u[nr_u].password, cuv3);
	strcpy(u[nr_u].mail, cuv4);
	nr_u++;
}

void add_c(char msg[200])
{
	char *cuv1, *cuv2, *cuv3, *cuv4, *cuv5, *cuv6;
	char copie[200];
	strcpy(copie, msg);
	cuv1 = strtok(copie, " ");
	cuv2 = strtok(NULL, " ");
	cuv3 = strtok(NULL, " ");
	cuv4 = strtok(NULL, " ");
	cuv5 = strtok(NULL, " ");
	cuv6 = strtok(NULL, " ");
	c[nr_c].id = nr_c;
	strcpy(c[nr_c].nume_c, cuv1);
	strcpy(c[nr_c].nume_joc, cuv2);
	char copie2[200];
	strcpy(copie2, cuv3);
	c[nr_c].nr_jucatori = atoi(copie2);
	strcpy(c[nr_c].regula, cuv4);
	strcpy(c[nr_c].mod_partide, cuv5);
	strcpy(c[nr_c].ora, cuv6);
	nr_c++;
}

void add_user_on_log_array(char msg[200], int index)
{
	char *cuv1, *cuv2;
	char copie[200];
	strcpy(copie, msg);
	cuv1 = strtok(copie, " ");
	cuv2 = strtok(NULL, " ");
	
	strcpy(l[index].username, cuv2);	///savlez numele userului logat in structura de logati
	
	///savlez tipul userului logat in structura de logati
	if(strcmp(cuv1, "admin") == 0)
		l[index].login = 2;			
	else
		l[index].login = 1;
}

void jucare_partide(int id_c, int id_u1, int id_u2)
{
	int scor_player1 = rand()%10;
	int scor_player2 = rand()%10;
	
	sqlite_data_add_s(scor_player1, scor_player2, id_c, id_u1, id_u2);	
}

int sqlite_data_create()
{
	sqlite3 *dbC;
    char *err_msg = 0;
    
    int rcC = sqlite3_open("Campionate.db", &dbC);
    
   	if (rcC != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(dbC));
        sqlite3_close(dbC);
        
        return 1;
    }
    
    char *sqlC = "DROP TABLE IF EXISTS Campionate;" 
                "CREATE TABLE Campionate(Id INT, Denumire_Campionat TEXT, Denumire_Joc TEXT, Numar_Jucatori INT, Regula TEXT, Mod_Extragere TEXT, Ora TEXT);" 
                "INSERT INTO Campionate VALUES(0, 'next star', 'karaoke' , 20, 'eliminare multipla', 'bootcamp', '12:00');" 
                "INSERT INTO Campionate VALUES(1, 'apolo cup', 'box', 12, 'KO', '1 vs 1', '23:00');"
                "INSERT INTO Campionate VALUES(2, 'under the gun', 'poker', 100, 'ffa', 'mese multiple', '19:00');" 
                "INSERT INTO Campionate VALUES(3, 'chess master', 'chess', 32, 'eliminare singulara', 'grupe', '16:30');" 
                "INSERT INTO Campionate VALUES(4, 'backgammon 10/20', 'backgammon', 8, 'eliminare singulara', 'Optimi', '18:25');";
                
    rcC = sqlite3_exec(dbC, sqlC, 0, 0, &err_msg);
    
    char *sqllC = "SELECT * FROM Campionate";
    
    nr_c = 0;
        
    rcC = sqlite3_exec(dbC, sqllC, callbackC, 0, &err_msg);
    
    if (rcC != SQLITE_OK ) {
        
        fprintf(stderr, "Failed to select data\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(dbC);
        
        return 1;
    } 
    
    sqlite3_close(dbC);
    
    ////##############
    
    sqlite3 *dbU;
    
    int rcU = sqlite3_open("Utilizatori.db", &dbU);
    
   	if (rcU != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(dbU));
        sqlite3_close(dbU);
        
        return 1;
    }
    
    
    char *sqlU = "DROP TABLE IF EXISTS Utilizatori;" 
                "CREATE TABLE Utilizatori(Id INT, Username TEXT, Password TEXT, Mail TEXT, Tip TEXT);" 
                "INSERT INTO Utilizatori VALUES(0, 'iulica', 'iulica', 'iulik@mail.com', 'admin');" 
                "INSERT INTO Utilizatori VALUES(1, 'florin', 'florin', 'florin_baiat_fin@mail.com', 'admin');"
                "INSERT INTO Utilizatori VALUES(2, 'crocodel', 'crocodel', 'croko@mail.com', 'user');";
                
    rcU = sqlite3_exec(dbU, sqlU, 0, 0, &err_msg);

	
    char *sqllU = "SELECT * FROM Utilizatori";
    
    nr_u = 0;
        
    rcU = sqlite3_exec(dbU, sqllU, callbackU, 0, &err_msg);
    
    if (rcU != SQLITE_OK ) {
        
        fprintf(stderr, "Failed to select data\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(dbU);
        
        return 1;
    } 
   
    
    sqlite3_close(dbU);
    
    ///###########
    
    sqlite3 *dbS;
    
    int rcS = sqlite3_open("Scor.db", &dbS);
    
   	if (rcS != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(dbS));
        sqlite3_close(dbS);
        
        return 1;
    }
    
    
    char *sqlS = "DROP TABLE IF EXISTS Scor;" 
                "CREATE TABLE Scor(Nume_campionat TEXT, Nume_player1 TEXT, Scor_player_1 INT, Nume_player2 TEXT, Scor_player_2 INT);" ;
                
    rcS = sqlite3_exec(dbS, sqlS, 0, 0, &err_msg);
    
    if (rcS != SQLITE_OK ) {
        
        fprintf(stderr, "Failed to select data\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(dbS);
        
        return 1;
    } 
   
    
    sqlite3_close(dbS);
}

int callbackC(void *NotUsed, int argc, char **argv, char **azColName)
{
    NotUsed = 0;
    
    //extragem informatiile din tabelul Campionate
    
    c[nr_c].id = atoi(argv[0]);
	strcpy(c[nr_c].nume_c, argv[1]);
	strcpy(c[nr_c].nume_joc, argv[2]);
	c[nr_c].nr_jucatori = atoi(argv[3]);
	strcpy(c[nr_c].regula, argv[4]);
	strcpy(c[nr_c].mod_partide, argv[5]);
	strcpy(c[nr_c].ora, argv[6]);
	nr_c++;
    
    return 0;
}

int callbackU(void *NotUsed, int argc, char **argv, char **azColName)
{
    NotUsed = 0;
    
    //extragem informatiile din tabelul Utilizatori
	u[nr_u].id = atoi(argv[0]);
	strcpy(u[nr_u].username, argv[1]);
	strcpy(u[nr_u].password, argv[2]);
	strcpy(u[nr_u].mail, argv[3]);
	strcpy(u[nr_u].tip, argv[4]);
	nr_u++;
    
    return 0;
}

int sqlite_data_add_u()
{
	sqlite3 *dbU;
	char *err_msg = 0;
	
	int rcU = sqlite3_open("Utilizatori.db", &dbU);
	
   	if (rcU != SQLITE_OK) {
		
		fprintf(stderr, "Cannot open database: %s\n", 
			    sqlite3_errmsg(dbU));
		sqlite3_close(dbU);
		
	}
	
	char insert_into[256];
	bzero(insert_into, 256);
				
	strcpy(insert_into, "INSERT INTO Utilizatori VALUES(");
	char id[100] = "";
	id[0] =  (char)(u[nr_u - 1].id + '0');
	strcat(insert_into, id);
	strcat(insert_into, ", '");
	strcat(insert_into, u[nr_u - 1].username);
	strcat(insert_into, "', '");
	strcat(insert_into, u[nr_u - 1].password);
	strcat(insert_into, "', '");
	strcat(insert_into, u[nr_u - 1].mail);
	strcat(insert_into, "', '");
	strcat(insert_into, u[nr_u - 1].tip);
	strcat(insert_into, "');");
	
	printf("\n%s\n", insert_into);
	
			    
	rcU = sqlite3_exec(dbU, insert_into, 0, 0, &err_msg);
	
	if (rcU != SQLITE_OK ) {
		
		fprintf(stderr, "Failed to select data\n");
		fprintf(stderr, "SQL error: %s\n", err_msg);

		sqlite3_free(err_msg);
		sqlite3_close(dbU);
		
	} 
   
	
	sqlite3_close(dbU);
}

int sqlite_data_add_c()
{
	sqlite3 *dbC;
	char *err_msg = 0;
	
	int rcC = sqlite3_open("Campionate.db", &dbC);
	
   	if (rcC != SQLITE_OK) {
		
		fprintf(stderr, "Cannot open database: %s\n", 
			    sqlite3_errmsg(dbC));
		sqlite3_close(dbC);
		
	}
	
	char insert_into[256];
	bzero(insert_into, 256);
				
	strcpy(insert_into, "INSERT INTO Campionate VALUES(");
	char id[100] = "";
	id[0] =  (char)(c[nr_c - 1].id + '0');
	strcat(insert_into, id);
	strcat(insert_into, ", '");
	strcat(insert_into, c[nr_c - 1].nume_c);
	strcat(insert_into, "', '");
	strcat(insert_into, c[nr_c - 1].nume_joc);
	strcat(insert_into, "', ");
	id[0] =  (char)(c[nr_c - 1].nr_jucatori + '0');
	strcat(insert_into, id);
	strcat(insert_into, ", '");
	strcat(insert_into, c[nr_c - 1].regula);
	strcat(insert_into, "', '");
	strcat(insert_into, c[nr_c - 1].mod_partide);
	strcat(insert_into, "', '");
	strcat(insert_into, c[nr_c - 1].ora);
	strcat(insert_into, "');");
	
	printf("\n%s\n", insert_into);
	
			    
	rcC = sqlite3_exec(dbC, insert_into, 0, 0, &err_msg);
	
	if (rcC != SQLITE_OK ) {
		
		fprintf(stderr, "Failed to select data\n");
		fprintf(stderr, "SQL error: %s\n", err_msg);

		sqlite3_free(err_msg);
		sqlite3_close(dbC);
		
	} 
   
	
	sqlite3_close(dbC);
}

int sqlite_data_add_s(int scor_1, int scor_2, int id_c, int id_u1, int id_u2)
{
	sqlite3 *dbS;
	char *err_msg = 0;
	
	int rcS = sqlite3_open("Scor.db", &dbS);
	
   	if (rcS != SQLITE_OK) {
		
		fprintf(stderr, "Cannot open database: %s\n", 
			    sqlite3_errmsg(dbS));
		sqlite3_close(dbS);
		
	}
	
	char insert_into[256];
	bzero(insert_into, 256);
				
	strcpy(insert_into, "INSERT INTO Scor VALUES('");
	
	for(int i = 0; i < nr_c; i++)
	{
		if(c[i].id == id_c)
		{
			strcat(insert_into, c[i].nume_c);
			break;
		}
	}
	
	strcat(insert_into, "', '");
	
	for(int i = 0; i < nr_u; i++)
	{
		if(u[i].id == id_u1)
		{
			strcat(insert_into, u[i].username);
			break;
		}
	}
	
	strcat(insert_into, "', ");
	char id[100] = "";
	id[0] =  (char)(scor_1 + '0');
	
	strcat(insert_into, id);
	strcat(insert_into, ", '");
	
	for(int i = 0; i < nr_u; i++)
	{
		if(u[i].id == id_u2)
		{
			strcat(insert_into, u[i].username);
			break;
		}
	}
	
	strcat(insert_into, "', ");
	
	id[0] =  (char)(scor_2 + '0');
	strcat(insert_into, id);
	strcat(insert_into, ");");

	printf("\n%s\n", insert_into);
	
			    
	rcS = sqlite3_exec(dbS, insert_into, 0, 0, &err_msg);
	
	if (rcS != SQLITE_OK ) {
		
		fprintf(stderr, "Failed to select data\n");
		fprintf(stderr, "SQL error: %s\n", err_msg);

		sqlite3_free(err_msg);
		sqlite3_close(dbS);
		
	} 
	
	sqlite3_close(dbS);
}

int sqlite_data_afisare_scor()
{
	sqlite3 *dbS;
	char *err_msg = 0;
	
	int rcS = sqlite3_open("Scor.db", &dbS);
	
   	if (rcS != SQLITE_OK) {
		
		fprintf(stderr, "Cannot open database: %s\n", 
			    sqlite3_errmsg(dbS));
		sqlite3_close(dbS);
		
	}
	
	char *sqllS = "SELECT * FROM Scor";
    
    nr_u = 0;
        
    rcS = sqlite3_exec(dbS, sqllS, callbackS, 0, &err_msg);
    
    if (rcS != SQLITE_OK ) {
		
		fprintf(stderr, "Failed to select data\n");
		fprintf(stderr, "SQL error: %s\n", err_msg);

		sqlite3_free(err_msg);
		sqlite3_close(dbS);
		
	} 
}

int callbackS(void *NotUsed, int argc, char **argv, char **azColName)
{
    NotUsed = 0;
    
    //afisam scorul
    
    strcpy(s.nume_c, argv[0]);
    strcpy(s.player1, argv[1]);
    strcpy(s.scor1, argv[2]);
    strcpy(s.player2, argv[3]);
    strcpy(s.scor2, argv[4]);
    
    if (write (descriptor, s.nume_c, 200) <= 0)
	{
		printf("[Thread %d] ",descriptor);
		perror ("[Thread]Eroare la write() catre client.\n");
	}
	if (write (descriptor, s.player1, 200) <= 0)
	{
		printf("[Thread %d] ",descriptor);
		perror ("[Thread]Eroare la write() catre client.\n");
	}
	if (write (descriptor, s.scor1, 200) <= 0)
	{
		printf("[Thread %d] ",descriptor);
		perror ("[Thread]Eroare la write() catre client.\n");
	}
	if (write (descriptor, s.player2, 200) <= 0)
	{
		printf("[Thread %d] ",descriptor);
		perror ("[Thread]Eroare la write() catre client.\n");
	}
	if (write (descriptor, s.scor2, 200) <= 0)
	{
		printf("[Thread %d] ",descriptor);
		perror ("[Thread]Eroare la write() catre client.\n");
	}
    
    
    return 0;
}

void raspunde(void *arg)
{
    int nr, i=0;
	struct thData tdL; 
	tdL= *((struct thData*)arg);
	if (read (tdL.cl, &nr,sizeof(int)) <= 0)
	{
		printf("[Thread %d]\n",tdL.idThread);
		perror ("Eroare la read() de la client.\n");
	}
	
	printf ("[Thread %d]Mesajul a fost receptionat...%d\n",tdL.idThread, nr);
		      
	/*pregatim mesajul de raspuns */
	nr++;      
	printf("[Thread %d]Trimitem mesajul inapoi...%d\n",tdL.idThread, nr);
		      
		      
	/* returnam mesajul clientului */
	if (write (tdL.cl, &nr, sizeof(int)) <= 0)
	{
		printf("[Thread %d] ",tdL.idThread);
		perror ("[Thread]Eroare la write() catre client.\n");
	}
	else
		printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
}
///gcc -pthread ChampionshipServer.c -o ChampionshipServer -lsqlite3
