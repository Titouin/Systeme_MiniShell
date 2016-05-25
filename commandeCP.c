#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h> // EAGAIN - EINTR
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h> // openDir

/* Copie le contenue d'un repertoire source dans un répertoire destination en créeant les dossiers avec les bon droit et réutilisant copieFichier. */
void gestionRepertoire (char * nomRepertoireSource, char * nomRepertoireCible);

/* Copie un fichier source vers un fichier destination en lui donnant les bon droits. */
void copieFichier(char * nomFichierSource, char * nomFichierCible, struct stat fichierSourceStat);

int main ( int argc, char ** argv )
{
	/* Test si le programme est lancé avec le bon nombre d'argument */
	if( argc != 3 ){
		write(2, "ERREUR : nbArgument. \n", strlen("ERREUR : nbArgument. \n"));
		exit(2);
	}
 
	/* Création de la strucutre source stat */
	/* Récupération des données du fichierSource/dossierSource */
	struct stat fichierSourceStat;
	if(stat(argv[1],&fichierSourceStat) < 0)    
        	return 1;

	/* Test si le fichierSource est un répertoire */
	if(S_ISDIR(fichierSourceStat.st_mode))
	{	
		/* Répertoire */
		/* Test si le second parametres est bien un repertoire cible */
		struct stat fichierCibleStat;
		if(stat(argv[2],&fichierCibleStat) < 0)    
			return 1;
		if(!S_ISDIR(fichierCibleStat.st_mode)) {
			write(2, "ERREUR : Le second argument n'est pas un répertoire. \n", strlen("ERREUR : Le second argument n'est pas un répertoire. \n"));
			exit(4);
		}		

		gestionRepertoire(argv[1],argv[2]);
	} else {
		// Fichier
		copieFichier(argv[1], argv[2], fichierSourceStat);
	}

	return 0;		
}

void gestionRepertoire (char * nomRepertoireSource, char * nomRepertoireCible)
{
		// Garde en mémoire le chemin 
		char pathSource[256];
		char pathCible[256];
		
		// Création de la structure 
		struct dirent *fichierLu;
		DIR *repertoire;
		// Ouverture du répertoire
		repertoire = opendir(nomRepertoireSource);
		//Test si le fichier a correctement été ouvert
		if (repertoire == NULL) { 
			write(2, "ERREUR : Problème ouverture répertoire. \n", strlen("ERREUR : Problème ouverture répertoire. \n"));
			exit(9);
		}


		// Boucle sur tous les fichiers du répertoire
		while ((fichierLu = readdir(repertoire))) {
			// On ne fais pas les dossier . et .. qui sont déja crée dans un dossir existant
			if (strcmp(fichierLu->d_name, ".") != 0 && strcmp(fichierLu->d_name, "..") != 0) {
				// Enregistre l'arborescence
				strcpy(pathSource, nomRepertoireSource);
				strcpy(pathCible, nomRepertoireCible);
				// Ajoute un / 
				strcat(pathSource, "/");
				strcat(pathCible, "/");
				// Ajoute le nom du fichier lu 
				strcat(pathSource, fichierLu->d_name);
				strcat(pathCible,fichierLu->d_name);

				// Récupere les infos du fichier source
				struct stat fichierSourceStat;
				if(stat(pathSource, &fichierSourceStat) < 0) {
					write(2, "ERREUR : Problème récupération stat. \n", strlen("ERREUR : Problème récupération stat. \n"));
					exit(11);
				}

				// Test si le fichier est un répertoire
				if(S_ISDIR(fichierSourceStat.st_mode))
				{	
					// Créer le répertoire dans le dossier cible
					mkdir(pathCible, fichierSourceStat.st_mode);
					gestionRepertoire(pathSource, pathCible);
	 			} else
				{
					copieFichier(pathSource, pathCible, fichierSourceStat); 
				}
			}
		}

		// Fermeture du répertoire
		if (closedir(repertoire) == -1)
		{
			write(2, "ERREUR : Echec fermeture répertoire. \n", strlen("ERREUR : Echec fermeture répertoire. \n"));
			exit(-1);
		}
}

void copieFichier(char * nomFichierSource, char * nomFichierCible, struct stat fichierSourceStat)
{
		

	// Ouvre le fichier source 
	int descFichierSource = open( nomFichierSource, O_RDONLY,  0660);
	// Test si le fichier c'est correctement ouvert
	if( descFichierSource < 0 )
	{
		write(2, "ERREUR: Ouverture fichier source.\n", strlen("ERREUR: Ouverture fichier source.\n"));
		exit(2);
	}
	
	// Ouvre le fichier cible 
	int descFichierCible = open( nomFichierCible, O_WRONLY | O_CREAT, 0660);
	// Test si le fichier c'est correctement ouvert
	if( descFichierCible < 0 )
	{
		write(2, "ERREUR: Ouverture fichier cible.\n", strlen("ERREUR: Ouverture fichier cible.\n"));
		exit(3);
	}

	// Création du buffer 
	char c[fichierSourceStat.st_size];
	int nbLus, nbEcrit;

	// Lit tant que plus d'erreur
	do {
    		nbLus = read( descFichierSource, &c , fichierSourceStat.st_size );
	} while ( nbLus == EAGAIN || nbLus == EINTR );
	
	// Ecrit les caracteres dans le fichier
	do {
		nbEcrit = write( descFichierCible , &c , fichierSourceStat.st_size );;
	} while ( nbEcrit == EAGAIN || nbLus == EINTR );

	// Donne les meme droits du fichier source au fichier cible		
	fchmod(descFichierCible, fichierSourceStat.st_mode);	

	// Ferme les fichiers ouverts
	close(descFichierCible);
	close(descFichierSource);
}
