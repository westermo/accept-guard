/* accept-guard.c - verifies accept() per interface:port for tcp connections
 *
 * Copyright (c) 2018 Thomas Eliasson <lajson@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
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

#define ACL_ENV     "ACCEPT_GUARD_ACL"
#define IFACE_ANY   "any"
#define MAX_IFACES  (64 * 4)
#define MAX_PORTS   2

struct acl {
	char iface[IF_NAMESIZE];
	int  ports[MAX_PORTS];
};

/* The access control list containing interfaces and
 * ports that are allowed access. */
static struct acl acl[MAX_IFACES];

static size_t strlencpy(char *dst, const char *src, size_t len)
{
	const char *p = src;
	size_t num = len;

	while (num > 0) {
		num--;
		if ((*dst++ = *src++) == 0)
			break;
	}

	if (num == 0 && len > 0)
		*dst = 0;

	return src - p - 1;
}

/*
 * Parse environment variable for a list of allowed interfaces and ports
 * Result is stored in global static variable @acl.
 * Only parsed if no interfaces registered, otherwise returns without action.
 */
static void parse_acl(void)
{
	char *record_pos, *field_pos, *port_pos;
	char *record, *iface, *ports, *port;
	char *env;
	int i = 0;
	int j;

	/* Only performed if list is empty. */
	if (acl[0].iface[0] != '\0')
		return;

	env = getenv(ACL_ENV);
	if (!env)
		return;

	record_pos = env;
	record = strtok_r(record_pos, "; ", &record_pos);
	while (record && i < MAX_IFACES) {
		field_pos = record;

		iface = strtok_r(field_pos, ": ", &field_pos);
		if (!iface)
			goto next;

		strlencpy(acl[i].iface, iface, sizeof(acl[i].iface));
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
	next:
		record = strtok_r(record_pos, "; ", &record_pos);
	}
}

/* Figure out in-bound interface and port from socket */
static int identify_inbound(int sd, int ifindex, char *ifname, size_t len, int *port)
{
	struct ifaddrs *ifaddr, *ifa;
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);

	if (ifindex) {
		if_indextoname(ifindex, ifname);
		return 0;
	}

	if (-1 == getsockname(sd, (struct sockaddr *)&sin, &slen))
		return -1;

	if (-1 == getifaddrs(&ifaddr))
		return -1;

	for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
		size_t ina_len = sizeof(struct in_addr);
		struct sockaddr_in *iin;

		if (!ifa->ifa_addr)
			continue;

		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;

		iin = (struct sockaddr_in *)ifa->ifa_addr;
		if (!memcmp(&sin.sin_addr, &iin->sin_addr, ina_len)) {
			strlencpy(ifname, ifa->ifa_name, len);
			break;
		}
	}
	freeifaddrs(ifaddr);

	*port = ntohs(sin.sin_port);

	return 0;
}

static int port_allowed(struct acl *entry, int port)
{
	int i;

	for (i = 0; i < MAX_PORTS; i++) {
		if (entry->ports[i] == port)
			return 1;
	}

	return 0;
}

static int iface_allowed(int sd, int ifindex)
{
	char ifname[IF_NAMESIZE] = "UNKNOWN";
	int port = 0;
	int i;

	/* If incoming interface cannot be identified, deny access. */
	if (identify_inbound(sd, ifindex, ifname, sizeof(ifname), &port))
		return 0;

	for (i = 0; i < MAX_IFACES; i++) {
		/* If reached last item => deny access */
		if (acl[i].iface[0] == '\0')
			return 0;

		if (!strncmp(ifname, acl[i].iface, IF_NAMESIZE) ||
		    !strncmp(IFACE_ANY, acl[i].iface, IF_NAMESIZE))
			return port_allowed(&acl[i], port);
	}

	return 0;
}

int accept(int socket, struct sockaddr *addr, socklen_t *length_ptr)
{
	int (*org_accept)(int socket, struct sockaddr *addr, socklen_t *length_ptr);
	int result;

	/* Parse configuration from environment variable. */
	parse_acl();

	org_accept = dlsym(RTLD_NEXT, "accept");
	result = org_accept(socket, addr, length_ptr);
	if (result > 0) {
		if (!iface_allowed(result, 0)) {
			shutdown(result, SHUT_RDWR);
			close(result);

			/* Set as not valid socket, since it's not valid for access. */
			errno = EBADF;
			return -1;
		}
	}

	return result;
}

/* Peek into socket to figure out where an inbound packet comes from */
static int peek_ifindex(int sd)
{
	static char cmbuf[0x100];
	struct msghdr msgh;
	struct cmsghdr *cmsg;
	struct sockaddr_in sin;
	int on = 1;

	setsockopt(sd, SOL_IP, IP_PKTINFO, &on, sizeof(on));

	memset(&msgh, 0, sizeof(msgh));
	msgh.msg_name = &sin;
	msgh.msg_namelen = sizeof(sin);
	msgh.msg_control = cmbuf;
	msgh.msg_controllen = sizeof(cmbuf);

	if (recvmsg(sd, &msgh, MSG_PEEK) < 0)
		return 0;

	for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg; cmsg = CMSG_NXTHDR(&msgh, cmsg)) {
		struct in_pktinfo *ipi = (struct in_pktinfo *)CMSG_DATA(cmsg);

		if (cmsg->cmsg_level != SOL_IP || cmsg->cmsg_type != IP_PKTINFO)
			continue;

		return ipi->ipi_ifindex;
	}

	return 0;
}

ssize_t recvfrom(int sd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen)
{
	ssize_t (*org_recvfrom)(int, void *, size_t, int, struct sockaddr *, socklen_t *);
	int ifindex, rc;

	/* Parse configuration from environment variable. */
	parse_acl();

	ifindex = peek_ifindex(sd);

	org_recvfrom = dlsym(RTLD_NEXT, "recvfrom");
	rc = org_recvfrom(sd, buf, len, flags, addr, addrlen);
	if (rc > 0) {
		if (!iface_allowed(sd, ifindex)) {
			shutdown(rc, SHUT_RDWR);
			close(rc);
			/* Set as not valid socket, since it's not valid for access. */
			errno = EBADF;
			return -1;
		}
	}

	return rc;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
