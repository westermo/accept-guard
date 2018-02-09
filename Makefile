include build.mk

CFLAGS   += -fPIC
OBJS      = accept-guard.o
LIBS      = -ldl
LIBNAME   = accept-guard.so

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	@printf "  LIB     $@\n"
	@$(CC) $(CFLAGS) -shared $(OBJS) -o $@ $(LIBS)

clean:
	@$(RM) $(LIBNAME) *.o

distclean: clean
	@$(RM) *~