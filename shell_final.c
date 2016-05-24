#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h> /* FORK - EXEC */
#include <string.h> /* Fonction sur les string ex: STRCPY */
#include <fcntl.h> /* OPEN */
#include <errno.h> /* EAGAIN - EINTR */

void inviteCommande();
char lireCommande();
void executionCommande();
void changeDirectory();
void testCommande();
void parsing();
void addCommandeHistory();
void commandeHistory();
void commandeTouch();
void commandeCat();
void commandeCP();
void redirection();

char * commande;
char ** commandeParse;
int nbArg;

int main()
{
	while(1) {
		inviteCommande();
		lireCommande();
	}	

	free(commande);
	int i;
	for(i=0; i < nbArg; i++){
		free(commandeParse[i]);
	}
	free(commandeParse);
	return 0;
}

void inviteCommande(){
	/* Récupere le hostname -> nom de la machine */
	char hostName[1024];
	if ( gethostname(hostName, 1024) ) {
		return EXIT_FAILURE;
	}
	/* Récupere le repertoireCourant */
	char repCourant[1024];
	if ( getcwd (repCourant, 1024) == NULL ) {
		return EXIT_FAILURE;
	}
	/* Supprime tous aprés le premier point dans le hostname*/

	*strchr(hostName, '.') = '\0';

	/* Affiche l'invite de commande */
	printf("[%s@%s : %s]$ ", getenv("USER"), hostName, repCourant);
}

char lireCommande(){
	char buffer[1024];
	/* Lit l'entrée standard et quitte si l'utilisateur utilise CTRL + D */
	if(!fgets(buffer, sizeof(buffer)-1, stdin)){
		exit(1);
	}

	/* Supprime le \n a la fin du buffer */
	char *p = strchr(buffer, '\n');
	if(p)
	{
		*p = 0;
	}

	/* Alloue l'espace au pointeur commande et y ajoute la commande */
	commande = malloc(sizeof(buffer));
	strcpy(commande, buffer);
	parsing();
}

void parsing() {
	char * buffer;
	nbArg = 0;
	buffer = (char* ) malloc (sizeof(commande));

	/* Recupere le string jusqu'au premiere espace */
	buffer = strtok (commande, " ");
	
	/* Alloue la mémoire au tableau 2D */
	commandeParse = (char**) malloc (sizeof(char*));

	/* Boucle sur tous les mots de la ligne de commande */
	while ( buffer != NULL ) {
		/* Alloue de la mémoire pour ajouter le mots */
		commandeParse[nbArg] = (char *) malloc (sizeof(buffer));
		/* Ajoute l'élément */
		strcpy(commandeParse[nbArg], buffer);
		nbArg++;
		/* Recupere le string jusqu'au premiere espace */
		buffer = strtok(NULL, " " );
		/* Ajoute de l'espace pour un pointeur de char si ce n'est pas le dernier mot */
		if ( buffer != NULL ) {
			 commandeParse = realloc ( commandeParse, sizeof(commandeParse) + sizeof(buffer));
		}
	}
	free(buffer);

	commandeParse = realloc ( commandeParse, sizeof(char*));
	commandeParse[nbArg] = (char *) malloc (sizeof(char));
	commandeParse[nbArg] = NULL;

	addCommandeHistory();
	testCommande();	
}

void testCommande(){

	// Test si l'utilisateur a rediriger la sortie 
	int i, placeFichier;
	for(i=0, placeFichier=0; i<nbArg; i++, placeFichier++){
		if ( !strcmp(commandeParse[i], ">" ) ) { /* Test si user à rediriger la sortie */
			redirection(++placeFichier);
			return;
		}
	}

	if ( !commandeParse[0] ) /* Si commande null ne fais rien et relance un invite commande */
		;
	else if ( !strcmp(commandeParse[0], "cd" ) ) /* Test si user utilise CD */
		changeDirectory();
	else if ( !strcmp(commandeParse[0], "history" ) ) /* Test si user utilise HISTORY */
		commandeHistory();

 	else if ( !strcmp(commandeParse[0], "touch" ) ) /* Test si user utilise TOUCH */
		commandeTouch();
	else if ( !strcmp(commandeParse[0], "exit" ) ) /* Test si user utilise EXIT */ 
		exit(0);
	else if ( !strcmp(commandeParse[0], "cat" ) ) /* Test si user utilise CAT */ 
		commandeCat();
	else if ( !strcmp(commandeParse[0], "cp" ) ) /* Test si user utilise CP */
		commandeCP();
	else /* Exécution de commande avec exec */ 
		executionCommande();

}

