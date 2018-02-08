# accept-guard
Interface/port access control by wrapping the accept system call i Linux.

The accept guard wrapper will allow accesses based on a list of interfaces and ports.
It is preloaded using the LD_PRELOAD environment variable, and access is controlled by
the environment variable ACL.

ACL environment variable syntax:
<iface>:<port>[<port][;<iface>:<port>[<port];] .........

If interface is defined as 'any', access will be allow on all interfaces on the
listed ports. 
If used, it is expected to be the first and only interface entry for the same port,
since the accept call will be allowed based on the first matching rule.
 
An example script to start mongoose webserver and limit access to port 80 for interface
eth2, port 443 for interface eth3 and allow both ports on interface eth4:

#!/bin/sh
export LD_PRELOAD=/lib/access-guard.so
export ACL="eth2:80;eth3:443;eth4:80,443"
exec /bin/mongoose /etc/mongoose/config 
