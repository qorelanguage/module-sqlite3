#if COND_SINGLE_COMPILATION_UNIT
#PGSQL_SOURCES = single-compilation-unit.cc
#else
SQLITE3_SOURCES = sqlite3connection.cc sqlite3executor.cc sqlite3module.cc
#endif

lib_LTLIBRARIES = sqlite3.la
sqlite3_la_SOURCES = $(SQLITE3_SOURCES)
sqlite3_la_LDFLAGS = -module -avoid-version ${SQLITE3_LIBS}

INCLUDES = -I$(top_srcdir)/include $(SQLITE3_INCLUDES)

AM_CPPFLAGS=$(QORE_CPPFLAGS)

if COND_PROFILE
AM_CXXFLAGS=-g -pg
else
if COND_DEBUG
AM_CXXFLAGS=-g
endif
endif

install-exec-hook:
	mv $(DESTDIR)$(libdir)/sqlite3.$(MODULE_SUFFIX) $(DESTDIR)$(libdir)/sqlite3-api-$(MODULE_API).qmod
	rm -f $(DESTDIR)$(libdir)/sqlite3.la $(DESTDIR)$(libdir)/sqlite3.a


