#ifndef RT_H_
#define RT_H_

#include <cgmath/cgmath.h>
#include "image.h"

struct material {
	cgm_vec3 color;
	float roughness;
	float ior;

	struct image *tex_color;
};

struct framebuffer {
	int width, height;
	cgm_vec3 *pixels;
};

extern struct framebuffer fb;

int fbsize(int width, int height);

void render(void);

#endif	/* RT_H_ */
