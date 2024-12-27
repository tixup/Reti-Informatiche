#ifndef UTILITY_H
#define UTILITY_H

#include "questions.h"
#include "constants.h"
#include "leaderboard.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

int doppia_recv(int socket, char *buffer);

int doppia_send(int socket, char *buffer, int len);

int request_nickname(int client_sock, char* nickname, LeaderboardEntry leaderboard[], int *num_participants);

void handle_quiz(int client_sock, LeaderboardEntry *participant, QuizQuestion* questions);

void generate_participants_list(char *buffer, LeaderboardEntry leaderboard[], int num_participants);

void evaluate_response(int client_sock, LeaderboardEntry *participant, QuizQuestion *questions, char* response);

LeaderboardEntry* find_participant_by_socket(int client_sock, LeaderboardEntry leaderboard[], int num_participants);

int remove_participant_by_socket(int client_sock, LeaderboardEntry leaderboard[], int *num_participants);

void display_sorted_leaderboard(char *buffer, LeaderboardEntry leaderboard[], int num_participants, int theme_index);

void notify_and_close(fd_set *fd_master, int fd_max, int listener, char *message);

void completed_quiz(LeaderboardEntry leaderboard[], int num_participants);

int verify_server(char *buffer);

#endif
