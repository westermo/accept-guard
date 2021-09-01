VERSION   = 1.2
NAME      = accept-guard
PKG       = $(NAME)-$(VERSION)
ARCHIVE   = $(PKG).tar.gz

libdir    = $(prefix)/lib
docdir    = $(prefix)/share/doc/$(NAME)

RM        = rm -f
INSTALL   = install

CFLAGS   ?= -g -O2 -W -Wall -Wextra
CFLAGS   += -fPIC
OBJS      = accept-guard.o
LIBS      = -ldl
LIBNAME   = accept-guard.so
DOCFILES  = README.md LICENSE

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CC) $(CFLAGS) -shared $(OBJS) -o $@ $(LIBS)

check: all
	$(MAKE) -C test $@

install: $(LIBNAME)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -d $(DESTDIR)$(docdir)
	$(INSTALL) -m 0644 $(LIBNAME) $(DESTDIR)$(libdir)/$(LIBNAME)
	for file in $(DOCFILES); do					\
		$(INSTALL) -m 0644 $$file $(DESTDIR)$(docdir)/$$file;	\
	done

uninstall:
	-$(RM) $(DESTDIR)$(libdir)/$(LIBNAME)
	-$(RM) -r $(DESTDIR)$(docdir)

clean:
	$(MAKE) -C test $@
	-$(RM) $(LIBNAME) *.o

dist:
	git archive --format=tar.gz --prefix=$(PKG)/ -o ../$(ARCHIVE) v$(VERSION)

distclean: clean
	-$(RM) *~

release: dist
	(cd ..; md5sum $(ARCHIVE) > $(ARCHIVE).md5)
