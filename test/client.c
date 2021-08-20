#include <err.h>
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

int func(int sd, char *addr)
{
	char buf[MAX];
	int n;

	snprintf(buf, sizeof(buf), "HELO %s, THIS IS CLIENT", addr);
	n = write(sd, buf, sizeof(buf));
	if (n == -1) {
		warn("Failed communicating with server at %s:%d", addr, PORT);
		return 1;
	}

	n = read(sd, buf, sizeof(buf));
	if (n <= 0) {
		usleep(10000);
		warnx("Failed reading response from server at %s:%d", addr, PORT);
		return 1;
	}

	buf[n] = 0;
	warnx("server replied: %s", buf);

	return 0;
}
  
int main(int argc, char *argv[])
{
	struct sockaddr_in sin = { 0 };
	char *addr = "127.0.0.1";
	int sd, rc;
  
	if (argc > 1)
		addr = argv[1];

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1)
		err(1, "Failed creating socket");

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(addr);
	sin.sin_port = htons(PORT);
  
	if (connect(sd, (struct sockaddr*)&sin, sizeof(sin)) == -1)
		err(1, "Failed connecting to server");
  
	rc = func(sd, addr);
	close(sd);

	return rc;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
