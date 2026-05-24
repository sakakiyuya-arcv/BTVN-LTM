#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

void *thread_proc(void *);

int main() {
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == -1) {
        perror("socket() failed");
        return 1;
    }
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);
    
    if (connect(client, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("connect() failed");
        close(client);
        return 1;
    }
    
    printf("Connected to server on port 8080...\n");
    
    pthread_t id;
    pthread_create(&id, NULL, thread_proc, &client);
    pthread_detach(id);

    char buf[256];
    while (1) {
        fgets(buf, sizeof(buf), stdin);
        send(client, buf, strlen(buf), 0);
        if (strcmp(buf, "exit\n") == 0)
            break;
    }

    close(client);
    return 0;
}

void *thread_proc(void *params) {
    int client = *(int *)params;
    char buf[256];
    while (1) {
        int len = recv(client, buf, sizeof(buf) - 1, 0);
        
        if (len <= 0) {
            printf("\n[Server da ngat ket noi]\n");
            exit(0);
        }
        
        buf[len] = 0;
        printf("%s", buf);
        fflush(stdout);
    }
    return NULL;
}