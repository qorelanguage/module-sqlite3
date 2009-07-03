AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

noinst_HEADERS = \
    src/sqlite3connection.h \
    src/sqlite3executor.h \
    src/sqlite3module.h

EXTRA_DIST = \
    COPYING \
    ChangeLog \
    AUTHORS \
    README \
    RELEASE-NOTES \
    test/sqlite3test-basic.q \
    test/sqlite3test-threading.q \
    test/blob.png \
    docs/sqlite3-module-doc.html \
    qore-sqlite3-module.spec

ACLOCAL_AMFLAGS=-I m4

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
