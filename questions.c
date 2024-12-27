#include "questions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Carica le domande e risposte da un file di testo.
 * 
 * @param filename: Nome del file da cui leggere le domande e risposte.
 * @param questions: Array di strutture `QuizQuestion` in cui memorizzare i dati.
 * @param num_questions: Puntatore a un intero per memorizzare il numero di domande caricate.
 */

void load_questions(const char *filename, QuizQuestion *questions, int *num_questions) {
    // Apre il file in modalità lettura
    FILE *file = fopen(filename, "r");
    if (!file) { // Controllo di errore
        perror("Errore nell'apertura del file");
        *num_questions = 0;
        exit(1); // Termina il programma in caso di errore
    }

    int count = 0; // Inizializza il contatore delle domande a zero
    char line[MAX_QUESTION_LEN];

    // Legge il file riga per riga
    while (fgets(line, sizeof(line), file)) {

        // Rimuove il carattere di newline
        line[strcspn(line, "\n")] = '\0';
        // Salva la domanda
        strncpy(questions[count].question, line, MAX_QUESTION_LEN);
        // Legge la risposta
        if (!fgets(line, sizeof(line), file)) {

            printf("Errore: risposta mancante per la domanda %d\n", count + 1);
            break;
        }

        line[strcspn(line, "\n")] = '\0';
        strncpy(questions[count].answer, line, MAX_ANSWER_LEN);

        // Incrementa il contatore delle domande
        count++;

        // Controlla se si è raggiunto il limite massimo
        if (count >= MAX_QUESTIONS) {
            printf("Avviso: raggiunto il limite massimo di domande (%d)\n", MAX_QUESTIONS);
            break;
        }
    }
    *num_questions = count;
    // Chiude il file dopo aver completato la lettura
    fclose(file);
    // Debug
    printf("Totale domande caricate da %s: %d\n", filename, *num_questions);
}


