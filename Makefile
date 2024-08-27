CC = gcc

# include user.mak with additional flags to compiler and linker
include ./user.mak

LIBS += -lwebsockets -lssl -lcrypto
GIT_COMMIT = $(shell git describe --dirty | sed 's/[^0-9]*//')
CFLAGS += -DGIT_COMMIT=\"$(GIT_COMMIT)\"

# include paths
CFLAGS+=-Isource \
        -Isource/firmwarestack \
        -Isource/firmwarestack/middleware

# source/appl
OBJECTS = source/appl/main.o \
          source/appl/bsp.o \
          source/appl/firestreamer.o

#source/tcsfs/middleware
OBJECTS += source/firmwarestack/middleware/videostreamer/videostreamer.o \
           source/firmwarestack/middleware/gassensor/gassensor.o

# source/tcsfs/tools
#OBJECTS += source/firmwarestack/tools/toolbox.o \
#		   source/firmwarestack/tools/json/json.o \
#		   source/firmwarestack/tools/resource/resource.o \
#		   source/firmwarestack/tools/hash/sha1/sha1.o

# source/tcsfs/misc
#OBJECTS += source/firmwarestack/misc/dbg/dbg.o


# source/tcsfs/thirdparty/jansson
CFLAGS += -isystem source/firmwarestack/thirdparty/jansson/inc -DHAVE_STDINT_H=1
THIRDPARTYOBJECTS += source/firmwarestack/thirdparty/jansson/src/dump.o \
           source/firmwarestack/thirdparty/jansson/src/error.o \
           source/firmwarestack/thirdparty/jansson/src/hashtable.o \
           source/firmwarestack/thirdparty/jansson/src/hashtable_seed.o \
           source/firmwarestack/thirdparty/jansson/src/load.o \
           source/firmwarestack/thirdparty/jansson/src/memory.o \
           source/firmwarestack/thirdparty/jansson/src/pack_unpack.o \
           source/firmwarestack/thirdparty/jansson/src/strbuffer.o \
           source/firmwarestack/thirdparty/jansson/src/strconv.o \
           source/firmwarestack/thirdparty/jansson/src/utf.o \
           source/firmwarestack/thirdparty/jansson/src/value.o

ifdef DEBUG
CFLAGS += -g -O0 -Wall -Wextra -UNDEBUG
else
CFLAGS += -O2 -Wall -Wextra -DNDEBUG
endif

CFLAGS += -MD

$(THIRDPARTYOBJECTS): CFLAGS += -Wno-unused-function -Wno-format-truncation

$(PROGRAM_NAME): $(OBJECTS) $(THIRDPARTYOBJECTS)
	$(CC) $(OBJECTS) $(THIRDPARTYOBJECTS) -lrt -lm -pthread -o $(PROGRAM_NAME) $(LDFLAGS) $(LDLIBS) $(LIBS)


# source/appl
main.o: main.c
	source/appl/main.c
bsp.o: bsp.c
	source/appl/bsp.c


# source/firmwarestack/middleware
videostreamer.o: videostreamer.c
	source/firmwarestack/middleware/videostreamer/videostreamer.c
gassensor.o: gassensor.c
	source/firmwarestack/middleware/gassensor/gassensor.c

# source/tcsfs/tools
toolbox.o: toolbox.c
	source/tcsfs/tools/toolbox.c
base64.o: base64.c
	source/tcsfs/tools/base64/base64.c
json.o: json.c
	source/tcsfs/tools/json/json.c
resource.o: resource.c
	source/tcsfs/tools/resource/resource.c
sha1.o: sha1.c
	source/tcsfs/tools/hash/sha1/sha1.c

# source/firmwarestack/misc
dbg.o: dbg.c
	source/firmwarestack/misc/dbg/dbg.c

# source/firmwarestack/platforms
hwbridge.o: hwbridge.c
	source/firmwarestack/platforms/os/devices/osmcu/hwbridge.c

# source/tcsfs/thirdparty/jansson
dump.o: dump.c
	source/tcsfs/thirdparty/jansson/src/dump.c
error.o: error.c
	source/tcsfs/thirdparty/jansson/src/error.c
hashtable.o: hashtable.c
	source/tcsfs/thirdparty/jansson/src/hashtable.c
hashtable_seed.o: hashtable_seed.c
	source/tcsfs/thirdparty/jansson/src/hashtable_seed.c
load.o: load.c
	source/tcsfs/thirdparty/jansson/src/load.c
memory.o: memory.c
	source/tcsfs/thirdparty/jansson/src/memory.c
pack_unpack.o: pack_unpack.c
	source/tcsfs/thirdparty/jansson/src/pack_unpack.c
strbuffer.o: strbuffer.c
	source/tcsfs/thirdparty/jansson/src/strbuffer.c
strconv.o: strconv.c
	source/tcsfs/thirdparty/jansson/src/strconv.c
utf.o: utf.c
	source/tcsfs/thirdparty/jansson/src/utf.c
value.o: value.c
	source/tcsfs/thirdparty/jansson/src/value.c


.PHONY : clean
clean:
	rm -f $(OBJECTS)
	rm -f $(OBJECTS:.o=.d)
	rm -f $(THIRDPARTYOBJECTS)
	rm -f $(THIRDPARTYOBJECTS:.o=.d)

install:

.PHONY: all install clean

-include $(OBJECTS:.o=.d)
-include $(THIRDPARTYOBJECTS:.o=.d)
