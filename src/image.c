#include <imago2.h>
#include "image.h"

int load_image(struct image *img, const char *fname)
{
	if(!(img->pixels = img_load_pixels(fname, &img->width, &img->height, IMG_FMT_RGBF))) {
		fprintf(stderr, "load_image: failed to load %s\n", fname);
		return -1;
	}
	return 0;
}

void destroy_image(struct image *img)
{
	img_free_pixels(img->pixels);
	img->pixels = 0;
}
