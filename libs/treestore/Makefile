obj = treestore.o text.o dynarr.o
lib = libtreestore.a

CFLAGS = -O3

$(lib): $(obj)
	$(AR) rcs $@ $(obj)

.PHONY: clean
clean:
	rm -f $(obj) $(lib)
