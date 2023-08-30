# golos version
VERSION = 0.1

# paths
PREFIX = /usr/local

# includes and libs
INCS = `pkg-config --cflags rnnoise`
LIBS = -lm -lpthread `pkg-config --libs rnnoise`

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700L -DVERSION=\"${VERSION}\"
CFLAGS   = -g -std=c99 -pedantic -Wall -Wextra -Og ${INCS} ${CPPFLAGS}
# CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Wno-deprecated-declarations -O3 ${INCS} ${CPPFLAGS}
LDFLAGS  = ${LIBS}

# compiler and linker
CC = gcc
