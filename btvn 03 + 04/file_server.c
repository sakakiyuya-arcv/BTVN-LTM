#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

void handle_client(int client) {
    DIR *d = opendir(".");
    if (!d) {
        close(client);
        return;
    }

    struct dirent *dir;
    char file_list[256][256];
    int count = 0;

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) {
            strcpy(file_list[count], dir->d_name);
            count++;
        }
    }
    closedir(d);

    if (count == 0) {
        char *err = "ERROR No files to download \r\n";
        send(client, err, strlen(err), 0);
        close(client);
        return;
    }

    char header[64];
    snprintf(header, sizeof(header), "OK %d\r\n", count);
    send(client, header, strlen(header), 0);

    for (int i = 0; i < count; i++) {
        char line[512];
        snprintf(line, sizeof(line), "%s\r\n", file_list[i]);
        send(client, line, strlen(line), 0);
    }
    send(client, "\r\n", 2, 0);

    char buf[256];
    while (1) {
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break;
        
        buf[ret] = 0;
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0) continue;

        FILE *f = fopen(buf, "rb");
        if (f != NULL) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);

            char ok_msg[64];
            snprintf(ok_msg, sizeof(ok_msg), "OK %ld\r\n", size);
            send(client, ok_msg, strlen(ok_msg), 0);

            char file_buf[1024];
            int bytes;
            while ((bytes = fread(file_buf, 1, sizeof(file_buf), f)) > 0) {
                send(client, file_buf, bytes, 0);
            }
            fclose(f);
            break; 
        } else {
            char *err_msg = "ERROR File khong ton tai. Nhap lai:\r\n";
            send(client, err_msg, strlen(err_msg), 0);
        }
    }
    close(client);
}

int main() {
    signal(SIGCHLD, SIG_IGN);

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

        if (fork() == 0) {
            close(listener);
            handle_client(client);
            exit(0);
        }
        close(client);
    }

    close(listener);
    return 0;
}