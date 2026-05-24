#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>

int waiting_client = -1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct Pair {
    int c1;
    int c2;
};

void *pair_thread(void *arg) {
    struct Pair *p = (struct Pair *)arg;
    int c1 = p->c1;
    int c2 = p->c2;
    free(p);

    char buf[1024];
    fd_set readfds;
    int max_fd = (c1 > c2) ? c1 : c2;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(c1, &readfds);
        FD_SET(c2, &readfds);

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            break;
        }

        if (FD_ISSET(c1, &readfds)) {
            int ret = recv(c1, buf, sizeof(buf), 0);
            if (ret <= 0) break;
            send(c2, buf, ret, 0);
        }

        if (FD_ISSET(c2, &readfds)) {
            int ret = recv(c2, buf, sizeof(buf), 0);
            if (ret <= 0) break;
            send(c1, buf, ret, 0);
        }
    }

    close(c1);
    close(c2);
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

        pthread_mutex_lock(&mutex);
        if (waiting_client == -1) {
            waiting_client = client;
            char *msg = "Dang cho ghep cap...\n";
            send(client, msg, strlen(msg), 0);
            pthread_mutex_unlock(&mutex);
        } else {
            int c1 = waiting_client;
            int c2 = client;
            waiting_client = -1;
            pthread_mutex_unlock(&mutex);

            char *msg = "Da ghep cap! Bat dau chat:\n";
            send(c1, msg, strlen(msg), 0);
            send(c2, msg, strlen(msg), 0);

            struct Pair *p = malloc(sizeof(struct Pair));
            p->c1 = c1;
            p->c2 = c2;

            pthread_t tid;
            pthread_create(&tid, NULL, pair_thread, p);
            pthread_detach(tid);
        }
    }

    close(listener);
    return 0;
}