#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

int listener;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        int client = accept(listener, NULL, NULL);
        pthread_mutex_unlock(&mutex);
        
        if (client < 0) continue;

        printf("New client connected: %d\n", client);

        char buf[256];
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        
        if (ret > 0) {
            buf[ret] = 0;
            puts(buf);
            
            char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nXin chao cac ban\n";
            send(client, msg, strlen(msg), 0);
        }
        
        close(client);
    }
    return NULL;
}

int main() {
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);

    pthread_t threads[5];
    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    close(listener);
    return 0;
}