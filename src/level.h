#ifndef LEVEL_H_
#define LEVEL_H_

#include "rt.h"
#include "geom.h"

struct level {
	struct bvhnode *st_root;
	struct bvhnode *dyn_root;

	struct material *mtls;
};

int load_level(struct level *lvl, const char *fname);

int ray_level(cgm_ray *ray, struct level *lvl, float tmax, struct rayhit *hit);

#endif	/* LEVEL_H_ */
