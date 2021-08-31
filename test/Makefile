EXEC   := client server
TESTS  := accept.sh accept6.sh recvfrom.sh recvmsg.sh

all: $(EXEC)

check: all
	@./runner.sh $(TESTS)

clean:
	$(RM) $(EXEC) *.o

distclean: clean
	$(RM) *~
