obj = font.o draw.o drawgl.o drawrast.o utf8.o
name = libdrawtext
lib_a = $(name).a

CFLAGS = -pedantic -Wall -O3 -DNO_FREETYPE -fcommon

$(lib_a): $(obj)
	$(AR) rcs $@ $(obj)

.PHONY: clean
clean:
	rm -f $(obj) $(lib_a)
