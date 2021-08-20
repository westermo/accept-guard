#include <err.h>
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

void func(int sd)
{
	char buf[MAX];
	int n;

	n = read(sd, buf, sizeof(buf));
	if (n == -1) {
		warn("Failed reading from client socket");
		return;
	}

	buf[n] = 0;
	warnx("client query: %s", buf);

	snprintf(buf, sizeof(buf), "Welcome to the TCP Server!");
	n = write(sd, buf, sizeof(buf));
	if (n == -1)
		warn("Failed sending reply to client");
}
  
int main()
{
	struct sockaddr_in sin;
	int sd, client;
	socklen_t len;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1)
		err(1, "Failed creating socket");

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(PORT);
  
	if ((bind(sd, (struct sockaddr*)&sin, sizeof(sin))) == -1)
		err(1, "Failed binding socket to port *:%d", PORT);
  
	if ((listen(sd, 5)) == -1)
		err(1, "Failed setting listening socket");

	while (1) {
		len = sizeof(sin);
		client = accept(sd, (struct sockaddr*)&sin, &len);
		if (client < 0) {
			warn("Failed accepting client connection");
			continue;
		}
  
		func(client);
	}
  
	return close(sd);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
