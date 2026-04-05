#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

typedef enum { STATE_WANT_NAME, STATE_WANT_MSSV, STATE_DONE } ClientState;

typedef struct {
    int fd;
    ClientState state;
    char name[100];
    char mssv[20];
} ClientContext;

void send_email(ClientContext *ctx) {
    char email[256] = "";
    char temp_name[100];
    strcpy(temp_name, ctx->name);

    char *words[10];
    int count = 0;
    char *token = strtok(temp_name, " \n\r");
    while (token && count < 10) {
        words[count++] = token;
        token = strtok(NULL, " \n\r");
    }

    if (count > 0) {
        strcpy(email, words[count-1]); 
        strcat(email, ".");
        for (int i = 0; i < count - 1; i++) {
            strncat(email, words[i], 1); 
        }
        strcat(email, ctx->mssv + 2);
        strcat(email, "@sis.hust.edu.vn\n");
        send(ctx->fd, "Your email is: ", 15, 0);
        send(ctx->fd, email, strlen(email), 0);
    }
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(8080), .sin_addr.s_addr = INADDR_ANY };
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    ClientContext clients[64];
    int nclients = 0;
    char buf[256];

    printf("Non-blocking Server started on port 8080...\n");

    while (1) {
        int new_fd = accept(listener, NULL, NULL);
        if (new_fd != -1) {
            ioctl(new_fd, FIONBIO, &ul);
            clients[nclients].fd = new_fd;
            clients[nclients].state = STATE_WANT_NAME;
            send(new_fd, "Nhap Ho va Ten: ", 17, 0);
            nclients++;
        }

        for (int i = 0; i < nclients; i++) {
            int len = recv(clients[i].fd, buf, sizeof(buf)-1, 0);
            
            if (len > 0) {
                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0;

                if (clients[i].state == STATE_WANT_NAME) {
                    strcpy(clients[i].name, buf);
                    clients[i].state = STATE_WANT_MSSV;
                    send(clients[i].fd, "Nhap MSSV: ", 11, 0);
                } 
                else if (clients[i].state == STATE_WANT_MSSV) {
                    strcpy(clients[i].mssv, buf);
                    send_email(&clients[i]);
                    clients[i].state = STATE_DONE;
                }
            }
        }
    }
    return 0;
}