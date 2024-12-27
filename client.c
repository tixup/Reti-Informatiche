#include "constants.h"
#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>


int main(int argc, char* argv[]) {
    
    /* Strutture dati */
    int sock, ret;
    int control;
    bool nck;
    bool connected;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char formatted_response[BUFFER_SIZE];

    /* Creazione del socket del client */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    /* Creazione indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); // Porta del server
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    /* Resetto variabile nck e connected */
    nck = false;
    connected = false;
    
    while(1){
    
      /* Mostra il menu di benvenuto al client */
      snprintf(buffer, BUFFER_SIZE, 
              "------ Trivia Quiz ------\n"
              "+++++++++++++++++++++++++\n"
              "Vuoi partecipare a un quiz o uscire?\n"
              "1 - Partecipare\n"
              "2 - Uscire\n"
              "+++++++++++++++++++++++++\n");
      printf("%s", buffer);
      
      /* 1. Scegliere se partecipare o uscire */
      fgets(buffer, BUFFER_SIZE, stdin);
      buffer[strcspn(buffer, "\n")] = '\0';

      // Se l'utente sceglie di uscire, terminare
      if (strncmp(buffer, "2", 1) == 0) {
          memset(buffer, 0, BUFFER_SIZE); // Pulisco il buffer
          printf("Hai scelto di uscire dal Trivia Quiz.\n");
          close(sock);
          return 0;
      }
      
      // Se l'utente sceglie di partecipare
      if(strncmp(buffer, "1", 1) == 0){
          memset(buffer, 0, BUFFER_SIZE);
          // Connessione al server, se non stata fatta in precedenza
          if(!connected){
              ret = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
              connected = true;
              if(ret < 0){
                  perror("Errore in fase di connessione: \n");
                  exit(-1);
              }
          }
          // Invio al server la decisione con token 'START'
          strcpy(buffer, "START_Partecipo\0");
          doppia_send(sock, buffer, strlen(buffer));
      } 
        // Se l'utente sceglie una decisone non valida
        else {
          printf("Hai scelto un opzione non valida.\n");
          close(sock);
          return 0;
      }
      
       
      /* 2. Inserire il nickname */
    
      // Se nickname non presente
      if(!nck){
          // Riceve e stampa il messaggio per il nickname
          control = doppia_recv(sock, buffer); 
          // Controllo che il server sia sempre online
          if(verify_server(buffer) == 0 || control == -2){
              printf("Il Server si è disconnesso. Quiz terminato.\n");
              break;
          }
          printf("%s", buffer); 
          
          // Prende nickname da tastiera
          fgets(response, BUFFER_SIZE, stdin);
          response[strcspn(response, "\n")] = '\0';
          // Formatta la risposta come "NCK_risposta"
          snprintf(formatted_response, BUFFER_SIZE + 4, "NCK_%s", response);
          // Invia il nickname al server
          doppia_send(sock, formatted_response, strlen(formatted_response)); 
          memset(buffer, 0, BUFFER_SIZE);
          
          // Riceve e stampa esito nickname 
          control = doppia_recv(sock, buffer);
          if(verify_server(buffer) == 0 || control == -2){
              printf("Il Server si è disconnesso. Quiz terminato.\n");
              break;
          }
          printf("%s", buffer);
          
          // Se esito non soddisfacente, provo un altro nickname
          while(strstr(buffer, "Riprova")){
              fgets(response, BUFFER_SIZE, stdin);
              response[strcspn(response, "\n")] = '\0';
              // Formatta la risposta come "NCK_risposta"
              snprintf(formatted_response, BUFFER_SIZE + 4, "NCK_%s", response);
              doppia_send(sock, formatted_response, strlen(formatted_response));
              memset(buffer, 0, BUFFER_SIZE);
              control = doppia_recv(sock, buffer); 
              if(verify_server(buffer) == 0 || control == -2){
                  printf("Il Server si è disconnesso. Quiz terminato.\n");
                  break;
              }
              // Stampa esito del nickname
              printf("%s", buffer); 
          }
          
          // Riceve e stampa menù Temi
          control = doppia_recv(sock, buffer); 
          if(verify_server(buffer) == 0 || control == -2){
              printf("Il Server si è disconnesso. Quiz terminato.\n");
              break;
          }
          printf("%s", buffer);
          // Nickname presente nella leaderboard
          nck = true;
          
        } else {
          // Riceve e stampa direttamente menù dei Temi dato che ha già nickname
          control = doppia_recv(sock, buffer);
          if(verify_server(buffer) == 0 || control == -2){
              printf("Il Server si è disconnesso. Quiz terminato.\n");
              break;
          }
          printf("%s", buffer);
        }
      
      /* Pulizia del Buffer */
      memset(buffer, 0, BUFFER_SIZE);
      
      /* 3. Scegliere il tema delle domande */
      do{
          // Prende scelta menù da tastiera
          printf("\nTema n°: ");
          fgets(buffer, BUFFER_SIZE, stdin);
          buffer[strcspn(buffer, "\n")] = '\0';
          
          // Se la scelta è Tecnologia
          if(strncmp(buffer, "1", 1) == 0){
              memset(buffer, 0, BUFFER_SIZE);
              strcpy(buffer, "TEMA_Tech\0");
              // Invia scelta al server
              doppia_send(sock, buffer, strlen(buffer));
              printf("\n-- Hai scelto il tema 'Tecnologia' --\n");
          } 
          // Se la scelta è Generale
          else if(strncmp(buffer, "2", 1) == 0){
              memset(buffer, 0, BUFFER_SIZE);
              strcpy(buffer, "TEMA_Gen\0");
              // Invia scelta al server
              doppia_send(sock, buffer, strlen(buffer));
              printf("\n-- Hai scelto il tema 'Generale' --\n");
          } 
          // Se la scelta non è valida
          else {
              close(sock);
              return 0;
          }
      
          // Esito Tema
          memset(buffer, 0, BUFFER_SIZE);
          // Ricezione esito del tema
          control = doppia_recv(sock, buffer);
          if(verify_server(buffer) == 0 || control == -2){
              printf("Il Server si è disconnesso. Quiz terminato.\n");
              break;
          }
          // Se l'esito è positivo, esco dal do-while
          if(strstr(buffer, "confermata")){
            break;
          }
          // Se l'esito è negativo stampa messaggio di errore del server
          printf("%s", buffer); 
      }while(strstr(buffer, "completato"));
      
      /* Pulizia del Buffer */
      memset(buffer, 0, BUFFER_SIZE); 
      
      /* 4. Rispondere alle domande */
      printf("------------- Domande -------------\n\n");
      while (1) {
          memset(buffer, 0, BUFFER_SIZE);
          
          // Riceve una domanda o messaggio dal server
          control = doppia_recv(sock, buffer); 
          if(verify_server(buffer) == 0 || control == -2){
              printf("Il Server si è disconnesso. Quiz terminato.\n");
              break;
          }
          
          // Controlla se il messaggio indica la fine del quiz dopo le domande
          if (strncmp(buffer, "Hai totalizzato", 15) == 0) {
              printf("--> Quiz terminato. %s\n", buffer);
              break; // Fine del quiz
          }
          
          // Controlla se il messaggio indica la fine del quiz con la endquiz
          if (strncmp(buffer, "Grazie", 6) == 0) {
              nck = false;
              printf("--> Quiz terminato. %s\n", buffer);
              break; // Fine del quiz
          }

          printf("%s", buffer); // Mostra il messaggio ricevuto

          fgets(response, BUFFER_SIZE, stdin);
          response[strcspn(response, "\n")] = '\0';
          
          // Formatta la risposta come "ANSWER_risposta"
          snprintf(formatted_response, BUFFER_SIZE + 7, "ANSWER_%s", response);
          // Invia la risposta al server
          doppia_send(sock, formatted_response, strlen(formatted_response)); 
          
          /* Caso particolare: se client risponde 'endquiz' ad una domanda */
          // (senza aspettare il menù comandi)
          if (strncmp(formatted_response, "ANSWER_endquiz", 14) == 0) {
              // Elimino nickname
              nck = false;
              memset(buffer, 0, BUFFER_SIZE);
              control = doppia_recv(sock, buffer);
              if(verify_server(buffer) == 0 || control == -2){
                  printf("Il Server si è disconnesso. Quiz terminato.\n");
                  break;
              }
              printf("%s", buffer);
              printf("--> Quiz terminato. [hai risposto 'endquiz' ad una domanda]\n\n");
              break; // Fine del quiz
          }
          
          memset(buffer, 0, BUFFER_SIZE);
          
          // Riceve e mostra esito della risposta e il menù comandi
          control = doppia_recv(sock, buffer); 
          if(verify_server(buffer) == 0 || control == -2){
              printf("Il Server si è disconnesso. Quiz terminato.\n");
              break;
          }
          printf("%s", buffer);
          
          // Preleva un comando da tastiera
          fgets(response, BUFFER_SIZE, stdin);
          response[strcspn(response, "\n")] = '\0';
          
          // Formatta il comando come "ANSWER_comando"
          snprintf(formatted_response, BUFFER_SIZE + 7, "ANSWER_%s", response);
          // Invia comando al server
          doppia_send(sock, formatted_response, strlen(formatted_response)); 
        
        }
    
    }

    /* Chiude il socket e termina */
    close(sock);
    return 0;
}
