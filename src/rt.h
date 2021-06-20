#ifndef RT_H_
#define RT_H_

#include <cgmath/cgmath.h>
#include "image.h"
#include "tpool.h"

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
extern struct thread_pool *tpool;
extern float view_xform[16];

int fbsize(int width, int height);

void render(void);

#endif	/* RT_H_ */
