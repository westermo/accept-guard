EXEC   := client server

all: $(EXEC)

check: all
	unshare -mrun ./tcp.sh

clean:
	$(RM) $(EXEC) *.o

distclean: clean
	$(RM) *~
