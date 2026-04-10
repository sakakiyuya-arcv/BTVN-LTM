#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }
    
    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    
    printf("Server is listening on port 8080...\n");
    
    fd_set fdread, fdtest, fdcheck;
    FD_ZERO(&fdread);
    FD_ZERO(&fdcheck);

    FD_SET(listener, &fdread);

    struct timeval tv;
    char buf[256];
    
    char client_id[FD_SETSIZE][64]; 

    while (1) {
        fdtest = fdread;

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int ret = select(FD_SETSIZE, &fdtest, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select() failed");
            break;
        }
        if (ret == 0) {
            continue;
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &fdtest)) {
                if (i == listener) {
                    int client = accept(listener, NULL, NULL);
                    if (client < FD_SETSIZE) {
                        printf("New client connected: %d\n", client);
                        FD_SET(client, &fdread); 
                        
                        char *prompt = "Vui long nhap ten (id: name): ";
                        send(client, prompt, strlen(prompt), 0);
                    } else {
                        printf("Max clients reached. Connection rejected.\n");
                        close(client);
                    }
                } 
                else {
                    ret = recv(i, buf, sizeof(buf) - 1, 0); 

                    if (ret <= 0) {
                        printf("Client %d disconnected\n", i);    
                        FD_CLR(i, &fdread);
                        FD_CLR(i, &fdcheck);
                        memset(client_id[i], 0, sizeof(client_id[i])); 
                        close(i); 
                    } 
                    else {
                        buf[ret] = '\0'; 
                        buf[strcspn(buf, "\r\n")] = 0; 
                        
                        if (FD_ISSET(i, &fdcheck)) { 
                            printf("Received from %d: %s\n", i, buf);
                            
                            char temp[1024];
                            snprintf(temp, sizeof(temp), "%s%s\n", client_id[i], buf);
                            
                            for (int j = 0; j < FD_SETSIZE; j++) {
                                if (FD_ISSET(j, &fdread) && j != listener && j != i) {
                                    if (FD_ISSET(j, &fdcheck)) {
                                        send(j, temp, strlen(temp), 0);
                                    }
                                }
                            }
                        } 
                        else { 
                            char id[30], name[64];
                            
                            if (sscanf(buf, "%29[^:]: %63[^\n]", id, name) == 2) {
                                FD_SET(i, &fdcheck); 
                                snprintf(client_id[i], sizeof(client_id[i]), "%s: ", id);
                                
                                char *success = "Xac thuc thanh cong! Ban da vao phong.\n";
                                send(i, success, strlen(success), 0);
                                printf("Client %d xac thuc thanh cong voi ID: %s\n", i, id);
                            } else {
                                char *fail = "Sai cu phap. Nhap lai (id: name): ";
                                send(i, fail, strlen(fail), 0);
                            }
                        }
                    }
                }            
            }
        }
    }

    close(listener);
    return 0;
}