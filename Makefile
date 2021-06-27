src = $(wildcard src/*.c) $(wildcard libs/miniglut/*.c)
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = cyberay

opt = -O3 -ffast-math -fno-strict-aliasing
dbg = -g
warn = -pedantic -Wall
def = -DMINIGLUT_USE_LIBC
inc = -Ilibs -Ilibs/treestore -Ilibs/miniglut -Ilibs/drawtext
libdir = -Llibs/treestore -Llibs/imago -Llibs/drawtext

CFLAGS = $(warn) $(opt) $(dbg) $(def) $(inc) -fcommon -pthread -MMD
LDFLAGS = -ldrawtext -limago -ltreestore $(libgl) $(libdir) -lm -pthread

libgl = -lGL -lGLU -lX11 -lXext

$(bin): $(obj) libs
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: libs
libs:
	$(MAKE) -C libs

.PHONY: clean-libs
clean-libs:
	$(MAKE) -C libs clean
