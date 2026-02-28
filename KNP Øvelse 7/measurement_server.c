/* UDP measurement server for Øvelse 7
   Listens on port 9000.
   Responds to:
     'U' or 'u' -> content of /proc/uptime
     'L' or 'l' -> content of /proc/loadavg
     anything else -> error message

   Start with: ./measurement_server

   Based on demo by Michael Alrøe
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

/* Read the contents of a /proc file into buf. Returns 0 on success. */
int read_proc_file(const char *path, char *buf, int buf_size)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) return -1;
    int n = fread(buf, 1, buf_size - 1, f);
    buf[n] = '\0';
    fclose(f);
    return 0;
}

int main(void)
{
    printf("Starting measurement server on port %d...\n", PORT);

    int sock, n;
    socklen_t fromlen;
    struct sockaddr_in server;
    struct sockaddr_in from;
    char recv_buf[BUF_SIZE];
    char send_buf[BUF_SIZE];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) error("ERROR, socket");

    bzero(&server, sizeof(server));
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port        = htons(PORT);

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
        error("ERROR, binding");

    printf("Server ready. Waiting for requests...\n");

    fromlen = sizeof(from);
    while (1) {
        n = recvfrom(sock, recv_buf, sizeof(recv_buf), 0,
                     (struct sockaddr *)&from, &fromlen);
        if (n < 0) error("ERROR, recvfrom");
        recv_buf[n] = '\0';

        char cmd = recv_buf[0];
        printf("Received command: '%c'\n", cmd);

        bzero(send_buf, sizeof(send_buf));

        if (cmd == 'U' || cmd == 'u') {
            if (read_proc_file("/proc/uptime", send_buf, sizeof(send_buf)) < 0)
                strncpy(send_buf, "ERROR: could not read /proc/uptime\n", sizeof(send_buf) - 1);
        } else if (cmd == 'L' || cmd == 'l') {
            if (read_proc_file("/proc/loadavg", send_buf, sizeof(send_buf)) < 0)
                strncpy(send_buf, "ERROR: could not read /proc/loadavg\n", sizeof(send_buf) - 1);
        } else {
            snprintf(send_buf, sizeof(send_buf),
                     "ERROR: unknown command '%c'. Use 'U' for uptime or 'L' for load average.\n", cmd);
        }

        n = sendto(sock, send_buf, strlen(send_buf), 0,
                   (struct sockaddr *)&from, fromlen);
        if (n < 0) error("ERROR, sendto");
    }

    return 0;
}
