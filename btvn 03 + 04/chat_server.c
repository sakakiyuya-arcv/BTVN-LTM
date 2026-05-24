#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

int clients[256];
int num_clients = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *client_thread(void *arg) {
    int client = *(int *)arg;
    free(arg);
    char buf[1024];
    char client_name[64];

    while (1) {
        send(client, "(client_id: client_name): ", 35, 0);
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) {
            close(client);
            return NULL;
        }
        buf[ret] = 0;
        buf[strcspn(buf, "\r\n")] = 0;

        if (sscanf(buf, "client_id: %s", client_name) == 1) {
            break;
        }
    }

    pthread_mutex_lock(&mutex);
    clients[num_clients++] = client;
    pthread_mutex_unlock(&mutex);

    while (1) {
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break;
        buf[ret] = 0;
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0) continue;

        time_t t = time(NULL);
        struct tm *tmp = localtime(&t);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p", tmp);

        char send_buf[2048];
        snprintf(send_buf, sizeof(send_buf), "%s %s: %s\n", time_str, client_name, buf);

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_clients; i++) {
            if (clients[i] != client) {
                send(clients[i], send_buf, strlen(send_buf), 0);
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_clients; i++) {
        if (clients[i] == client) {
            clients[i] = clients[num_clients - 1];
            num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
    close(client);
    return NULL;
}

int main() {
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