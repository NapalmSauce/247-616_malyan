//main:
// 2018-10-11, Yves Roy, creation (247-637 S-0003)

//INCLUSIONS
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <wait.h>
#include <unistd.h>
#include "main.h"
#include "piloteSerieUSB.h"
#include "interfaceTouche.h"
#include "interfaceMalyan.h"

//Definitions privees
#define MAIN_LONGUEUR_MAXIMALE 99

#define PIPE_READ  0
#define PIPE_WRITE 1

//Declarations de fonctions privees:
int main_initialise(void);
void main_termine(void);

//Definitions de variables privees:
//pas de variables privees

//Definitions de fonctions privees:
int main_initialise(void)
{
  if (piloteSerieUSB_initialise() != 0)
  {
    return -1;
  }
  if (interfaceTouche_initialise() != 0)
  {
    return -1;
  }
  if (interfaceMalyan_initialise() != 0)
  {
    return -1;
  }
  return 0;
}

void main_termine(void)
{
  piloteSerieUSB_termine();
  interfaceTouche_termine();
  interfaceMalyan_termine();
}

//Definitions de variables publiques:
//pas de variables publiques


//Definitions de fonctions publiques:
int main(int argc,char** argv)
{
  int erreur = 0;
  unsigned char toucheLue='D';
  char reponse[MAIN_LONGUEUR_MAXIMALE+1];
  int nombre;
  
  char msg = 'D';
  
  int ret = EXIT_SUCCESS;
  
  int pipeFD[2];
  
  pid_t pid;
  

  if (main_initialise())
  {
    printf("main_initialise: erreur\n");
    return 0;
  }

  fprintf(stdout,"Tapez:\n\r");
  fprintf(stdout, "Q\": pour terminer.\n\r");
  fprintf(stdout, "6\": pour démarrer le ventilateur.\n\r");
  fprintf(stdout, "7\": pour arrêter le ventilateur.\n\r");  
  fprintf(stdout, "8\" : donne la position actuelle.\n\r");  
  fprintf(stdout, "P\": va a la position x=20, y=20, z=20\n\r");  
  fprintf(stdout, "H\": positionne la tête d'impression a l'origine\n\r");  
  fprintf(stdout, "autre chose pour générer une erreur.\n\r");
  fflush(stdout);
  
  if(pipe(pipeFD) != 0)
  { 
    main_termine();
    return EXIT_FAILURE;
  }
  
  pid = fork();
  
  if(pid > 0) // Parent
  {
    while (toucheLue != 'Q')
    {
      printf("Entrez une commande\n");
      toucheLue = interfaceTouche_lit();
 //     printf("Caractère lu = '%c'\n", toucheLue);
 
      write(pipeFD[PIPE_WRITE], &toucheLue, 1);
      
      read(pipeFD[PIPE_READ], &msg, 1);
      
      if(!msg) //Fatal
      {
        break;
      }
    }
    wait(NULL);
  }
  else if(pid == 0) //enfant
  {
    while (toucheLue != 'Q')
    {
      printf("Entrez une commande\n");
      toucheLue = read(pipeFD[PIPE_READ], &toucheLue, 1);
      
      printf("Caractère lu = '%c'\n", toucheLue);
      switch (toucheLue)
      {
      case '6':
          if (interfaceMalyan_demarreLeVentilateur() < 0)
          {
            erreur = 1;
          }
        break;
      case '7':
          if (interfaceMalyan_arreteLeVentilateur() < 0)
          {
            erreur = 1;
          }
        break;
      case '8':
          if (interfaceMalyan_donneLaPosition < 0)
          {
            erreur = 1;
          }
        break;
      case 'P':
          if (interfaceMalyan_vaALaPosition(20 ,20 ,20) < 0)
          {
            erreur = 1;
          }
        break;
      case 'H':
          if (interfaceMalyan_retourneALaMaison() < 0)
          {
            erreur = 1;
          }
        break;
        
      case 'Q':
        break;
      default:
          if (interfaceMalyan_genereUneErreur() < 0)
          {
            erreur = 1;
          }
      }
      if (erreur == 1)
      {
        printf("erreur lors de la gestion de la commande\n");
        write(pipeFD[PIPE_WRITE],"\x01",1);
        break;
      }
      else
      {
        usleep(100000);                
        nombre = interfaceMalyan_recoitUneReponse(reponse, MAIN_LONGUEUR_MAXIMALE);
        if (nombre < 0)
        {
          erreur = errno;
          printf("main: erreur lors de la lecture: %d\n", erreur);
          perror("erreur: ");
        }
        else
        {
          reponse[nombre] = '\0';
          printf("nombre reçu: %d, réponse: %s", nombre, reponse);      
  //      fflush(stdout);
  
          if(strstr(reponse,"ok"))
          {
            printf("Pas d'erreurs cote imprimante.\n\r");
          }
          else //Pas de "ok"
          {
            printf("Une erreur est survenue du cote de l'imprimante.\n\r");
          }
        }
        write(pipeFD[PIPE_WRITE],"\x00",1);
      }
    }
  }
  else // Echec de creation du processus enfant
  { 
    ret = EXIT_FAILURE;
  }

  close(pipeFD[PIPE_READ]);
  close(pipeFD[PIPE_WRITE]);
  
  main_termine();
  return ret;
}
