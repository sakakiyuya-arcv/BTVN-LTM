#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>

void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2]))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10); else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10); else b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void send_file(int client, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        char *msg = "HTTP/1.1 404 Not Found\r\n\r\nFile khong ton tai.";
        send(client, msg, strlen(msg), 0);
        return;
    }

    char *content_type = "application/octet-stream";
    if (strstr(path, ".txt")) content_type = "text/plain; charset=UTF-8";
    else if (strstr(path, ".html")) content_type = "text/html; charset=UTF-8";
    else if (strstr(path, ".mp3")) content_type = "audio/mpeg";
    else if (strstr(path, ".wav")) content_type = "audio/wav";
    else if (strstr(path, ".mp4")) content_type = "video/mp4";
    else if (strstr(path, ".jpg") || strstr(path, ".jpeg")) content_type = "image/jpeg";
    else if (strstr(path, ".png")) content_type = "image/png";

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char header[512];
    snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", content_type, size);
    send(client, header, strlen(header), 0);

    char buf[4096];
    int bytes;
    while ((bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
        send(client, buf, bytes, 0);
    }
    fclose(f);
}

void send_dir(int client, const char *path, const char *uri) {
    DIR *d = opendir(path);
    if (!d) {
        char *msg = "HTTP/1.1 404 Not Found\r\n\r\nThu muc khong ton tai.";
        send(client, msg, strlen(msg), 0);
        return;
    }

    char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n<html><body><h2>Danh sach tep tin va thu muc:</h2>";
    send(client, header, strlen(header), 0);

    struct dirent *dir;
  
    char line[4096]; 
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;

        char full_path[1024];
        if (strcmp(path, ".") == 0) {
            snprintf(full_path, sizeof(full_path), "./%s", dir->d_name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", path, dir->d_name);
        }

        struct stat st;
        stat(full_path, &st);

        char link[1024];
        if (strcmp(uri, "/") == 0) {
            snprintf(link, sizeof(link), "/%s", dir->d_name);
        } else {
            snprintf(link, sizeof(link), "%s/%s", uri, dir->d_name);
        }

        if (S_ISDIR(st.st_mode)) {
            snprintf(line, sizeof(line), "<b><a href=\"%s\">%s/</a></b><br>", link, dir->d_name);
        } else {
            snprintf(line, sizeof(line), "<i><a href=\"%s\">%s</a></i><br>", link, dir->d_name);
        }
        send(client, line, strlen(line), 0);
    }
    closedir(d);

    char footer[] = "</body></html>";
    send(client, footer, strlen(footer), 0);
}

void process_request(int client, char *request) {
    char method[16], raw_uri[256], uri[256];
    sscanf(request, "%s %s", method, raw_uri);

    url_decode(uri, raw_uri);

    char path[512] = ".";
    if (strcmp(uri, "/") != 0) {
        snprintf(path, sizeof(path), ".%s", uri);
    }

    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            send_dir(client, path, uri);
        } else {
            send_file(client, path);
        }
    } else {
        char *msg = "HTTP/1.1 404 Not Found\r\n\r\nDuong dan khong ton tai.";
        send(client, msg, strlen(msg), 0);
    }
}

int main() {
    signal(SIGCHLD, SIG_IGN);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    printf("Server is listening on port 8080...\n");

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        if (fork() == 0) {
            close(listener);
            char buf[4096] = {0};
            recv(client, buf, sizeof(buf) - 1, 0);
            if (strlen(buf) > 0) {
                process_request(client, buf);
            }
            close(client);
            exit(0);
        }
        close(client);
    }

    close(listener);
    return 0;
}