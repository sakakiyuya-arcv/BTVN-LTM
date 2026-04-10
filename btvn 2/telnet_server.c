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
    
    fd_set fdread, fdtest, fdcheck;
    FD_ZERO(&fdread);
    FD_ZERO(&fdcheck);

    FD_SET(listener, &fdread);

    struct timeval tv;
    char buf[256]; 

    while (1) {
        fdtest = fdread;

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int ret = select(FD_SETSIZE, &fdtest, NULL, NULL, NULL);
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
                        
                        char *prompt = "Enter user and pass: ";
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
                        close(i); 
                    } 
                    else {
                        buf[ret] = '\0'; 
                        buf[strcspn(buf, "\r\n")] = 0; 
                        
                        if (FD_ISSET(i, &fdcheck)) { 
                            printf("Command from %d: %s\n", i, buf);
                            
                            char command[1024];
                            snprintf(command, sizeof(command), "%s > out.txt", buf);
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
                                    FD_SET(i, &fdcheck); 
                                    break;
                                }
                            }
                            
                            if (!FD_ISSET(i, &fdcheck)) {
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
    return 0;
}
