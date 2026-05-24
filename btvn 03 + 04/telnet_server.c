#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

char accounts[100][64];
int account_count = 0;

void *client_thread(void *arg) {
    int client = *(int *)arg;
    free(arg);
    char buf[1024] = {0};
    int authenticated = 0;
    
    send(client, "User and Pass:\n", 14, 0);

    while (1) {
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break;
        
        buf[ret] = '\0';
        buf[strcspn(buf, "\r\n")] = 0;
        if (strlen(buf) == 0) continue;

        if (!authenticated) {
            for (int k = 0; k < account_count; k++) {
                if (strcmp(buf, accounts[k]) == 0) {
                    authenticated = 1;
                    break;
                }
            }

            if (authenticated) {
                send(client, "OK\n> ", 5, 0);
            } else {
                send(client, "ERROR\n", 4, 0);
            }
        } 
        else {
            char command[2048] = {0};
            char filename[64] = {0};
            
            snprintf(filename, sizeof(filename), "out_%d.txt", client);
            snprintf(command, sizeof(command), "%s > %s 2>&1", buf, filename);
            system(command);

            FILE *f = fopen(filename, "r");
            if (f != NULL) {
                char *line = NULL;
                size_t len = 0;
                while (getline(&line, &len, f) != -1) {
                    send(client, line, strlen(line), 0);
                }
                fclose(f);
                if (line) free(line);
                remove(filename);
            }
            send(client, "> ", 2, 0);
        }
    }
    close(client);
    return NULL;
}

int main() {
    FILE *file = fopen("acc.txt", "r");
    if (file) {
        char *line = NULL;
        size_t len = 0;
        while (getline(&line, &len, file) != -1) {
            line[strcspn(line, "\r\n")] = 0;
            strcpy(accounts[account_count++], line);
        }
        fclose(file);
        if (line) free(line);
    }

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        pthread_t tid;
        int *arg = malloc(sizeof(int));
        *arg = client;
        pthread_create(&tid, NULL, client_thread, arg);
        pthread_detach(tid);
    }
    close(listener);
    return 0;
}