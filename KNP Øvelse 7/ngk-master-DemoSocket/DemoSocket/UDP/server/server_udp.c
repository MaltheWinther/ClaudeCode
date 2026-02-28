/* Creates a datagram server.  The port
number is passed as an argument.  This
server runs forever
Based on example: https://www.linuxhowtos.org/C_C++/socket.htm

Modified: Michael Alrøe
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

int main(int argc, char *argv[])
{
	printf("Starting UDP server...\n");
	int sock, n;
	socklen_t fromlen;
	struct sockaddr_in server;
	struct sockaddr_in from;
	char buf[256];

	if (argc < 2) error("ERROR, no port provided\n");
	
	sock=socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) error("ERROR, socket");

	bzero(&server,sizeof(server));
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(atoi(argv[1]));

	printf("Binding...\n");
	if (bind(sock,(struct sockaddr *)&server,sizeof(server))<0)
	    error("ERROR, binding");

	fromlen = sizeof(from);
	while (1) {
		printf("Receive...\n");
		n = recvfrom(sock,buf,sizeof(buf),0,(struct sockaddr*)&from,&fromlen);
		if (n < 0) error("ERROR, recvfrom");
        buf[n]=0;  //handle null termination 
		printf("Received a datagram: %s", buf);
		n = sendto(sock,buf, strlen(buf),0,(struct sockaddr*)&from,fromlen);
		if (n  < 0) error("ERROR, sendto");
	}

	return 0;
}
