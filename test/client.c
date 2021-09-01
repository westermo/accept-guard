/* Simple TCP/UDP client to verify accept() and recvfrom() filtering */
#include <err.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX 80
#define PORT "8080"

socklen_t sock_creat(int family, int type, char *addr, char *port, struct sockaddr *sa, socklen_t *salen)
{
	struct addrinfo hints = { 0 };
	struct addrinfo *result, *ai;
	int rc, sd = -1;

	hints.ai_family = family;
	hints.ai_socktype = type;
	hints.ai_flags = AI_NUMERICHOST;

	if ((rc = getaddrinfo(addr, port, &hints, &result)))
		errx(1, "Failed looking up %s:%s: %s", addr, port, gai_strerror(rc));

	for (ai = result; ai; ai = ai->ai_next) {
		if (ai->ai_socktype != type)
			continue;

		sd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sd == -1)
			continue;

		if (ai->ai_socktype == SOCK_STREAM) {
			if (connect(sd, ai->ai_addr, ai->ai_addrlen) == -1)
				err(1, "Failed connecting to server");
		}

		if (sa)
			*sa = *ai->ai_addr;
		if (salen)
			*salen = ai->ai_addrlen;
		break;
	}
	freeaddrinfo(result);

	if (sd == -1)
		err(1, "Failed creating socket");

	return sd;
}

void tcp(int family, char *addr, char *port)
{
	char buf[MAX];
	int sd, len;

	snprintf(buf, sizeof(buf), "HELO %s:%s, THIS IS CLIENT", addr, port);

	sd = sock_creat(family, SOCK_STREAM, addr, port, NULL, NULL);
	len = write(sd, buf, sizeof(buf));
	if (len == -1)
		err(1, "Failed communicating with server at %s:%s", addr, port);

	len = recv(sd, buf, sizeof(buf), 0);
	if (len <= 0) {
		usleep(10000);
		err(1, "Failed reading response from server at %s:%s", addr, port);
	}

	buf[len] = 0;
	warnx("server replied: %s", buf);
	close(sd);
}

void udp(int family, char *addr, char *port)
{
	struct timeval tv = { 1, 0 };
	struct sockaddr sa;
	socklen_t salen;
	char buf[MAX];
	int sd, len;

	sd = sock_creat(family, SOCK_DGRAM, addr, port, &sa, &salen);
	if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		err(1, "Failed setting SO_RCVTIMEO");

	snprintf(buf, sizeof(buf), "HELO %s, THIS IS CLIENT", addr);
	len = sendto(sd, buf, strlen(buf), 0, &sa, salen);
	if (len == -1)
		err(1, "Failed communicating with %s:%s", addr, port);

	warnx("Receiving on %d", sd);
	len = recvfrom(sd, buf, sizeof(buf), 0, &sa, &salen);
	if (len == -1)
		err(1, "Failed reading response from %s:%s", addr, port);

	buf[len] = 0;
	warnx("server replied: %s", buf);
	close(sd);
}

int main(int argc, char *argv[])
{
	char *addr = "127.0.0.1";
	int family = AF_INET;
	char *port = PORT;
	int type = 1;
	int c;
  
	while ((c = getopt(argc, argv, "6p:tu")) != EOF) {
		switch (c) {
		case '6':
			family = AF_INET6;
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
			errx(1, "Usage: %s [-6tu] [-p PORT]", argv[0]);
		}
	}

	if (optind < argc)
		addr = argv[optind];

	warnx("Connecting to server %s:%s ...", addr, port);
	if (type)
		tcp(family, addr, port);
	else
		udp(family, addr, port);

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
