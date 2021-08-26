/* Simple TCP/UDP server to verify accept() and recvfrom() filtering */
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX 80
#define PORT 8080


int sock_creat(int type)
{
	int val = 1;
	int sd;

	sd = socket(AF_INET, type, 0);
	if (sd == -1)
		return -1;

	if (setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)))
		err(1, "Failed enabling SO_REUSEPORT");

	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)))
		err(1, "Failed enabling SO_REUSEADDR");

	return sd;
}

void tcp(int sd)
{
	struct sockaddr_in sin;
	socklen_t len;
	int client;

	if ((listen(sd, 5)) == -1)
		err(1, "Failed setting listening socket");

	while (1) {
		char buf[MAX];
		int n;

		warnx("waiting for client connection ...");

		len = sizeof(sin);
		client = accept(sd, (struct sockaddr*)&sin, &len);
		if (client < 0) {
			warn("Failed accepting client connection");
			continue;
		}
  
		n = read(client, buf, sizeof(buf));
		if (n == -1) {
			warn("Failed reading from client socket");
			continue;
		}

		buf[n] = 0;
		warnx("client query: %s", buf);

		snprintf(buf, sizeof(buf), "Welcome to the TCP Server!");
		n = write(client, buf, sizeof(buf));
		if (n == -1) {
			warn("Failed sending reply to client");
			continue;
		}
	}
}

void udp(int sd)
{
	struct sockaddr_in sin;
	socklen_t sinlen;
	char buf[MAX];
	ssize_t len;

	while (1) {
		sinlen = sizeof(sin);
		len = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr *)&sin, &sinlen);
		if (len == -1) {
			warn("Failed reading client request");
			continue;
		}

		buf[len] = 0;
		warnx("client query: %s", buf);

		snprintf(buf, sizeof(buf), "Welcome to the UDP Server!");
		len = sendto(sd, buf, strlen(buf), 0,  (struct sockaddr *)&sin, sizeof(sin));
		if (len == -1)
			warn("Failed sending reply to client");
	}
}

int main(int argc, char *argv[])
{
	struct sockaddr_in sin;
	int port = PORT;
	int type, sd, c;

	while ((c = getopt(argc, argv, "p:tu")) != EOF) {
		switch (c) {
		case 'p':
			port = atoi(optarg);
			break;

		case 't':
			sd = sock_creat(SOCK_STREAM);
			type = 1;
			break;

		case 'u':
			sd = sock_creat(SOCK_DGRAM);
			type = 0;
			break;

		default:
			errx(1, "Usage: %s [-tu] [-p PORT]", argv[0]);
		}
	}

	if (sd == -1)
		err(1, "Failed creating socket");

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
  
	warnx("Starting up, binding to *:%d", port);
	if ((bind(sd, (struct sockaddr*)&sin, sizeof(sin))) == -1)
		err(1, "Failed binding socket to port *:%d", PORT);
  
	if (type)
		tcp(sd);
	else
		udp(sd);
  
	return close(sd);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
