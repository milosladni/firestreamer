CC = gcc

# include user.mak with additional flags to compiler and linker
include ./user.mak

LIBS += -lssl -lcrypto
GIT_COMMIT = $(shell git describe --dirty | sed 's/[^0-9]*//')
CFLAGS += -DGIT_COMMIT=\"$(GIT_COMMIT)\"

CFLAGS += $(shell pkg-config --cflags gstreamer-1.0)
CFLAGS += $(shell pkg-config --cflags --libs gthread-2.0)
CFLAGS += $(shell pkg-config --cflags glib-2.0)

LIBS += $(shell pkg-config --libs gstreamer-1.0)
LIBS += -lgstreamer-1.0
LIBS += -lgstapp-1.0
LIBS += $(shell pkg-config --libs glib-2.0)

# include paths
CFLAGS+=-Isource

# source/appl
OBJECTS = source/appl/main.o \
          source/appl/bsp.o \
          source/appl/firestreamer.o

ifdef DEBUG
CFLAGS += -g -O0 -Wall -Wextra -UNDEBUG
else
CFLAGS += -O2 -Wall -Wextra -DNDEBUG
endif

CFLAGS += -MD

$(PROGRAM_NAME): $(OBJECTS)
	$(CC) $(OBJECTS) -lrt -lm -pthread -o $(PROGRAM_NAME) $(LDFLAGS) $(LDLIBS) $(LIBS)


# source/appl
main.o: main.c
	source/appl/main.c
bsp.o: bsp.c
	source/appl/bsp.c
firestreamer.o: firestreamer.c
	source/appl/firestreamer.c


.PHONY : clean
clean:
	rm -f $(OBJECTS)
	rm -f $(OBJECTS:.o=.d)

install:

.PHONY: all install clean

-include $(OBJECTS:.o=.d)