void executionCommande() {
	pid_t idpr;
	int cptRendu;
	idpr = fork();

	/* Test si fork reussi */
	if ( idpr < 0 )
	{
		fprintf(stderr, "ERREUR : FORK processus %d. \n ", getpid());
		exit (1);
	}
	/* Code du processus fils */
	if ( idpr == 0 )
	{	
		// On récupére tout les dossiers d'environnemt qui peuvent contenir les commandes
		char *path = getenv("PATH");

		int tailleEnv, i =0;
		char * buffer;
		buffer = calloc(1024,sizeof(char));
		
		
		for(tailleEnv = 0; tailleEnv < strlen(path); tailleEnv++){
			// On boucle sur les carectéres jusqu'a ce qu'on arrive au délimiteur 
			if(path[tailleEnv]==':'){
				// Ajoute la commande a la suite du path
				strcat(buffer, "/");
				strcat(buffer, commandeParse[0]);
				//Test s
				if(!access(buffer,X_OK)){
					/* Utilisation de execv car on fourni le chemin de la commande */	
					execv		(buffer,commandeParse);
					fprintf(stderr,"ERREUR : EXEC processus %d. \n", getpid());
					exit(0);

				} else {
					free(buffer);
					buffer = calloc(4096,sizeof(char));
				}
				// On réinitialise le compteur
				i=0;
			} else /* Insere le path au buffer */
			{
				buffer[i++] = path[tailleEnv];
			}
		}
		free(buffer);
		fprintf(stderr,"ERREUR : Commande introuvable. \n");
		exit(0);
	}

	waitpid(idpr, &cptRendu, WCONTINUED );
}

void changeDirectory() {
	/* Test si nombre d'argument bien égale à 2 */
	if ( nbArg != 2 ){
		fprintf(stderr,"ERREUR : Nb Argument cd invalide. \n ");
	}
	else if ( chdir (commandeParse[1]) ){ /* Change le répertoire courant avec le deuxieme argument */
		fprintf(stderr,"ERREUR : Répertoire non éxistant. \n ");
	}
}

void commandeTouch(){
	/* On ouvre le fichier et on le crée si il éxiste pas déja */
	int descFichier = open( commandeParse[1], O_WRONLY | O_CREAT,  0660);
	// Test si le fichier c'est correctement ouvert
	if( descFichier < 0 )
	{
		fprintf(stderr, "ERREUR: Ouverture/Création fichier \n.");
		return;
	} 
	
	struct timespec buffer[2]; 
	buffer[1].tv_nsec = UTIME_NOW; 
	if(commandeParse[2] && strcmp(commandeParse[2], "-m") == 0) 
		buffer[0].tv_nsec = UTIME_OMIT; 
	else
		 buffer[0].tv_nsec = UTIME_NOW; 
	futimens(descFichier, buffer); 
	
	// Ferme le fichier ouvert
	close(descFichier);	
}

void commandeCat(){
	char line [1024];
	if ( nbArg == 2){
		FILE *file = fopen ( commandeParse[1], "r" );
		if (file != NULL) {
			while ( fgets ( line, sizeof(line), file ) != NULL ) /* read a line */
			{
				printf("%s", line);
			}
			fclose ( file ); 
		} else {
			perror(commandeParse[1]);
		}
	} else if ( nbArg == 3 ) {
		if (!strcmp(commandeParse[1], "-n" )){
			int cpt = 0;
			FILE *file = fopen ( commandeParse[2], "r" );
			if (file != NULL) {
				while ( fgets ( line, sizeof(line), file ) != NULL ) /* read a line */
				{
					printf("%d %s", ++cpt,  line);
				}
				fclose ( file );
			} else {
				perror(commandeParse[2]);
			}			
		} else {
			int i;
			for (i = 1; i <= 2; i++) {
				FILE *file = fopen ( commandeParse[i], "r" );
				if (file != NULL) {
					while ( fgets ( line, sizeof(line), file ) != NULL ) /* read a line */
					{
						printf("%s",  line);
					}
					fclose ( file );
				} else {
					perror(commandeParse[i]);
				}
			}
		}
	} else {
		fprintf(stderr, "ERREUR : Nb Argument cat invalide. \n");
	}
}

