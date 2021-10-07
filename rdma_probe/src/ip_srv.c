#include<netdb.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include "ip_srv.h"

int check_add_port(char **service,int port,
		const char *servername,
		struct addrinfo *hints,
		struct addrinfo **res)
{
	int number;
	const int FAILURE = -1, SUCCESS=0;

	if (asprintf(service,"%d", port) < 0) {
		return FAILURE;
	}

	number = getaddrinfo(servername,*service,hints,res);

	if (number < 0) {
		fprintf(stderr, "%s for %s:%d\n", gai_strerror(number), servername, port);
		return FAILURE;
	}

	return SUCCESS;
}

int ip_client_connect(struct ip_srv_conf* comm)
{
	struct addrinfo *res, *t;
	struct addrinfo hints;
	char *service;

	int sockfd = -1;
	memset(&hints, 0, sizeof hints);
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (check_add_port(&service,comm->port,comm->servername,&hints,&res)) {
		fprintf(stderr, "Problem in resolving basic address and port\n");
		return -1;
	}

	for (t = res; t; t = t->ai_next) {
		sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);

		if (sockfd >= 0) {
			if (!connect(sockfd, t->ai_addr, t->ai_addrlen))
				break;
			close(sockfd);
			sockfd = -1;
		}
	}

	freeaddrinfo(res);

	if (sockfd < 0) {
		fprintf(stderr, "Couldn't connect to %s:%d\n",comm->servername,comm->port);
		return -1;
	}

	comm->sockfd = sockfd;
	return 0;
}

int ip_server_connect(struct ip_srv_conf* comm)
{
	struct addrinfo *res, *t;
	struct addrinfo hints;
	char *service;
	int n;

	int sockfd = -1, connfd;
	memset(&hints, 0, sizeof hints);
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (check_add_port(&service,comm->port,NULL,&hints,&res)) {
		fprintf(stderr, "Problem in resolving basic address and port\n");
		return -1;
	}

	for (t = res; t; t = t->ai_next) {
		sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);

		if (sockfd >= 0) {
			n = 1;
			setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof n);
			if (!bind(sockfd, t->ai_addr, t->ai_addrlen))
				break;
			close(sockfd);
			sockfd = -1;
		}
	}
	freeaddrinfo(res);

	if (sockfd < 0) {
		fprintf(stderr, "Couldn't listen to port %d\n", comm->port);
		return 1;
	}

	listen(sockfd, 1);
	printf("Listening on port %d\n", comm->port);
	connfd = accept(sockfd, NULL, 0);

	if (connfd < 0) {
		fprintf(stderr, "accept() failed\n");
		close(sockfd);
		return -1;
	}
	close(sockfd);
	comm->sockfd = connfd;
	return 0;
}