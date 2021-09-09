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

static struct acl acl[MAX_IFACES];

static int     (*org_accept)   (int, struct sockaddr *, socklen_t *);
static ssize_t (*org_recv)     (int, void *, size_t, int);
static ssize_t (*org_recvfrom) (int, void *, size_t, int, struct sockaddr *, socklen_t *);
static ssize_t (*org_recvmsg)  (int, struct msghdr *, int);


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
#ifdef AF_INET6
	struct sockaddr_storage ss = { 0 };
#else
	struct sockaddr_in ss = { 0 };
#endif
	socklen_t slen = sizeof(ss);

	if (ifindex) {
		if_indextoname(ifindex, ifname);
		return 0;
	}

	if (-1 == getsockname(sd, (struct sockaddr *)&ss, &slen))
		return -1;

	if (-1 == getifaddrs(&ifaddr))
		return -1;

	for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr)
			continue;

#ifdef AF_INET6
		/* Detect and handle IPv4-mapped-on-IPv6 */
		if (ifa->ifa_addr->sa_family == AF_INET && ss.ss_family == AF_INET6) {
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&ss;
			size_t ina_len = sizeof(struct in_addr);
			size_t ina_offset = sizeof(struct in6_addr) - sizeof(struct in_addr);
			struct sockaddr_in *iin;

			if (!IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr))
				continue;

			iin = (struct sockaddr_in *)ifa->ifa_addr;
			if (!memcmp((unsigned char *)&sin6->sin6_addr + ina_offset,
					&iin->sin_addr, ina_len)) {
				strlencpy(ifname, ifa->ifa_name, len);
				*port = ntohs(sin6->sin6_port);
				break;
			}
		}
#endif

		if (ifa->ifa_addr->sa_family != ss.ss_family)
			continue;

		if (ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in *sin = (struct sockaddr_in *)&ss;
			size_t ina_len = sizeof(struct in_addr);
			struct sockaddr_in *iin;

			iin = (struct sockaddr_in *)ifa->ifa_addr;
			if (!memcmp(&sin->sin_addr, &iin->sin_addr, ina_len)) {
				strlencpy(ifname, ifa->ifa_name, len);
				*port = ntohs(sin->sin_port);
				break;
			}
		}
#ifdef AF_INET6
		else if (ifa->ifa_addr->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&ss;
			size_t ina_len = sizeof(struct in6_addr);
			struct sockaddr_in6 *iin;

			iin = (struct sockaddr_in6 *)ifa->ifa_addr;
			if (!memcmp(&sin6->sin6_addr, &iin->sin6_addr, ina_len)) {
				strlencpy(ifname, ifa->ifa_name, len);
				*port = ntohs(sin6->sin6_port);
				break;
			}
		}
#endif
	}
	freeifaddrs(ifaddr);

	return 0;
}

