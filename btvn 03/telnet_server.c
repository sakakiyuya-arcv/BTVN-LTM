#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

void handle_client(int client, char accounts[][64], int account_count) {
    char buf[1024] = {0};
    int authenticated = 0;
    char *prompt = "User va Pass";
    send(client, prompt, strlen(prompt), 0);

    while (1) {
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break;
        
        buf[ret] = '\0';
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0) continue;

        if (!authenticated) {
            for (int k = 0; k < account_count; k++) {
                if (strcmp(buf, accounts[k]) == 0) {
                    authenticated = 1;
                    break;
                }
            }

            if (authenticated) {
                char *success = "Dang nhap thanh cong!\n> ";
                send(client, success, strlen(success), 0);
            } else {
                char *fail = "Loi: Sai tai khoan hoac mat khau. Nhap lai:\n";
                send(client, fail, strlen(fail), 0);
            }
        } 
        else {
            char command[2048] = {0};
            char filename[64] = {0};
            
            snprintf(filename, sizeof(filename), "out_%d.txt", getpid());
            snprintf(command, sizeof(command), "%s > %s 2>&1", buf, filename);
            system(command);

            FILE *f = fopen(filename, "r");
            if (f != NULL) {
                char *line = NULL;
                size_t len = 0;
                ssize_t read_bytes;
                
                while ((read_bytes = getline(&line, &len, f)) != -1) {
                    send(client, line, strlen(line), 0);
                }
                
                fclose(f);
                if (line) {
                    free(line);
                }
                remove(filename);
            } else {
                char *err = "Loi thuc thi lenh.\n";
                send(client, err, strlen(err), 0);
            }
            
            char *done = "> ";
            send(client, done, strlen(done), 0);
        }
    }
    close(client);
}

int main() {
    signal(SIGCHLD, SIG_IGN);

    FILE *file = fopen("acc.txt", "r");
    if (file == NULL) {
        perror("Loi mo file acc.txt");
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read_bytes;
    char accounts[100][64];
    int account_count = 0;

    while ((read_bytes = getline(&line, &len, file)) != -1) {
        line[strcspn(line, "\r\n")] = 0;
        strcpy(accounts[account_count], line);
        account_count++;
    }
    fclose(file);
    if (line) free(line);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("Loi tao socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Loi bind");
        close(listener);
        return 1;
    }

    if (listen(listener, 10) < 0) {
        perror("Loi listen");
        close(listener);
        return 1;
    }

    printf("Server dang chay tren cong 8080...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        if (fork() == 0) {
            close(listener);
            handle_client(client, accounts, account_count);
            exit(0);
        }
        close(client);
    }

    close(listener);
    return 0;
}