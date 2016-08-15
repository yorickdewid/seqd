# Sequence Hole punching

Use an UDP character sequence to temporarily open a firewall port. `seqd` can be an alternative to port knocking when certain TCP ports are blocked.

Prerequisites:
 * Running firewall allowing established, related connection streams.
 * Open UDP on designated port

Root permissions are required when binding to ports below 1024.

## Usage

```
seqd [-46q] [-p port] [-s seq] [-u command] [-d command] [-t sec]

  -4         Bind IPv4 only
  -6         Bind IPv6 only
  -q         Quiet
  -p port    Listen on port
  -s seq     Sequence to match
  -u command Run command when sequence is matched
  -d command Run command when timeout passed
  -t sec     Timeout
```

## Build

To build `seqd` just run `make` in the repository.

Compatible with:
 * FreeBSD
 * OpenBSD

License
----

GPL

