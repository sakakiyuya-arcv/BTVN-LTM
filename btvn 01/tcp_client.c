#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        exit(1);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket failed\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    // if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    //     printf("Địa chỉ IP không hợp lệ\n");
    //     close(client_socket);
    //     exit(1);
    // }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed\n");
        close(client_socket);
        exit(1);
    }

    printf("Connected to server\n");
    char greeting[1024];
    memset(greeting, 0, sizeof(greeting)); 

    int bytes_received = recv(client_socket, greeting, sizeof(greeting) - 1, 0);

    if (bytes_received > 0) {
        greeting[bytes_received] = '\0'; 
        printf(" %s", greeting);
    } 
    
    while (1) {
        printf("> ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;

        // buffer[strcspn(buffer, "\n")] = 0;

        if (strncmp(buffer, "exit", 4) == 0) break;

        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("send failed\n");
            break;
        }
    }

    close(client_socket);

    return 0;
}