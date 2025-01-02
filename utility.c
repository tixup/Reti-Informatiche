#include "utility.h"
#include "leaderboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

/* Funzioni di utilità per Server e Client */


/**
 * Gestisce la receive.
 * 
 * @param socket Socket del client o server.
 * @param buffer Array di recezione.
 * @return -1 Errore nella recv
 * @return -2 Ricezione di close();
 * @return 0 Lunghezza zero
 * @return len Lunghezza del messaggio
 */
int doppia_recv(int socket, char *buffer){               
    int ret, len;
    // Ricevi lunghezza
    ret = recv(socket, &len, sizeof(int), 0);
    if(ret == -1){
        perror("len doppia_recv. ");
        return -1;
    }
    // Se torna 0, allora è una close
    if(!ret){                                     
        printf("Ricevuta close()\n");
        return -2;                                  
    }
    
    // Conversione Network to Host
    len = ntohl(len);                               

    if(!len) return 0;                              
    ret = recv(socket, buffer, len, 0);                   
    if(ret == -1){
        perror("buf doppia_recv. ");
        return -1;
    }
    // Inseriamo il carattere '\0' alla fine del buffer appena ricevuto
    memset(buffer + ret, 0, 1);                       
    return len;
}


/**
 * Gestisce la send.
 * 
 * @param socket Socket del client o server.
 * @param buffer Array del messaggio.
 * @param len lunghezza messaggio
 * @return -1 Errore sulla send
 * @return ret 
 */
int doppia_send(int socket, char *buffer, int len){
    int ret;
    // Lunghezza in big endian
    ret = htonl(len);
    // Inviamo la lunghezza
    ret = send(socket, &ret, sizeof(int), 0);           
    if(ret == -1){
        perror("len doppia_send. ");
        return -1;
    }
    // Se abbiamo inviato len = 0 allora il messaggio è vuoto
    if(!ntohl(len)) return 0;   
    // Invio del messaggio
    ret = send(socket, buffer, len, 0);                   
    if(ret == -1){
        perror("buf doppia_send. ");
        return -1;
    }
    return ret;
}

/**
 * Richiede al client di inserire un nickname univoco.
 * 
 * @param client_sock Socket del client.
 * @param leaderboard Array della leaderboard.
 * @param num_participants Puntatore al numero di partecipanti attivi.
 * @return 1 se il nickname è valido, 0 altrimenti.
 */
int request_nickname(int client_sock, char *nickname, LeaderboardEntry leaderboard[], int *num_participants) {
    char buffer[BUFFER_SIZE];
    strncpy(buffer, nickname, MAX_NICKNAME_LEN);
    
    // Controlla che il nickname sia unico
    for (int i = 0; i < *num_participants; i++) {
        if (strncmp(leaderboard[i].nickname, buffer, MAX_NICKNAME_LEN) == 0) {
            strcpy(buffer, "Nickname già in uso. Riprova con un altro.\n");
            doppia_send(client_sock, buffer, strlen(buffer));
            return 0;
        }
    }

    // Aggiunge il nuovo partecipante alla leaderboard
    strncpy(leaderboard[*num_participants].nickname, buffer, MAX_NICKNAME_LEN);
    leaderboard[*num_participants].score_tech = 0;
    leaderboard[*num_participants].score_general = 0;
    leaderboard[*num_participants].current_question = 0;
    leaderboard[*num_participants].theme_index = -1;
    leaderboard[*num_participants].client_sock = client_sock;
    leaderboard[*num_participants].theme_tech_completed = false;
    leaderboard[*num_participants].theme_gen_completed = false;
    (*num_participants)++;
    
    // Debug: stampa il nickname appena salvato
    printf("Partecipante aggiunto: %s\n", leaderboard[*num_participants - 1].nickname);
    
    return 1;
}


/**
 * Trova un partecipante nella leaderboard.
 * 
 * @param nickname Nickname del partecipante.
 * @param leaderboard Array della leaderboard.
 * @param num_participants Numero di partecipanti attivi.
 * @return Puntatore al partecipante, o NULL se non trovato.
 */
LeaderboardEntry* find_participant(const char *nickname, LeaderboardEntry leaderboard[], int num_participants) {
    for (int i = 0; i < num_participants; i++) {
        if (strncmp(leaderboard[i].nickname, nickname, MAX_NICKNAME_LEN) == 0) {
            return &leaderboard[i];
        }
    }
    return NULL;
}

/**
 * Gestisce la domanda del quiz per un client specifico.
 * 
 * @param client_sock Socket del client.
 * @param questions Array delle domande.
 * @param particpant Utente della leaderboard.
 */
