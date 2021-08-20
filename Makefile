include build.mk
VERSION   = 1.0
NAME      = accept-guard
PKG       = $(NAME)-$(VERSION)
ARCHIVE   = $(PKG).tar.gz

libdir    = $(prefix)/lib
docdir    = $(prefix)/share/doc/$(NAME)

RM        = rm -f
INSTALL   = install

CFLAGS   += -fPIC
OBJS      = accept-guard.o
LIBS      = -ldl
LIBNAME   = accept-guard.so
DOCFILES  = README.md LICENSE

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	@printf "  LIB     $@\n"
	@$(CC) $(CFLAGS) -shared $(OBJS) -o $@ $(LIBS)

install: $(LIBNAME)
	@printf "  INSTALL $(LIBNAME) to $(DESTDIR)\n"
	@$(INSTALL) -d $(DESTDIR)$(libdir)
	@$(INSTALL) -d $(DESTDIR)$(docdir)
	@$(INSTALL) -m 0644 $(LIBNAME) $(DESTDIR)$(libdir)/$(LIBNAME)
	@for file in $(DOCFILES); do					\
		$(INSTALL) -m 0644 $$file $(DESTDIR)$(docdir)/$$file;	\
	done

uninstall:
	@printf "  UNINST  $(LIBNAME) from $(DESTDIR)\n"
	-@$(RM) $(DESTDIR)$(libdir)/$(LIBNAME)
	-@$(RM) -r $(DESTDIR)$(docdir)

clean:
	@$(RM) $(LIBNAME) *.o

dist:
	@git archive --format=tar.gz --prefix=$(PKG)/ -o ../$(ARCHIVE) v$(VERSION)

distclean: clean
	@$(RM) *~

release: dist
	@printf "  RELEASE ../$(ARCHIVE)\n"
	@(cd ..; md5sum $(ARCHIVE) > $(ARCHIVE).md5)