void addCommandeHistory(){
	// Ouvre le fichier History
	int descFichierHistory = open( "fichierHistory.txt", O_WRONLY | O_APPEND | O_CREAT, 0666 );
	// Test si le fichier c'est correctement ouvert
	if( descFichierHistory < 0 )
	{
		fprintf(stderr,"ERREUR: Ouverture fichier History. \n");
		exit(2);
	}

	// Ecrit la commande dans le fichier
	write( descFichierHistory, commande, strlen(commande));
	// Ajoute un retour a la ligne dans le fichier
	write( descFichierHistory, "\n", strlen("\n"));

	close( descFichierHistory );
}

void commandeHistory(){
	char line [1024];
	FILE *file = fopen ( "fichierHistory.txt", "r" );
	if (file != NULL) {
		if ( nbArg == 1){
			int cpt = 1;
			while ( fgets ( line, sizeof(line), file ) != NULL ) /* read a line */
			{
				printf("%d %s", cpt,  line);
				cpt++; 
			}
		} else if ( nbArg == 2 ) {
			int cpt= 1 ;
			if ( commandeParse[1][0] == '!' ) {
				commandeParse[1][0] = '0';
				int n = atoi(commandeParse[1]);

				while ( fgets ( line, sizeof(line), file ) != NULL && cpt <= n){
					if ( n == cpt ) {
						/* Supprime le \n a la fin du buffer */
						char *p = strchr(line, '\n');
						if(p)
						{
							*p = 0;
						}
						commande = malloc(sizeof(line));
						strcpy(commande, line);


						parsing();
					}
					cpt++;
				}
				
				commande = malloc(sizeof(line));
				strcpy(commande, line);
				
				
				
			} else {
				int nbLigne = 1;
				while ( fgets ( line, sizeof(line), file ) != NULL ) /* read a line */
				{
					nbLigne++; 
				}

				// Replace le pointeur au début de fichier
				rewind(file);

				while ( fgets ( line, sizeof(line), file ) != NULL ) /* read a line */
				{
					if ( cpt >= nbLigne - atoi(commandeParse[1]) )
					{
						printf("%d %s", cpt,  line);
					}
					cpt++; 
				}			
			}

		}

		fclose ( file ); 
	} else {
		perror("fichierHistory.txt");
	}
}

void commandeCP(){
	pid_t idpr;
	int cptRendu;
	idpr = fork();

	/* Test si fork reussi */
	if ( idpr < 0 )
	{
		fprintf(stderr, "ERREUR : FORK processus %d. \n ", getpid());
		exit (1);
	}
	/* Code du processus fils */
	if ( idpr == 0 )
	{		
		/* Utilisaion de execv car on fourni le chemin de la commande */
		execvp("./commandeCP",commandeParse);
		fprintf(stderr,"ERREUR : EXEC processus %d. \n", getpid());
		exit(0);
	}

	waitpid(idpr, &cptRendu, WCONTINUED );

}

void redirection(int placeFichier){

	// Ouvre le fichier dans le quel on veux rediriger la sortie 	
	int descFichierCible = open( commandeParse[placeFichier], O_WRONLY | O_APPEND | O_CREAT, 0666 );
	// Test si le fichier c'est correctement ouvert
	if( descFichierCible < 0 )
	{
		fprintf(stderr,"ERREUR: Ouverture fichier %s. \n", commandeParse[placeFichier]);
		return;
	} else {
		// Sauvegarde la sortie standard
		int oldout = dup(1);		
		dup2(descFichierCible,1);
		close(descFichierCible);

		// On exécute la commande du coup on supprime la redirection
		char * buffer = NULL;
		buffer = (char*) malloc (1024 * sizeof(char));
		strcpy(buffer,commandeParse[0]);
		int i;
		printf("PlaceFichier %d \n", placeFichier);
		for(i=0; i< (placeFichier-1); i++){
			fprintf(stdout,"Buffer : %s \n", buffer);
			strcat(buffer," ");
			printf("CommandeParse : %s \n", commandeParse[i]);
			strcat(buffer,commandeParse[i]);
		}
		commande = malloc(sizeof(buffer));			
		strcpy(commande, buffer);
		parsing();
		dup2(oldout,1);
		close(oldout);
	
		free(buffer);
	}
}

