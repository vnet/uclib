AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CFLAGS = -Wall

noinst_PROGRAMS = sha socket websocket

sha_SOURCES = test/sha.c
socket_SOURCES = test/socket.c
websocket_SOURCES = test/websocket.c

nobase_include_HEADERS = $(wildcard $(srcdir)/uclib/*.[ch])
