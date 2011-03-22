CC=gcc
CFLAGS+= -std=c99 -pedantic -Wall -Wextra -g
# supported additional flags:
# * -DSHORTCUTS_EXTENSION
# * -DDEBUG
# * -DNDEBUG (assert.h)

MODULES=scanner stab_tree buffer main assembler output wait_queue
PROGNAME=pico

MODULES_O=$(foreach module,$(MODULES),$(module).o)
MODULES_C=$(foreach module,$(MODULES),$(module).c)

all: $(PROGNAME)

$(PROGNAME): $(MODULES_O)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) *.o $(PROGNAME) $(PROGNAME).zip

pack:
	zip $(PROGNAME).zip *.c *.h Makefile

.PHONY: clean all