static int port_allowed(struct acl *entry, int port)
{
	int i;

	if (port == 0)
		return 1;	/* no port, local IPC traffic */

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

	/*
	 * If incoming interface cannot be identified, allow access.
	 * Possibly an AF_UNIX socket or other local access.
	 */
	if (identify_inbound(sd, ifindex, ifname, sizeof(ifname), &port))
		return 1;

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

static int is_inet_domain(int sd)
{
	socklen_t len;
	int val;

	len = sizeof(val);
	if (getsockopt(sd, SOL_SOCKET, SO_DOMAIN, &val, &len) == -1)
		return 0;      /* Fall back to allow syscall on error */

	if (val == AF_INET)
		return 1;
#ifdef AF_INET6
	if (val == AF_INET6)
		return 1;
#endif
	return 0;		/* Possibly AF_UNIX socket, allow */
}

static int is_sock_stream(int sd)
{
	socklen_t len;
	int val;

	len = sizeof(val);
	if (getsockopt(sd, SOL_SOCKET, SO_TYPE, &val, &len) == -1)
		return 1;	/* Fall back to allow syscall on error */

	if (val == SOCK_STREAM)
		return 1;

	return 0;
}

int accept(int socket, struct sockaddr *addr, socklen_t *addrlen)
{
	int rc;

	org_accept = dlsym(RTLD_NEXT, "accept");

	rc = org_accept(socket, addr, addrlen);
	if (rc != -1 && is_inet_domain(socket)) {

		parse_acl();

		if (!iface_allowed(rc, 0)) {
			shutdown(rc, SHUT_RDWR);
			close(rc);

			/* Set as not valid socket, since it's not valid for access. */
			errno = EBADF;
			return -1;
		}
	}

	return rc;
}

/* Peek into socket to figure out where an inbound packet comes from */
static int peek_ifindex(int sd)
{
	static char cmbuf[0x100];
	struct sockaddr_in sin;
	struct cmsghdr *cmsg;
	struct msghdr msgh;
	socklen_t orig_len;
	int orig_on = 0;
	int on = 1;

	orig_len = sizeof(orig_on);
	if (getsockopt(sd, SOL_IP, IP_PKTINFO, &orig_on, &orig_len) == -1)
		return 0;	/* Fall back to allow syscall on error */
	if (setsockopt(sd, SOL_IP, IP_PKTINFO, &on, sizeof(on)) == -1)
		return 0;	/* Fall back to allow syscall on error */

	memset(&msgh, 0, sizeof(msgh));
	msgh.msg_name = &sin;
	msgh.msg_namelen = sizeof(sin);
	msgh.msg_control = cmbuf;
	msgh.msg_controllen = sizeof(cmbuf);


	if (org_recvmsg(sd, &msgh, MSG_PEEK) == -1) {
		(void)setsockopt(sd, SOL_IP, IP_PKTINFO, &orig_on, sizeof(orig_on));
		return 0;
	}

	(void)setsockopt(sd, SOL_IP, IP_PKTINFO, &orig_on, sizeof(orig_on));
	for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg; cmsg = CMSG_NXTHDR(&msgh, cmsg)) {
		struct in_pktinfo *ipi = (struct in_pktinfo *)CMSG_DATA(cmsg);

		if (cmsg->cmsg_level != SOL_IP || cmsg->cmsg_type != IP_PKTINFO)
			continue;

		return ipi->ipi_ifindex;
	}

	return 0;
}

static ssize_t do_recv(int sd, int rc, int flags, int ifindex)
{
	if (rc == -1 || (flags & MSG_PEEK) || ifindex == 0 || !is_inet_domain(sd) || is_sock_stream(sd))
		goto done;

	parse_acl();

	if (!iface_allowed(sd, ifindex)) {
		errno = ERESTART;
		return -1;
	}
done:
	return rc;
}

ssize_t recv(int sd, void *buf, size_t len, int flags)
{
	int ifindex;

	org_recv = dlsym(RTLD_NEXT, "recv");
	org_recvfrom = dlsym(RTLD_NEXT, "recvfrom");
	org_recvmsg = dlsym(RTLD_NEXT, "recvmsg");

	ifindex = peek_ifindex(sd);

	return do_recv(sd, org_recv(sd, buf, len, flags), flags, ifindex);
}

ssize_t recvfrom(int sd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen)
{
	int ifindex;

	org_recvfrom = dlsym(RTLD_NEXT, "recvfrom");
	org_recvmsg = dlsym(RTLD_NEXT, "recvmsg");

	ifindex = peek_ifindex(sd);

	return do_recv(sd, org_recvfrom(sd, buf, len, flags, addr, addrlen), flags, ifindex);
}

ssize_t recvmsg(int sd, struct msghdr *msg, int flags)
{
	int ifindex;

	org_recvmsg = dlsym(RTLD_NEXT, "recvmsg");

	/* Try to peek ifindex from socket, on fail allow */
	ifindex = peek_ifindex(sd);

	return do_recv(sd, org_recvmsg(sd, msg, flags), flags, ifindex);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
