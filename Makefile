src = $(wildcard src/*.c) $(wildcard libs/miniglut/*.c)
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = cyberay

opt = -O3 -ffast-math -fno-strict-aliasing
dbg = -g
warn = -pedantic -Wall
def = -DMINIGLUT_USE_LIBC
inc = -Ilibs -Ilibs/treestore -Ilibs/miniglut
libdir = -Llibs/treestore -Llibs/imago

CFLAGS = $(warn) $(opt) $(dbg) $(def) $(inc) -pthread -MMD
LDFLAGS = $(libgl) $(libdir) -lm -pthread -limago -ltreestore

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
