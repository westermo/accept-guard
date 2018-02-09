/*
 * The accept guard wrapper will allow access based on the environment variable
 * @ACL_ENV, where allowed interfaces and ports are listed.

 * MIT License
 *
 * Copyright (c) 2018 Thomas Eliasson <lajson@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>

#define ACL_ENV "ACL"
#define IFACE_ANY "any"
#define MAX_IFACES 64 * 4
#define MAX_PORTS 2

typedef struct {
	char iface[IF_NAMESIZE];
	int ports[MAX_PORTS];
} access_entry;

/* The access control list containing interfaces and
 * ports that are allowed access. */
static access_entry acl[MAX_IFACES];

/*
 * Parse environment variable for a list of allowed interfaces and ports
 * Result is stored in global static variable @acl.
 * Only parsed if no interfaces registered, otherwise returns without action.
 */
static void parse_acl(void)
{
	int i = 0;
	int j;
	char *record, *iface, *ports, *port;
	char *record_pos, *field_pos, *port_pos;

	/* Only performed if list is empty. */
	if (acl[0].iface[0] != '\0')
		return;
	char *env = getenv(ACL_ENV);

	record_pos = env;
	record = strtok_r(record_pos, "; ", &record_pos);
	while (record && i < MAX_IFACES) {
		field_pos = record;
		iface = strtok_r(field_pos, ": ", &field_pos);
		if (iface) {
			strncpy(acl[i].iface, iface, IF_NAMESIZE);
			ports = strtok_r(field_pos, ": ", &field_pos);

			if (ports) {
				port_pos = ports;
				port = strtok_r(port_pos, ", ", &port_pos);
				j = 0;
				while (port && j < MAX_PORTS) {
					/*
					 * If strtol fails, 0 will be set.
					 * Port 0 is never used, so no problem
					 */
					acl[i].ports[j] = strtol(port, NULL, 10);
					port = strtok_r(port_pos, ", ", &port_pos);
					j++;
				}
			}
			i++;
		}
		record = strtok_r(record_pos, "; ", &record_pos);
	}
	return;
}

/* Figure out in-bound interface and port from socket */
static int identify_inbound(int sd, char *ifname, int *port)
{
	struct ifaddrs *ifaddr, *ifa;
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);

	if (-1 == getsockname(sd, (struct sockaddr *)&sin, &len))
		return -1;

	*port = ntohs(sin.sin_port);

	if (-1 == getifaddrs(&ifaddr))
		return -1;

	for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
		size_t len = sizeof(struct in_addr);
		struct sockaddr_in *iin;

		if (!ifa->ifa_addr)
			continue;

		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;

		iin = (struct sockaddr_in *)ifa->ifa_addr;
		if (!memcmp(&sin.sin_addr, &iin->sin_addr, len)) {
			strncpy(ifname, ifa->ifa_name, IF_NAMESIZE);
			break;
		}
	}
	freeifaddrs(ifaddr);
	return 0;
}

static int port_allowed(access_entry * entry, int port)
{
	int j;

	for (j = 0; j < MAX_PORTS; j++) {
		if (entry->ports[j] == port) {
			return 1;
		}
	}
	return 0;
}

static int iface_allowed(int sd)
{
	char ifname[IF_NAMESIZE] = "UNKNOWN";
	int i;
	int port = 0;

	/* If incoming interface cannot be identified, deny access. */
	if (identify_inbound(sd, ifname, &port))
		return 0;

	for (i = 0; i < MAX_IFACES; i++) {
		/* If reached last item => deny access */
		if (acl[i].iface[0] == '\0')
			return 0;
		if (!strncmp(ifname, acl[i].iface, IF_NAMESIZE) || !strncmp(IFACE_ANY, acl[i].iface, IF_NAMESIZE)) {
			return port_allowed(&acl[i], port);
		}
	}
	return 0;
}

int accept(int socket, struct sockaddr *addr, socklen_t * length_ptr)
{
	int (*org_accept) (int socket, struct sockaddr * addr, socklen_t * length_ptr);
	int result;

	/* Parse configuration from environment variable. */
	parse_acl();

	org_accept = dlsym(RTLD_NEXT, "accept");
	result = org_accept(socket, addr, length_ptr);
	if (result > 0) {
		if (!iface_allowed(result)) {
			shutdown(result, SHUT_RDWR);
			close(result);
			/* Set as not valid socket, since it's not valid for access. */
			errno = EBADF;
			return -1;
		}
	}
	return result;
}
