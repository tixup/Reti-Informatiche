#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include "constants.h"
#include <stdbool.h>

/* Struttura per rappresentare la Leaderboard */
typedef struct {
    char nickname[MAX_NICKNAME_LEN];
    int client_sock;                // Socket associato al client
    int score_tech;                 // Punteggio per tecnologia
    int score_general;              // Punteggio per cultura generale
    int current_question;           // Indice della domanda corrente
    int theme_index;                // Tema scelto (0 = tecnologia, 1 = cultura generale)
    bool theme_tech_completed;      // 'true' se completato
    bool theme_gen_completed;       // 'true' se completato
} LeaderboardEntry;

#endif
