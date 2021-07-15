#ifndef RT_H_
#define RT_H_

#include <cgmath/cgmath.h>
#include "image.h"
#include "tpool.h"

struct mtlattr {
	cgm_vec3 value;
	struct image *tex;
};

enum {
	MATTR_COLOR,
	MATTR_EMIT,
	MATTR_TRANSMIT,
	MATTR_ROUGHNESS,

	NUM_MATTR
};

struct material {
	char *name;
	struct mtlattr attr[NUM_MATTR];
	float ior;
	int metal;
	struct image *mask;
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

void tex_lookup(cgm_vec3 *res, struct image *img, float u, float v);

#endif	/* RT_H_ */
