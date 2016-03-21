bin_PROGRAMS = granota-gcc
pkglibexec_PROGRAMS = granota-as
granota_gcc_SOURCES = afl-gcc.c alloc-inl.h config.h debug.h types.h
granota_gcc_CPPFLAGS = -DAFL_PATH=\"$(pkglibexecdir)\" -DVERSION=\"$(PACKAGE_NAME)-$(PACKAGE_VERSION)\"
granota_as_SOURCES = afl-as.c afl-as.h alloc-inl.h config.h debug.h types.h
granota_as_CPPFLAGS = -DAFL_PATH=\"$(pkglibexecdir)\" -DVERSION=\"$(PACKAGE_NAME)-$(PACKAGE_VERSION)\"
install-exec-hook:
	cd $(pkglibexecdir) && ln -sf granota-as as
	cd $(bindir) && for i in g++ clang clang++; do ln -sf granota-gcc granota-$$i; done
uninstall-hook:
	cd $(pkglibexecdir) && rm -f as
	cd $(bindir) && for i in g++ clang clang++; do rm -f granota-$$i; done
