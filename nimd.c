#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#include "protocol.h"

#define MAX_PENDING 5

typedef struct {
    int fd;
    char name[73];
} Player;

typedef struct Game {
    pid_t pid;
    char p1_name[73];
    char p2_name[73];
    struct Game *next;
} Game;

Game *cur_games = NULL; // linked list of current active games
Player *unmatched_player = NULL;
void run_game(Player p1, Player p2); // forward declare the function to run the game

void print_error(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// check for 22 Already Playing error
int already_playing(char *name) {
    if(unmatched_player != NULL) {
        if(strcmp(unmatched_player->name, name) == 0) {
            return 1;
        }
    }

    Game *cur = cur_games;
    while(cur != NULL) {
        if(strcmp(cur->p1_name, name) == 0 || strcmp(cur->p2_name, name) == 0) {
            return 1;
        }
        cur = cur->next;
    }

    return 0;
}

// append a new game to the list of current games
void add_game(pid_t pid, char *p1_name, char *p2_name) {
    Game *new_game = malloc(sizeof(Game));
    new_game->pid = pid;
    strcpy(new_game->p1_name, p1_name);
    strcpy(new_game->p2_name, p2_name);
    new_game->next = cur_games;
    cur_games = new_game;
}

// when a child process ends, remove the game from the list
void remove_game(pid_t pid) {
    Game *cur = cur_games;
    Game *prev = NULL;

    while(cur != NULL) {
        if(cur->pid == pid) {
            if(prev == NULL) {
                cur_games = cur->next;
            } else {
                prev->next = cur->next;
            }
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

// check if any child processes ended, remove if so
void check_zombies() {
    int status;
    pid_t pid;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_game(pid);
    }
}

// send a message from the server to a client
void send_message(int fd, char *type, char *args) {
    char buf[MAX_LEN + 1];
    char body[MAX_LEN + 1];

    // add the type and arguments to the message
    if(args) {
        snprintf(body, sizeof(body), "%s|%s|", type, args);
    } else {
        snprintf(body, sizeof(body), "%s|", type);
    }

    // add the version and length to the front
    int message_len = strlen(body);
    snprintf(buf, sizeof(buf), "0|%02d|%s", message_len, body);

    write(fd, buf, strlen(buf));
}

// accept OPEN from client and respond with WAIT
int setup_handshake(int fd, char *name_buf) {
    Message m;
    int status = parse_message(fd, &m);

    // client disconnected
    if(status == -2) {
        close(fd);
        return -1;
    } else if(status == -1 || strncmp(m.type, "OPEN", 4) != 0) { // 10 Invalid
        send_message(fd, "FAIL", "10 Invalid");
        close(fd);
        return -1;
    }

    // check that the name is present and valid
    if(m.field_count != 1) {
        send_message(fd, "FAIL", "10 Invalid");
        close(fd);
        return -1;
    }

    if(strlen(m.fields[0]) > 72) {
        send_message(fd, "FAIL", "21 Long Name");
        close(fd);
        return -1;
    }

    // check if the player's name matches one that's already playing
    if(already_playing(m.fields[0])) {
        send_message(fd, "FAIL", "22 Already Playing");
        close(fd);
        return -1;
    }

    char *player_name = m.fields[0];
    send_message(fd, "WAIT", NULL);
    strncpy(name_buf, player_name, 72);
    name_buf[72] = '\0';
    return 0;
}

int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "Error: no port number specified\n");
        return EXIT_FAILURE;
    }

    char *port = argv[1];
    int status;
    int server_fd;
    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // for IPv4
    hints.ai_socktype = SOCK_STREAM; // for TCP stream
    hints.ai_flags = AI_PASSIVE; // listening locally

    // get the address info
    if((status = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(status));
        return EXIT_FAILURE;
    }

    // create socket
    server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(server_fd == -1) {
        print_error("socket");
    }

    // make the port reusable again
    int on = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    // bind socket to port
    if(bind(server_fd, res->ai_addr, res->ai_addrlen) == -1) {
        close(server_fd);
        print_error("bind");
    }

    freeaddrinfo(res);

    if(listen(server_fd, MAX_PENDING) == -1) {
        print_error("listen");
    }

    // loop to accept connections
    while(1) {
        check_zombies();

        struct sockaddr_in remote;
        socklen_t remote_len = sizeof(remote);

        int new_fd = accept(server_fd, (struct sockaddr *) &remote, &remote_len);
        if(new_fd < 0) {
            if(errno == EINTR) continue;
            perror("accept");
            continue;
        }

        // check if any child processes ended to update the list of current games
        check_zombies();

        printf("Incoming connection accepted: %d\n", new_fd);

        char player_name[73];
        if(setup_handshake(new_fd, player_name) < 0) {
            printf("Handshake failed: %d\n", new_fd);
            continue;
        }

        // no player waiting, so make this client wait
        if(unmatched_player == NULL) {
            unmatched_player = malloc(sizeof(Player));
            unmatched_player->fd = new_fd;
            strcpy(unmatched_player->name, player_name);
        } else {
            // a player is already waiting, so a game can start now
            pid_t pid = fork();
            if(pid == 0) {
                close(server_fd);

                Player p1 = *unmatched_player;
                Player p2 = {new_fd};
                strcpy(p2.name, player_name);
                free(unmatched_player);
                
                run_game(p1, p2);
                exit(0); // end the child process once the game ends
            } else if(pid > 0) {
                // close the clients in the parent process (theyre running in child process now)
                add_game(pid, unmatched_player->name, player_name);
                close(unmatched_player->fd);
                close(new_fd);
                free(unmatched_player);
                unmatched_player = NULL;
            } else {
                perror("fork");
                close(new_fd);
            }
        }
    }

    return EXIT_SUCCESS;
}