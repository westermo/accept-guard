ChangeLog
=========

All notable changes to the project are documented in this file.


[v1.1][] - UNRELEASED
---------------------

### Changes
- Scope ACL environment variable with `ACCEPT_GUARD_` prefix to avoid
  clashing with other uses of ACL on the system.  Incompatible change!
- Simplify build system slightly
- Add basic IPv4 TCP test to verify accept guard, based on `unshare`
- Replace unsafe `strncpy()` with safer version that NUL terminates.
  In a world of systemd named interfaces we are always at `IFNAMSIZ`

### Fixes
- Check return value from `getenv()`, may be `NULL`


[v1.0][] - 2021-08-20
---------------------

First public release.  Basic `accept()` wrapper which reads allowed
interface:port tuples from an `ACL=iface:port;iface2:port` environment
variable.

[v1.1]: https://github.com/troglobit/smcroute/compare/v1.0...v1.1
[v1.0]: https://github.com/troglobit/smcroute/compare/v0.0...v1.0
