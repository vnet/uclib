AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CFLAGS = -Wall

noinst_PROGRAMS = fheap serialize sha socket websocket

fheap_SOURCES = test/fheap.c
sha_SOURCES = test/sha.c
socket_SOURCES = test/socket.c
websocket_SOURCES = test/websocket.c
serialize_SOURCES = test/serialize.c
LDADD = libuclib.a

lib_LIBRARIES = libuclib.a

libuclib_a_SOURCES = uclib/uclib.c

nobase_include_HEADERS = $(wildcard $(srcdir)/uclib/*.h)
