#include "rt.h"

struct framebuffer fb;

int fbsize(int width, int height)
{
	void *tmp;

	if(!(tmp = malloc(width * height * sizeof *fb.pixels))) {
		return -1;
	}

	free(fb.pixels);
	fb.pixels = tmp;
	fb.width = width;
	fb.height = height;
	return 0;
}

void render(void)
{
	int i, j;
	cgm_vec3 *fbptr = fb.pixels;

	for(i=0; i<fb.height; i++) {
		for(j=0; j<fb.width; j++) {
			if(i < 3 || i >= fb.height - 3 || j < 3 || j >= fb.width - 3) {
				fbptr->x = 1.0f;
				fbptr->y = fbptr->z = 0.0f;
			} else {
				fbptr->x = fbptr->z = 0.0f;
				fbptr->y = 1.0f;
			}
			fbptr++;
		}
	}
}
