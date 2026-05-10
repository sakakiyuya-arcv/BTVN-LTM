#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);

    for (int i = 0; i < 5; i++) {
        if (fork() == 0) {
            while (1) {
                int client = accept(listener, NULL, NULL);
                if (client < 0) continue;

                char buf[1024];
                int ret = recv(client, buf, sizeof(buf) - 1, 0);
                
                if (ret > 0) {
                    buf[ret] = 0;
                    puts(buf);
                    
                    char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nXin chao cac ban\n";
                    send(client, msg, strlen(msg), 0);
                }
                
                close(client);
            }
        }
    }

    wait(NULL);
    close(listener);
    return 0;
}