void handle_quiz(int client_sock, LeaderboardEntry *participant, QuizQuestion *questions) {
    char buffer[BUFFER_SIZE];
    int question_index = participant->current_question;

    // Se il quiz è terminato
    if (question_index >= 5) {
        snprintf(buffer, BUFFER_SIZE, "Hai totalizzato %d punti!\n",
                 participant->theme_index == 0 ? participant->score_tech : participant->score_general);
        doppia_send(client_sock, buffer, strlen(buffer));
        // Resetta per futuri quiz
        participant->current_question = 0; 
        
        // Smarco il tema del quiz completato
        if(participant->theme_index == 0){
            participant->theme_tech_completed = true;
        } else {
            participant->theme_gen_completed = true;
        }
      
        return;
    }

    // Invia la domanda corrente al client
    snprintf(buffer, BUFFER_SIZE, "Domanda %d: %s\n", question_index + 1, questions[question_index].question);
    doppia_send(client_sock, buffer, strlen(buffer));
    printf("Pongo la domanda %d al client %s\n", question_index + 1, participant->nickname);
    
    // Incrementa per la prossima domanda
    participant->current_question++; 
}

/**
 * Gestisce la risposta del quiz per un client specifico.
 * 
 * @param client_sock Socket del client.
 * @param response Risposta del client
 * @param questions Array delle domande.
 * @param particpant Utente della leaderboard.
 */
void evaluate_response(int client_sock, LeaderboardEntry *participant, QuizQuestion *questions, char *response) {
    char buffer[BUFFER_SIZE];
    strncpy(buffer, response, BUFFER_SIZE);
    
    // Domanda appena posta
    int question_index = participant->current_question - 1; 

    // Valuta la risposta
    if (strncmp(buffer, questions[question_index].answer, strlen(questions[question_index].answer)) == 0) {
          // Manda esito e menù dei comandi al client
          strcpy(buffer, "Risposta corretta!\n-- Comandi: next, showscore, endquiz --\n");
          doppia_send(client_sock, buffer, strlen(buffer));
        // Incrementa il punteggio del tema corrente
        if (participant->theme_index == 0) {
            participant->score_tech++;
        } else {
            participant->score_general++;
        }
    // Manda esito e menù dei comandi al client
    } else {
          strcpy(buffer, "Risposta errata!\n-- Comandi: next, showscore, endquiz --\n");
          doppia_send(client_sock, buffer, strlen(buffer));
    }
}


/**
 * Genera un messaggio contenente la lista di tutti i partecipanti connessi al server
 *
 * @param buffer Array in cui scrivere il messaggio generato.
 * @param leaderboard Array della leaderboard
 * @param num_participants Numero dei partecipanti attivi
 */
void generate_participants_list(char *buffer, LeaderboardEntry leaderboard[], int num_participants){
    // Pulisco il buffer
    memset(buffer, 0, BUFFER_SIZE);
    // Creazione del messaggio
    snprintf(buffer, BUFFER_SIZE, "Partecipanti online (%d):\n", num_participants);
    for(int i = 0; i < num_participants; i++){
        strncat(buffer, "- ", BUFFER_SIZE - strlen(buffer) - 1);
        strncat(buffer, leaderboard[i].nickname, BUFFER_SIZE - strlen(buffer) - 1);
        strncat(buffer, "\n", BUFFER_SIZE - strlen(buffer) - 1);
    }
}


/**
 * Trova i partecipanti nella Leaderboard tramite il socket
 *
 * @param client_sock Socket del client.
 * @param leaderboard Array della leaderboard
 * @param num_participants Numero dei partecipanti attivi
 * @return Puntatore al partecipante, NULL se non trovato
 */
LeaderboardEntry* find_participant_by_socket(int client_sock, LeaderboardEntry leaderboard[], int num_participants) {
    for (int i = 0; i < num_participants; i++) {
        if (leaderboard[i].client_sock == client_sock) {
            return &leaderboard[i];
        }
    }
    return NULL; // Partecipante non trovato
}


/**
 * Rimuove un partecipante dalla leaderboard in base al socket.
 * 
 * @param client_sock Socket del client da rimuovere.
 * @param leaderboard Array della leaderboard.
 * @param num_participants Puntatore al numero di partecipanti attivi.
 * @return 1 se il partecipante è stato rimosso, 0 altrimenti.
 */
int remove_participant_by_socket(int client_sock, LeaderboardEntry leaderboard[], int *num_participants) {
    for (int i = 0; i < *num_participants; i++) {
        if (leaderboard[i].client_sock == client_sock) {
            // Debug: stampa il nickname rimosso
            printf("Rimuovo il partecipante: %s (socket %d)\n", leaderboard[i].nickname, client_sock);

            // Sposta gli elementi successivi indietro di una posizione
            for (int j = i; j < *num_participants - 1; j++) {
                leaderboard[j] = leaderboard[j + 1];
            }
            
            // Decrementa il numero di partecipanti
            (*num_participants)--; 
            return 1;
        }
    }

    printf("Partecipante con socket %d non trovato nella leaderboard.\n", client_sock);
    return 0;
}


