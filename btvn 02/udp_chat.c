#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        return 1;
    }

    int port_s = atoi(argv[1]), port_d = atoi(argv[3]);
    char *ip_d = argv[2];
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in my_addr = {0}, dest_addr = {0};
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port_s);
    bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr));

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(ip_d);
    dest_addr.sin_port = htons(port_d);

    fcntl(0, F_SETFL, O_NONBLOCK);    
    fcntl(sock, F_SETFL, O_NONBLOCK); 

    char buf[1024];
    printf("Listening on port %d. Sending to %s:%d\n\n", port_s, ip_d, port_d);

    while (1) {
        int bytes_read = read(0, buf, sizeof(buf));
        if (bytes_read > 0) {
            sendto(sock, buf, bytes_read, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        }

        int n = recvfrom(sock, buf, sizeof(buf) - 1, 0, NULL, NULL);
        if (n > 0) {
            buf[n] = '\0'; 
            printf(">> port %d: %s", port_d, buf);
        }

    }
    return 0;
}