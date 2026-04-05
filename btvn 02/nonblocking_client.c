#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int client_sk = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(8080);

    if(connect(client_sk, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    char buffer[1024];
    int bytes;

    bytes = recv(client_sk, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s", buffer);

        char name[256];
        fgets(name, sizeof(name), stdin);
        name[strcspn(name, "\n")] = '\0';
        send(client_sk, name, strlen(name), 0);
    }

    bytes = recv(client_sk, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s", buffer);

        char mssv[256];
        fgets(mssv, sizeof(mssv), stdin);
        mssv[strcspn(mssv, "\n")] = '\0';
        send(client_sk, mssv, strlen(mssv), 0);
    }

    bytes = recv(client_sk, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Result: %s\n", buffer);
    }

    close(client_sk);
    return 0;
}