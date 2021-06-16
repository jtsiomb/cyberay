src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = cyberay

opt = -O3 -ffast-math
dbg = -g
warn = -pedantic -Wall

CFLAGS = $(warn) $(opt) $(dbg) -pthread -MMD
LDFLAGS = $(libgl) -lm -pthread

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
