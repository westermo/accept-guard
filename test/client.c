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
#define PORT 8080

void tcp(int sd, char *addr, int port)
{
	struct sockaddr_in sin = { 0 };
	char buf[MAX];
	int n;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(addr);
	sin.sin_port = htons(port);

	if (connect(sd, (struct sockaddr*)&sin, sizeof(sin)) == -1)
		err(1, "Failed connecting to server");

	snprintf(buf, sizeof(buf), "HELO %s, THIS IS CLIENT", addr);
	n = write(sd, buf, sizeof(buf));
	if (n == -1)
		err(1, "Failed communicating with server at %s:%d", addr, port);

	n = read(sd, buf, sizeof(buf));
	if (n <= 0) {
		usleep(10000);
		err(1, "Failed reading response from server at %s:%d", addr, port);
	}

	buf[n] = 0;
	warnx("server replied: %s", buf);
}

void udp(int sd, char *addr, int port)
{
	struct sockaddr_in sin = { 0 };
	struct timeval tv = { 1, 0 };
	socklen_t sinlen;
	char buf[MAX];
	int len;

	if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		err(1, "Failed setting SO_RCVTIMEO");

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(addr);
	sin.sin_port = htons(port);

	snprintf(buf, sizeof(buf), "HELO %s, THIS IS CLIENT", addr);
	warnx("Sending on %d: '%s'", sd, buf);
	len = sendto(sd, buf, strlen(buf), 0,  (struct sockaddr *)&sin, sizeof(sin));
	if (len == -1)
		err(1, "Failed communicating with %s:%d", addr, port);

	sinlen = sizeof(sin);
	warnx("Receiving on %d", sd);
	len = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr *)&sin, &sinlen);
	if (len == -1)
		err(1, "Failed reading response from %s:%d", addr, port);

	buf[len] = 0;
	warnx("server replied: %s", buf);
}

int main(int argc, char *argv[])
{
	char *addr = "127.0.0.1";
	int port = PORT;
	int type = 1;
	int sd, c;
  
	while ((c = getopt(argc, argv, "p:tu")) != EOF) {
		switch (c) {
		case 'p':
			port = atoi(optarg);
			break;

		case 't':
			sd = socket(AF_INET, SOCK_STREAM, 0);
			type = 1;
			break;

		case 'u':
			sd = socket(AF_INET, SOCK_DGRAM, 0);
			type = 0;
			break;

		default:
			errx(1, "Usage: %s [-tu] [-p PORT]", argv[0]);
		}
	}

	if (optind < argc)
		addr = argv[optind];

	if (sd == -1)
		err(1, "Failed creating socket");

	warnx("Connecting to server %s:%d ...", addr, port);
	if (type)
		tcp(sd, addr, port);
	else
		udp(sd, addr, port);

	return close(sd);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
