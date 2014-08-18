AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CFLAGS = -Wall

noinst_PROGRAMS = sha socket

sha_SOURCES = test/sha.c

socket_SOURCES = test/socket.c

nobase_include_HEADERS = $(wildcard $(srcdir)/uclib/*.[ch])
