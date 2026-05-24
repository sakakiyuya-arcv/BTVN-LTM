#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

void *client_thread(void *arg) {
    int client = *(int *)arg;
    free(arg); 
    
    char buf[256];
    
    while (1) {
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break;

        buf[ret] = '\0';
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0) continue;

        char cmd[32] = {0}, format[32] = {0};
        int n = sscanf(buf, "%s %s", cmd, format);

        if (n < 2 || strcmp(cmd, "GET_TIME") != 0) {
            char *msg = "Loi: Sai cu phap. Dung: GET_TIME [format]\n";
            send(client, msg, strlen(msg), 0);
            continue;
        }

        char *time_fmt = NULL;
        if (strcmp(format, "dd/mm/yyyy") == 0) time_fmt = "%d/%m/%Y";
        else if (strcmp(format, "dd/mm/yy") == 0) time_fmt = "%d/%m/%y";
        else if (strcmp(format, "mm/dd/yyyy") == 0) time_fmt = "%m/%d/%Y";
        else if (strcmp(format, "mm/dd/yy") == 0) time_fmt = "%m/%d/%y";

        if (time_fmt != NULL) {
            time_t t = time(NULL);
            struct tm *tmp = localtime(&t);
            char out[64] = {0};
            
            strftime(out, sizeof(out), time_fmt, tmp);
            strcat(out, "\n");
            
            send(client, out, strlen(out), 0);
        } else {
            char *msg = "Loi: Dinh dang thoi gian khong duoc ho tro.\n";
            send(client, msg, strlen(msg), 0);
        }
    }
    
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

        int *arg = malloc(sizeof(int));
        *arg = client;

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, arg);
        pthread_detach(tid); 
    }

    close(listener);
    return 0;
}