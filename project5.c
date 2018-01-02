// Shawn Banish - CS 0449 Project 5 (Multi-Threaded Web Server)

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MYPORT 55003

// worker thread function
void *handle_requests(void *p);	
// used to communicate
void send_funct(char *to_send, int connfd);
// writes to stats.txt
void write_stats(char *to_write);


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// begin main
int main() {
	// declare socket ID
	int sfd;
	// declare client ID
	int connfd;
	// taken from lab 6
	struct sockaddr_in addr;	

	// used for threading
	int thread_id;	
	// pthreads
	pthread_t thread;

	// taken from lab6
	if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket failed!\n");
		exit(EXIT_FAILURE);
	}

	// still taken from lab6
	memset(&addr.sin_zero, 0, sizeof(addr.sin_zero));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(MYPORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	// still taken from lab6
	if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
	{
		perror("Bind failed!\n");
		exit(EXIT_FAILURE);
	}

	// still taken from lab6 (almost done)
	if (listen(sfd, 10) < 0) 
	{
		perror("listen failed!\n");
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		// if we get a conncection and accept it
		if ((connfd = accept(sfd, NULL, NULL)) < 0) 
		{
			perror("Accept failed!\n");
			exit(EXIT_FAILURE);
		}

		// spawn our worker thread
		thread_id = pthread_create(&thread, NULL, handle_requests, (void *)&connfd);
	}

	close(sfd);
	return 0;
}

// void *p = connfd
void *handle_requests(void *p) 
{
	char *filename;
	char *temp_buffer;
	char *big_send_buffer;

	// used to check HTTP
	char http_checker[8];	
	// For user sent
	char rcv_buffer[1024];
	// temp buffer
	char send_buffer[1024];

	int file_size = 0;
	int amt = 0;
	int connfd = *(int *)p;
	
	// for time/date
	time_t time_is;
	struct tm *time_struct;

	if (recv(connfd, rcv_buffer, 1024, 0) > 0) 
	{
		if (strlen(rcv_buffer) <= 15) 
		{
			// makes  space
			temp_buffer = malloc(75);
			strcat(temp_buffer, "\nYour query must be at least 16 characters.\n");
			strcat(temp_buffer, "Closing connection.\n");
			// send the message
			send_funct(temp_buffer, connfd);
			// give the space back
			free(temp_buffer);	
		
			// close the connection
			close(connfd);

			// write to stats.txt
			pthread_mutex_lock(&mutex);
			write_stats(rcv_buffer);
			pthread_mutex_unlock(&mutex);

			return NULL;
		}


		if (!strncmp("GET", rcv_buffer, 3)) 
		{
			// make space
			filename = malloc(strlen(rcv_buffer) - 14);
			// copy filename
			strncpy(filename, rcv_buffer + 4, strlen(rcv_buffer) - 15);
			// copy HTTP
			strncpy(http_checker, rcv_buffer + 4 + strlen(filename) + 1, 8);

			if (!strncmp(http_checker, "HTTP/1.1", 8)) 
			{	
				// try to open file
				FILE *f = fopen(filename, "rt");

				if (f == NULL) 
				{
					// if not found, 404
					strcat(send_buffer, "\nHTTP/1.1 404 Not Found\n\n");
					send_funct(send_buffer, connfd);

					// close connection
					close(connfd);

					pthread_mutex_lock(&mutex);
					write_stats(rcv_buffer);
					pthread_mutex_unlock(&mutex);
					return NULL;
				}
				
				// go to end
				fseek(f, 0, SEEK_END);	
				// and see how far we've gone, the file_size
				file_size = ftell(f);
				// then go back to the beginning
				fseek(f, 0, SEEK_SET);

				// get current time
				time_is = time(NULL);
				// conver
				time_struct = gmtime(&time_is);	
				// make some room
				temp_buffer = malloc(50);

				// tprint time
				strftime (temp_buffer, 50, "Date: %a, %d %b %Y %X %Z\n", time_struct );

				strcat(send_buffer, "\nHTTP/1.1 200 OK\n");
				strcat(send_buffer, temp_buffer);
				strcat(send_buffer, "Content-Length: ");
				sprintf(temp_buffer, "%d\n", file_size);
				strcat(send_buffer, temp_buffer);
				strcat(send_buffer, "Connection: close\n");
				strcat(send_buffer, "Content-Type: text/html\n\n");
				// free the temp buffer
				free(temp_buffer);
				// make it big enough for the file
				temp_buffer = malloc(file_size);

				// read in all the bytes!
				fread(temp_buffer, 1, file_size, f);
				// close the file
				fclose(f);						

				// make enough room for what we have, plus the file
				big_send_buffer = malloc(file_size + strlen(send_buffer));
				strcat(big_send_buffer, send_buffer);
				strcat(big_send_buffer, temp_buffer);
				send_funct(big_send_buffer, connfd);
			}
			else 
			{
				strcat(send_buffer, "\nPlease follow the proper format:\n");
				strcat(send_buffer, "GET /FILENAME.html HTTP/1.1\n");
				strcat(send_buffer, "Closing connection.\n\n");
				send_funct(send_buffer, connfd);
			}	
		}
		else 
		{
			strcat(send_buffer, "\nPlease follow the proper format:\n");
			strcat(send_buffer, "GET /FILENAME.html HTTP/1.1\n");
			strcat(send_buffer, "Closing connection.\n\n");
			send_funct(send_buffer, connfd);
		}
	}
	
	// close connection
	close(connfd);
	
	pthread_mutex_lock(&mutex);
	write_stats(rcv_buffer);
	pthread_mutex_unlock(&mutex);

}

void send_funct(char *to_send, int connfd) {
	int amt = 0;

	// loop so everything sends
	while (amt < strlen(to_send)) {
		int ret = send(connfd, to_send + amt, strlen(to_send) - amt, 0);
		if (ret < 0) {
			perror("Send failed!\n");
			exit(EXIT_FAILURE);
		}
		amt += ret;
	}
}

// write the to_write buffer to the end of stats.txt
void write_stats(char *to_write) {
	FILE *f = fopen("stats.txt", "a+");

	if (f == NULL) 
	{
		printf("Error opening stats.txt\n");
		return;
	}

	fseek(f, 0, SEEK_END);
	fwrite(to_write, 1, strlen(to_write), f);
	fclose(f);
}