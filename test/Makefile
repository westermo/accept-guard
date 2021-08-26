EXEC   := client server

all: $(EXEC)

check: all
	unshare -mrun ./tcp.sh
	unshare -mrun ./udp.sh
	unshare -mrun ./ipv6.sh

clean:
	$(RM) $(EXEC) *.o

distclean: clean
	$(RM) *~
