src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = cyberay

opt = -O3 -ffast-math -fno-strict-aliasing
dbg = -g
warn = -pedantic -Wall
inc = -Ilibs

CFLAGS = $(warn) $(opt) $(dbg) -pthread -MMD $(inc)
LDFLAGS = $(libgl) -lm -pthread -limago

libgl = -lGL -lGLU -lglut

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)
