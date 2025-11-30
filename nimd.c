#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_PENDING 5

void print_error(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
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
        print_error("socket")
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
        error("listen");
    }

    printf("nimd listening on port %s\n", port);

    // loop to accept connections

    return EXIT_SUCCESS;
}