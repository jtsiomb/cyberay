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

#endif	/* RT_H_ */
