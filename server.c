#include "constants.h"
#include "utility.h"
#include "questions.h"
#include "leaderboard.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_PARTICIPANTS MAX_CLIENTS

/* Array della Leaderboard */
LeaderboardEntry leaderboard[MAX_PARTICIPANTS];
/* Numero dei partecipanti attivi */
int num_participants = 0;


int main(int argc, char* argv[]) {
    
    /* Strutture dati */
    int listener, fd_max, new_sock, i, client_len;
    int control;
    char *nickname;
    char *client_answer;    
    struct sockaddr_in server_addr, client_addr;
    char *msg;
    char participants_list[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];           

    /* Set principale, Set di lettura */
    fd_set master_set, read_set;        
    

    /* Caricamento delle domande */
    QuizQuestion tech_questions[5], general_questions[5];
    int num_tech = 0, num_general = 0;

    load_questions("tech_questions.txt", tech_questions, &num_tech);
    load_questions("general_questions.txt", general_questions, &num_general);

    /* Configurazione del socket listener */
    listener = socket(AF_INET, SOCK_STREAM, 0);
    
    /* Creazione indirizzo di bind */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* Aggancio del socket */
    if (bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nella bind");
        exit(1);
    }
    
    listen(listener, MAX_CLIENTS);

    /* Azzero i set */
    FD_ZERO(&master_set);
    FD_ZERO(&read_set);
    
    /* Aggiungo il socket di ascolto al set master */
    FD_SET(listener, &master_set);
    fd_max = listener;

    printf("Server Trivia Quiz in ascolto sulla porta %d\n\n", PORT);
    
    /* Visualizzo all'avvio la lista dei Temi */
    snprintf(buffer, BUFFER_SIZE, 
            "------ Trivia Quiz -----\n"
            "++++++++++++++++++++++++\n"
            "Temi:\n"
            "1 - Curiosità sulla Tecnologia\n"
            "2 - Cultura Generale\n"
            "++++++++++++++++++++++++\n");
    printf("%s", buffer);
    
    /* Visualizzo partecipanti Online */
    memset(participants_list, 0, BUFFER_SIZE);
    generate_participants_list(participants_list, leaderboard, num_participants);
    printf("%s\n", participants_list);

    while (1) {
        /* Imposto il set di socket da monitorare in lettura per la select() 
           select() non modifica il set 'master' dei descrittori monitorati  */
        read_set = master_set;

        /* Mi blocco (potenzialmente) in attesa di descrittori pronti */
        if (select(fd_max + 1, &read_set, NULL, NULL, NULL) == -1) {
            perror("Errore con select");
            msg = "Il server si è disconnesso. Il quiz è terminato.\n";
            // Notifico i client della chiusura del server
            notify_and_close(&master_set, fd_max, listener, msg);
            exit(1);
        }
        
        /* Scorro ogni descrittore */
        for (i = 0; i <= fd_max; i++) {
        
            /* Se il descrittore 'i' è rimasto nel set 'read_set', cioè se la select  
               ce lo ha lasciato, allora 'i' è pronto */
               
            if (FD_ISSET(i, &read_set)) {
            
                /* Se il descrittore 'i' è il listening socket  
                   ho ricevuto una richiesta di connessione     */
                   
                if (i == listener) {
                    // Calcolo lunghezza dell'indirizzo del client
                    client_len = sizeof(client_addr);
                    // accetto connessione 
                    new_sock = accept(listener, (struct sockaddr*)&client_addr, (socklen_t*)&client_len);
                    if (new_sock == -1) {
                        perror("Errore accettando la connessione");
                        continue;
                    }
                    /* Aggiunge il nuovo socket al set */
                    FD_SET(new_sock, &master_set);
                    if (new_sock > fd_max) fd_max = new_sock;

                    printf("Stabilita la connessione con il sock: %d.\n", new_sock);
                    
                } else {
                    /* Gestione di un client esistente */
                    
                    memset(buffer, 0, BUFFER_SIZE);
            
                    control = doppia_recv(i, buffer);
                    if (control == -2) {
                        // Disconnessione del client
                        printf("Il client socket %d si è disconnesso.\n", i);
                        // Rimuove il partecipante dalla leaderboard
                        if (remove_participant_by_socket(i, leaderboard, &num_participants)) {
                                    printf("Partecipante rimosso con successo.\n");
                                }
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    
                    printf("Messaggio ricevuto dal client socket %d: %s\n", i, buffer);

                    /* Gestiamo i casi possibili, a seconda del 'tipo' di messaggio
                       che abbiamo ricevuto dal client: */
                    
                    /* Menù iniziale */
                    if(strstr(buffer, "START")){
                          // Client vuole partecipare
                        if(strstr(buffer, "Partecipo")){
                                // Richiedo un nickname valido da essere inserito
                                // solamente se non è già presente nella leaderboard
                                LeaderboardEntry *participant = find_participant_by_socket(i, leaderboard, num_participants);
                                if(!participant){
                                    strcpy(buffer, "Inserisci il tuo nickname:\n\0");
                                    doppia_send(i, buffer, strlen(buffer));
                                } else {
                                    // Creo e invio menù dei temi al partecipante
                                    snprintf(buffer, BUFFER_SIZE, 
                                        "++++++++++++++++++++++++\n"
                                        "Temi:\n"
                                        "1 - Curiosità sulla Tecnologia\n"
                                        "2 - Cultura Generale\n"
                                        "++++++++++++++++++++++++\n");
                                    doppia_send(i, buffer, strlen(buffer));

                                    /* Genera e mostra la lista dei partecipanti attivi nel server */
                                    
                                    /* Visualizzo partecipanti Online */
                                    memset(participants_list, 0, BUFFER_SIZE);
                                    generate_participants_list(participants_list, leaderboard, num_participants);
                                    printf("%s\n", participants_list);
                                    }
                                
                            } 
                    }
                    
                    
                    /* Client ha mandato il suo nickname */
                    if(strstr(buffer, "NCK")){
                        // Sposta il puntatore dopo "NCK_"
                        nickname = buffer + 4; 
                        printf("Nickname ricevuto: %s\n", nickname);
                        // Controllo nickname
                        if(!request_nickname(i, nickname, leaderboard, &num_participants)){
                            printf("Errore nickname\n");
                            break;
                        } else {
                        
                        strcpy(buffer, "Nickname accettato! Benvenuto nel Trivia Quiz!\n");
                        doppia_send(i, buffer, strlen(buffer));
                        
                        /* Se tutto corretto, mando la scelta dei temi */

                        // Creo e invio menù dei temi
                        snprintf(buffer, BUFFER_SIZE, 
                            "++++++++++++++++++++++++\n"
                            "Temi:\n"
                            "1 - Curiosità sulla Tecnologia\n"
                            "2 - Cultura Generale\n"
                            "++++++++++++++++++++++++\n");
                        doppia_send(i, buffer, strlen(buffer));

                        /* Visualizzo partecipanti Online */
                        memset(participants_list, 0, BUFFER_SIZE);
                        generate_participants_list(participants_list, leaderboard, num_participants);
                        printf("%s\n", participants_list);                        
                        }
                    }
                    
                    /* Client ha scelto il Tema */
                    if(strstr(buffer, "TEMA")){
                        // Cerco il partecipante dato il socket
                        LeaderboardEntry *participant = find_participant_by_socket(i, leaderboard, num_participants);
                        if (!participant) {
                            printf("Errore: partecipante non trovato per socket %d.\n", i);
                            strcpy(buffer, "Errore: partecipante non trovato.\n\0");
                            doppia_send(i, buffer, strlen(buffer));
                            close(i);
                            FD_CLR(i, &master_set);
                            continue;
                        }
                        
                        /* Se il tema scelto è Tecnologia */
                        if(strstr(buffer, "Tech")){
                                // Controllo se non ha già completato questo tema
                                if(participant->theme_tech_completed){
                                    strcpy(buffer, "Hai già completato questa tema. Prova con un altro.\n");
                                    doppia_send(i, buffer, strlen(buffer));
                                    break;
                                }
                                
                                participant->theme_index = 0;
                                
                                strcpy(buffer, "Scelta Tema confermata\n");
                                doppia_send(i, buffer, strlen(buffer));
                                printf("Il client %s ha scelto il tema Tecnologia.\n", participant->nickname);

                                // Procediamo con la prima domanda
                                handle_quiz(i, participant, tech_questions);
                                
                            } 
                        /* Se il tema scelto è Generale */
                        else if(strstr(buffer, "Gen")){
                                // Controllo se non ha già completato questo tema
                                if(participant->theme_gen_completed){
                                    strcpy(buffer, "Hai già completato questa tema. Prova con un altro.\n");
                                    doppia_send(i, buffer, strlen(buffer));
                                    break;
                                }
                            
                                participant->theme_index = 1;
                                
                                strcpy(buffer, "Scelta Tema confermata\n");
                                doppia_send(i, buffer, strlen(buffer));
                                printf("Il client %s ha scelto il tema Generale.\n", participant->nickname);

                                // Procediamo con la prima domanda
                                handle_quiz(i, participant, general_questions);                            
                            }
                            
                        /* Se il comando è errato */
                        else {
                                printf("Il client socket %d si è disconnesso.\n", i);
                                strcpy(buffer, "Comando errato, si prega di riconnettersi \n");
                                doppia_send(i, buffer, strlen(buffer));
                                // Rimuove il partecipante dalla leaderboard
                                if (remove_participant_by_socket(i, leaderboard, &num_participants)) {
                                    printf("Partecipante rimosso con successo.\n");
                                }
                                close(i);
                                FD_CLR(i, &master_set);
                            }
                    }
                    
                    /* Client risponde alle domande del Quiz*/
                     if(strstr(buffer, "ANSWER")){
                        // Estrai la risposta dal buffer, spostando il puntatore dopo "ANSWER_"
                        client_answer = buffer + 7;
                  
                        // Trova il partecipante in base al socket
                        LeaderboardEntry *participant = find_participant_by_socket(i, leaderboard, num_participants);
                        if (participant) {
                        printf("Ho trovato il partecipante %d\n", i);
                            
                            /* Client vuole procedere alla prossima domanda */
                            if(strstr(client_answer, "next")){
                                memset(buffer, 0, BUFFER_SIZE);
                                handle_quiz(i, participant, participant->theme_index == 0 ? tech_questions : general_questions);
                                // Controllo se ha completato il quiz, in caso lo mostro a video
                                completed_quiz(leaderboard, num_participants);
                            }
                            
                            /* Client vuole vedere la Leaderboard */
                            else if(strstr(client_answer, "showscore")){
                                printf("Il client %s ha richiesto la leaderboard.\n", participant->nickname);

                                // Genera la leaderboard per il tema corrente
                                memset(buffer, 0, BUFFER_SIZE);
                                display_sorted_leaderboard(buffer, leaderboard, num_participants, participant->theme_index);

                                // Invia la leaderboard al client
                                doppia_send(i, buffer, strlen(buffer));
                            }
                            
                            /* Client vuole uscire */
                            else if(strstr(client_answer, "endquiz")){
                                // Rimuove riga del partecipante dalla leaderboard
                                if (remove_participant_by_socket(i, leaderboard, &num_participants)) {
                                    printf("Partecipante rimosso con successo.\n");
                                }
                                strcpy(buffer, "Grazie per aver partecipato!\n");
                                doppia_send(i, buffer, strlen(buffer));
                            }
                            
                            /* Client ha risposto alla domanda */
                            else {
                                // Valuta la risposta e manda l'esito al client
                                evaluate_response(i, participant, participant->theme_index == 0 ? tech_questions : general_questions, client_answer);
                            }
                            
                            /* Client non trovato */
                        } else {
                            printf("Errore: partecipante non trovato per socket %d.\n", i);
                            close(i);
                            FD_CLR(i, &master_set);
                        }
                     } 
                       
                    }
                }
            }
        }

    close(listener);
    return 0;
}
