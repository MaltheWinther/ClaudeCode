/* UDP client in the internet domain
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
	printf("Starting UDP client...\n");
	int sock, n;
	socklen_t serverlength;
	struct sockaddr_in server;
	struct hostent *hp;
	char buf[256];
	
	if (argc != 3) error("USAGE: server port\n");
	
	sock= socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) error("ERROR, socket");

	server.sin_family = AF_INET;
	hp = gethostbyname(argv[1]);
	if (hp==0) error("ERROR, Unknown host");

	bcopy((char *)hp->h_addr_list[0],(char *)&server.sin_addr,hp->h_length);
	server.sin_port = htons(atoi(argv[2]));
	serverlength=sizeof(server);
	printf("Please enter the message: ");
	bzero(buf,sizeof(buf));
	fgets(buf,sizeof(buf)-1,stdin);

	n=sendto(sock,buf,strlen(buf),0,(const struct sockaddr *)&server,serverlength);
	if (n < 0) error("ERROR, Sendto");
	n = recvfrom(sock,buf,sizeof(buf),0,(struct sockaddr *)&server, &serverlength);
	buf[n]=0;  //handle null termination
	if (n < 0) error("ERROR, recvfrom");
	printf("Got a datagram: %s\n", buf);
	
	close(sock);
	return 0;
}

