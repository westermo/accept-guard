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
#define PORT "8080"

static int use_recvmsg = 0;


int sock_creat(int family, int type, char *port)
{
	struct addrinfo hints = { 0 };
	struct addrinfo *result, *ai;
	int rc, sd = -1;

	hints.ai_family = family;
	hints.ai_socktype = type;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

	if ((rc = getaddrinfo(NULL, port, &hints, &result)))
		errx(1, "Failed looking up *:%s: %s", port, gai_strerror(rc));

	for (ai = result; ai; ai = ai->ai_next) {
		int val = 1;

		if (ai->ai_socktype != type)
			continue;

		sd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sd == -1)
			continue;

		if (setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)))
			err(1, "Failed enabling SO_REUSEPORT");

		if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)))
			err(1, "Failed enabling SO_REUSEADDR");

		warnx("Starting up, binding to *:%s", port);
		if ((bind(sd, ai->ai_addr, ai->ai_addrlen)) == -1)
			err(1, "Failed binding socket to port *:%s", port);
	}
	freeaddrinfo(result);

	if (sd == -1)
		err(1, "Failed creating socket");

	return sd;
}

void tcp(int family, char *port)
{
	struct sockaddr sa;
	socklen_t salen;
	int client, sd;

	sd = sock_creat(family, SOCK_STREAM, port);
	if ((listen(sd, 5)) == -1)
		err(1, "Failed setting listening socket");

	while (1) {
		char buf[MAX];
		int n;

		warnx("waiting for client connection ...");

		salen = sizeof(sa);
		client = accept(sd, &sa, &salen);
		if (client < 0) {
			warn("Failed accepting client connection");
			continue;
		}
  
		n = recv(client, buf, sizeof(buf), 0);
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

/*
 * Depending on the mode of operation we either call recvfrom() or
 * recvmsg() to test different API wrappers in accept-guard.so
 */
ssize_t udp_recv(int sd, char *buf, size_t len, struct sockaddr *sa, socklen_t *salen)
{
	struct iovec iov[1] = { { buf, len} };
	struct msghdr msg;

	if (!use_recvmsg)
		return recvfrom(sd, buf, len, 0, sa, salen);

	msg.msg_name       = sa;
	msg.msg_namelen    = *salen;
	msg.msg_iov        = iov;
	msg.msg_iovlen     = 1;
	msg.msg_control    = NULL;
	msg.msg_controllen = 0;

	return recvmsg(sd, &msg, 0);
}

void udp(int family, char *port)
{
	struct sockaddr sa;
	socklen_t salen;
	char buf[MAX];
	ssize_t len;
	int sd;

	sd = sock_creat(family, SOCK_DGRAM, port);

	while (1) {
		salen = sizeof(sa);
		len = udp_recv(sd, buf, sizeof(buf), &sa, &salen);
		if (len == -1) {
			warn("Failed reading client request");
			continue;
		}

		buf[len] = 0;
		warnx("client query: %s", buf);

		snprintf(buf, sizeof(buf), "Welcome to the UDP Server!");
		len = sendto(sd, buf, strlen(buf), 0, &sa, sizeof(sa));
		if (len == -1)
			warn("Failed sending reply to client");
	}
}

int main(int argc, char *argv[])
{
	int family = AF_INET;
	char *port = PORT;
	int type = 0;
	int c;

	while ((c = getopt(argc, argv, "6mp:tu")) != EOF) {
		switch (c) {
		case '6':
			family = AF_INET6;
			break;

		case 'm':
			use_recvmsg = 1;
			break;

		case 'p':
			port = optarg;
			break;

		case 't':
			type = 1;
			break;

		case 'u':
			type = 0;
			break;

		default:
			errx(1, "Usage: %s [-tu] [-p PORT]", argv[0]);
		}
	}

	if (type)
		tcp(family, port);
	else
		udp(family, port);
  
	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
