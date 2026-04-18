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
    FILE *file = fopen("acc.txt", "r");
    if (file == NULL) {
        perror("Lỗi mở file");
        return 1;
    }

    char *line = NULL;    
    size_t len = 0;       
    ssize_t read;         
    char accounts[100][64]; 
    int account_count = 0; 

    while ((read = getline(&line, &len, file)) != -1) {
        line[strcspn(line, "\r\n")] = 0;
        strcpy(accounts[account_count], line);
        account_count++;
    }

    fclose(file);
    
    if (line) {
        free(line);
        line = NULL;
        len = 0;
    }
    
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
                    if (client >= 0 && client < 1024) {
                        printf("New client connected: %d\n", client);
                        fds[client].fd = client;
                        
                        char *prompt = "Enter user and pass: ";
                        send(client, prompt, strlen(prompt), 0);
                    } else if (client >= 1024) {
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
                        close(i); 
                    } 
                    else {
                        buf[ret] = '\0'; 
                        buf[strcspn(buf, "\r\n")] = 0; 
                        
                        if (strlen(buf) == 0) {
                            continue;
                        }
                        
                        if (fdcheck[i]) { 
                            printf("Command from %d: %s\n", i, buf);
                            
                            char command[1024];
                            snprintf(command, sizeof(command), "%s > out.txt 2>&1", buf);
                            system(command);
                            
                            FILE *f = fopen("out.txt", "r");
                            if (f != NULL) {
                                send(i, "Command output:\n", 16, 0);
                                while ((read = getline(&line, &len, f)) != -1) {
                                    send(i, line, strlen(line), 0);
                                }

                                fclose(f);
                                if (line) {
                                    free(line);
                                    line = NULL; 
                                    len = 0;
                                }
                            } else {
                                char *err = "Error.\n";
                                send(i, err, strlen(err), 0);
                            }
                        } 
                        else { 
                            for(int k = 0; k < account_count; k++) {
                                if (strcmp(buf, accounts[k]) == 0) {
                                    char *success = "Authentication successful!\n";
                                    send(i, success, strlen(success), 0);
                                    fdcheck[i] = 1; 
                                    break;
                                }
                            }
                            
                            if (!fdcheck[i]) {
                                char *fail = "Authentication failed! Please try again.\n";
                                send(i, fail, strlen(fail), 0);
                            } 
                        }
                    }
                }            
            }
        }
    }

    close(listener);
    if (line) free(line);
    return 0;
}