#ifndef RT_H_
#define RT_H_

#include <cgmath/cgmath.h>
#include "image.h"
#include "tpool.h"

struct material {
	char *name;
	cgm_vec3 color, specular, emit;
	float transmit;
	float roughness;
	float ior;
	int metal;

	struct image *tex_color;
};

struct framebuffer {
	int width, height;
	cgm_vec4 *pixels;
};

struct framebuffer fb;
struct thread_pool *tpool;
float view_xform[16];

int fbsize(int width, int height);

void render(int samplenum);

#endif	/* RT_H_ */
