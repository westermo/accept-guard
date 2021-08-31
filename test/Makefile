EXEC   := client server

all: $(EXEC)

check: all
	unshare -mrun ./accept.sh
	unshare -mrun ./recvfrom.sh
	unshare -mrun ./accept6.sh

clean:
	$(RM) $(EXEC) *.o

distclean: clean
	$(RM) *~
