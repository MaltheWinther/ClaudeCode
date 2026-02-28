/* UDP measurement client for Øvelse 7
   Sends a command character to the measurement server and prints the response.

   Usage: ./get_measurement <server-ip> <cmd>
   Example: ./get_measurement 172.16.15.2 u

   Based on demo by Michael Alrøe
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PORT 9000
#define BUF_SIZE 256

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server-ip> <cmd>\n", argv[0]);
        fprintf(stderr, "  cmd: U/u for uptime, L/l for load average\n");
        exit(1);
    }

    int sock, n;
    socklen_t serverlength;
    struct sockaddr_in server;
    struct hostent *hp;
    char buf[BUF_SIZE];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) error("ERROR, socket");

    server.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    if (hp == 0) error("ERROR, unknown host");

    bcopy((char *)hp->h_addr_list[0], (char *)&server.sin_addr, hp->h_length);
    server.sin_port = htons(PORT);
    serverlength = sizeof(server);

    /* Send the command character */
    bzero(buf, sizeof(buf));
    buf[0] = argv[2][0];

    n = sendto(sock, buf, 1, 0, (const struct sockaddr *)&server, serverlength);
    if (n < 0) error("ERROR, sendto");

    /* Receive and print the response */
    n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                 (struct sockaddr *)&server, &serverlength);
    if (n < 0) error("ERROR, recvfrom");
    buf[n] = '\0';

    printf("%s", buf);

    close(sock);
    return 0;
}
