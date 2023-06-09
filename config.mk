# golos version
VERSION = 0.1

# paths
PREFIX = /usr/local

# includes and libs
INCS = 
LIBS = -lm -lpthread

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700L -DVERSION=\"${VERSION}\"
CFLAGS   = -g -std=c99 -pedantic -Wall -Wextra -O0 ${INCS} ${CPPFLAGS}
# CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Wno-deprecated-declarations -Os ${INCS} ${CPPFLAGS}
LDFLAGS  = ${LIBS} -fuse-ld=mold

# compiler and linker
CC = ccache cc
