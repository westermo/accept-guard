# accept-guard

Service access control by wrapping the Linux `accept()` and `recvfrom()`
system calls, for TCP and UDP respectively.

The accept guard wrapper allows access to services based on a list of
interfaces and ports.  It is loaded using the `LD_PRELOAD` environment
variable, controlled by the environment variable `ACCEPT_GUARD_ACL`

## Syntax

```
<rule>[;<rule>]
where rule obeys:
<interface>:<port>[,<port>]...
```

- When interface is set to `any`, access is allowed on all interfaces
  for the given Internet ports
- First matching rule allows access, i.e., it is expected that the first
  and only interface entry for the same port

## TCP Example

An example script to start mongoose webserver and limit access to port
80 for interface `eth2`, port 443 for interface `eth3` and allow both
ports on interface `eth4`:

```
#!/bin/sh
export LD_PRELOAD=/lib/access-guard.so
export ACCEPT_GUARD_ACL="eth2:80;eth3:443;eth4:80,443"
exec /usr/sbin/mongoose /etc/mongoose/config
```

## UDP Example

An example script to start Net-SNMP, limiting access to SNMP for
interface `eth2`, and allow both SNMP and SNMP traps on interface
`eth4`:

```
#!/bin/sh
export LD_PRELOAD=/lib/access-guard.so
export ACCEPT_GUARD_ACL="eth2:161;eth4:161,162"
exec /usr/sbin/snmpd -a -LS0-3d -c /etc/snmpd.conf -p /run/snmpd.pid
```
