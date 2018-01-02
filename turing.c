#include	<string.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<stdio.h>

int main()
{
	int sfd;
	int connfd;
	struct sockaddr_in addr;
	
	if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket failed!\n");
		exit(EXIT_FAILURE);
	}
	
	memset(&addr.sin_zero, 0, sizeof(addr.sin_zero));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(MYPORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Bind failed!\n");
	}
	
		// again, straight from hello world
	if (listen(sfd, 10) < 0) {
		perror("listen failed!\n");
		
		
	// continue forever!
	while (1) {

		// if we succesfully get a connection
		if ((connfd = accept(sfd, NULL, NULL)) < 0) {
			perror("Accept failed!\n");
			exit(EXIT_FAILURE);
		}

	char buffer[1024];
	strcpy(buffer, "Hello there!");
	send(connfd, buffer, strlen(buffer), 0);
	}
	
	close(connfd);
	close(sfd);
}