/**
 * Mostra la leaderboard ordinata.
 * 
 * @param buffer Puntatore a buffer.
 * @param leaderboard Array della leaderboard.
 * @param num_participants Puntatore al numero di partecipanti attivi.
 * @param theme_index Tema del quiz.
 * @return 1 se il partecipante è stato rimosso, 0 altrimenti.
 */
void display_sorted_leaderboard(char *buffer, LeaderboardEntry leaderboard[], int num_participants, int theme_index) {
    
    // Strutture dati
    int score_i;
    int score_j;
    int score;
    char entry[BUFFER_SIZE];
  
    // Copia i partecipanti in un array temporaneo
    LeaderboardEntry temp_leaderboard[num_participants];
    memcpy(temp_leaderboard, leaderboard, sizeof(LeaderboardEntry) * num_participants);

    // Ordina l'array temporaneo in base al punteggio del tema scelto
    for (int i = 0; i < num_participants - 1; i++) {
        for (int j = i + 1; j < num_participants; j++) {
            score_i = (theme_index == 0) ? temp_leaderboard[i].score_tech : temp_leaderboard[i].score_general;
            score_j = (theme_index == 0) ? temp_leaderboard[j].score_tech : temp_leaderboard[j].score_general;

            if (score_i < score_j) {
                // Scambio
                LeaderboardEntry temp = temp_leaderboard[i];
                temp_leaderboard[i] = temp_leaderboard[j];
                temp_leaderboard[j] = temp;
            }
        }
    }

    // Costruisci il messaggio della leaderboard
    memset(buffer, 0, BUFFER_SIZE);
    if(theme_index == 0){
        snprintf(buffer, BUFFER_SIZE, "Leaderboard tema Tecnologia:\n");
    } else {
        snprintf(buffer, BUFFER_SIZE, "Leaderboard tema Generale:\n");
    }
    

    for (int i = 0; i < num_participants; i++) {
        score = (theme_index == 0) ? temp_leaderboard[i].score_tech : temp_leaderboard[i].score_general;
        snprintf(entry, BUFFER_SIZE, "- %s (%d)\n", temp_leaderboard[i].nickname, score);
        strncat(buffer, entry, BUFFER_SIZE - strlen(buffer) - 1);
    }
    
    strcpy(entry, "Digita 'next' per proseguire o rispondi alla domanda precedente.\n");
    strncat(buffer, entry, BUFFER_SIZE - strlen(buffer) - 1);
}



/**
 * Notifica tutti i client connessi della chiusura del server.
 * 
 * @param fd_master Set principale, gestito dal programmatore.
 * @param fd_max Numero massimo di descrittori.
 * @param listener Socket di ascolto.
 * @param message Messaggio per i client online.
 */
 
 void notify_and_close(fd_set *fd_master, int fd_max, int listener, char *message){
    for(int i = 0; i <= fd_max; i++){
        // ignoro il listener
        if(FD_ISSET(i, fd_master) && i != listener){
            // notifico il client
            doppia_send(i, message, strlen(message));
            // chiudo socket e rimuovo dal set
            close(i);
            FD_CLR(i, fd_master);
        }
    }
 }
 
 
 /**
 * Verifica lo stato del server.
 * 
 * @param buffer Buffer del client.
 * @return 1 se il server è online, 0 altrimenti.
 */
 
 int verify_server(char *buffer){
    if(strncmp(buffer, "Il server si è disconnesso", 26) == 0){
        printf("Server Error Message: %s\n", buffer);
        return 0;
    }
    return 1;
 }
 
 
 /**
 * Mostra client che hanno completato i quiz.
 * 
 * @param leaderboard Array della leaderboard.
 * @param num_participants Puntatore al numero di partecipanti attivi.
 * @param participant Utente della Leaderboard
 */
 void completed_quiz(LeaderboardEntry leaderboard[], int num_participants, LeaderboardEntry *participant){
    
     // Se il client ha completato uno dei due quiz, stampo
    if( (participant->theme_tech_completed == 1 && participant->current_question == 0) || 
        (participant->theme_gen_completed == 1 && participant->current_question == 0) ) {
        
        printf("\n--- Quiz Completati ---\n");
    
        // Partecipanti che hanno completato il tema tecnologia
        printf("Quiz tema 'tecnologia' completato:\n");
        for(int i = 0; i < num_participants; i++){
          if(leaderboard[i].theme_tech_completed){
              printf("- %s\n", leaderboard[i].nickname);
          }
        }
        
        printf("\n");
        
        // Partecipanti che hanno completato il tema generale
        printf("Quiz tema 'generale' completato:\n");
        for(int i = 0; i < num_participants; i++){
          if(leaderboard[i].theme_gen_completed){
              printf("- %s\n", leaderboard[i].nickname);
          }
        }
        
        printf("\n");
    } 
    // Altrimenti non stampo nulla
    else {
        return;
    }
    
    
 }
