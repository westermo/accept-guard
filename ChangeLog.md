ChangeLog
=========

All notable changes to the project are documented in this file.


[v1.4][] - 2021-09-06
---------------------

### Changes
- Allow access if `getsockaname()` or `getifaddrs()` fails
- Use `SO_DOMAIN` socket option to query for `AF_INET` and `AF_INET6`
  domain sockets.  These are the only ones we are concerned with, let
  everything else pass through


[v1.3][] - 2021-09-03
---------------------

### Fixes
- Fixes for wrapping Net-SNMP and other services that use `AF_UNIX` IPC


[v1.2][] - 2021-09-01
---------------------

### Changes
- Add support for wrapping `recvmsg()` and `recv()` syscalls, in
  addition to the existing `recvfrom()` wrapper
- Slightly improved test framework, with .log files and overview
- Only check ACL if `accept()` doesn't return error

### Fixes
- Fix markdown links in changelog diffs
- Fix uninitialized variable in test server

[v1.1][] - 2021-08-26
---------------------

### Changes
- Add support for UDP services by wrapping `recvfrom()`
- Add support for IPv6
- Scope ACL environment variable with `ACCEPT_GUARD_` prefix to avoid
  clashing with other uses of ACL on the system.  Incompatible change!
- Simplify build system slightly
- Add basic test suite to verify accept guard, based on `unshare`
- Replace unsafe `strncpy()` with safer version that NUL terminates.
  In a world of systemd named interfaces we are always at `IFNAMSIZ`

### Fixes
- Check return value from `getenv()`, may be `NULL`


[v1.0][] - 2021-08-20
---------------------

First public release.  Basic `accept()` wrapper which reads allowed
interface:port tuples from an `ACL=iface:port;iface2:port` environment
variable.

[v1.4]: https://github.com/westermo/accept-guard/compare/v1.3...v1.4
[v1.3]: https://github.com/westermo/accept-guard/compare/v1.2...v1.3
[v1.2]: https://github.com/westermo/accept-guard/compare/v1.1...v1.2
[v1.1]: https://github.com/westermo/accept-guard/compare/v1.0...v1.1
[v1.0]: https://github.com/westermo/accept-guard/compare/v0.0...v1.0
