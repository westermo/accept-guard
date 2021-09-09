EXEC   := client server
CFLAGS := -W -Wall -Wextra -std=gnu99 -g -O2
TESTS  := accept.sh accept6.sh accept6-ipv4-mapped.sh recvfrom.sh recvmsg.sh

all: $(EXEC)

check: all
	@./runner.sh $(TESTS)

clean:
	$(RM) $(EXEC) *.o

distclean: clean
	$(RM) *~
