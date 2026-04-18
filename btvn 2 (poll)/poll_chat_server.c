#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>

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
    
    struct pollfd fds[1024];
    int fdcheck[1024];
    char client_id[1024][64];

    for (int i = 0; i < 1024; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
        fdcheck[i] = 0;
    }

    fds[listener].fd = listener;
    char buf[256];

    while (1) {
        int ret = poll(fds, 1024, 5000);
        
        if (ret < 0) {
            perror("poll() failed");
            break;
        }
        if (ret == 0) {
            continue;
        }

        for (int i = 0; i < 1024; i++) {
            if (fds[i].fd != -1 && (fds[i].revents & POLLIN)) {
                
                if (i == listener) {
                    int client = accept(listener, NULL, NULL);
                    if (client < 1024) {
                        printf("New client connected: %d\n", client);
                        fds[client].fd = client;
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
                        fds[i].fd = -1;
                        fdcheck[i] = 0;
                        memset(client_id[i], 0, sizeof(client_id[i])); 
                        close(i); 
                    } 
                    else {
                        buf[ret] = '\0'; 
                        buf[strcspn(buf, "\r\n")] = 0; 
                        
                        if (fdcheck[i]) { 
                            printf("Received from %d: %s\n", i, buf);
                            
                            char temp[1024];
                            snprintf(temp, sizeof(temp), "%s%s\n", client_id[i], buf);
                            
                            for (int j = 0; j < 1024; j++) {
                                if (fds[j].fd != -1 && j != listener && j != i) {
                                    if (fdcheck[j]) {
                                        send(j, temp, strlen(temp), 0);
                                    }
                                }
                            }
                        } 
                        else { 
                            char id[30], name[64];
                            
                            if (sscanf(buf, "%29[^:]: %63[^\n]", id, name) == 2) {
                                fdcheck[i] = 1; 
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