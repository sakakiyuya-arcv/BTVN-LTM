#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        // printf("Sử dụng: %s <Địa chỉ IP> <Cổng>\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char mssv[10], hoTen[50], ngaySinh[20], diemTrungBinh[10];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket failed\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    // if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    //     printf("Địa chỉ IP không hợp lệ\n");
    //     close(client_socket);
    //     exit(1);
    // }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed\n");
        close(client_socket);
        exit(1);
    }

    printf("Connected to server\n");
    buffer[0] = '\0';
    // printf("Nhập tin nhắn (Gõ 'exit' để thoát):\n");

    printf("Nhập MSSV: ");
    fgets(mssv, sizeof(mssv), stdin);
    mssv[strcspn(mssv, "\n")] = ' '; 
    strcat(buffer, mssv);
    printf("Nhập Họ Tên: ");
    fgets(hoTen, sizeof(hoTen), stdin);
    hoTen[strcspn(hoTen, "\n")] = ' '; 
    strcat(buffer, hoTen);
    printf("Nhập Ngày Sinh: ");
    fgets(ngaySinh, sizeof(ngaySinh), stdin);
    ngaySinh[strcspn(ngaySinh, "\n")] = ' '; 
    strcat(buffer, ngaySinh);
    printf("Nhập Điểm Trung Bình: ");
    fgets(diemTrungBinh, sizeof(diemTrungBinh), stdin);
    // diemTrungBinh[strcspn(diemTrungBinh, "\n")] = 0;
    strcat(buffer, diemTrungBinh);

    int sent_bytes = send(client_socket, buffer, strlen(buffer), 0);

    if (sent_bytes > 0) {
        printf("send success!\n");
    } else {
        perror("send failed\n");
    }

    // while (1) {
    //     printf("> ");
    //     if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;

    //     // buffer[strcspn(buffer, "\n")] = 0;

    //     if (strncmp(buffer, "exit", 4) == 0) break;

    //     if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
    //         perror("Lỗi gửi dữ liệu");
    //         break;
    //     }
    // }

    close(client_socket);
    // printf("Đã đóng kết nối.\n");

    return 0;
}