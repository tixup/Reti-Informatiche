#ifndef QUESTIONS_H
#define QUESTIONS_H
#include "constants.h"

/* Struttura per rappresentare una domanda e la sua risposta */
typedef struct {
    char question[MAX_QUESTION_LEN];    // Testo della domanda
    char answer[MAX_ANSWER_LEN];        // Testo della risposta corretta
} QuizQuestion;

/* Funzione per caricare domande e risposte da un file */
void load_questions(const char *filename, QuizQuestion *questions, int *num_questions);

#endif