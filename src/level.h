#ifndef LEVEL_H_
#define LEVEL_H_

#include "rt.h"
#include "bvh.h"

struct level {
	struct bvhnode *st_root;
	struct bvhnode *dyn_root;

	struct material *mtls;
	int num_mtls, max_mtls;
};

int load_level(struct level *lvl, const char *fname);
void destroy_level(struct level *lvl);

int ray_level(cgm_ray *ray, struct level *lvl, float tmax, struct rayhit *hit);

void draw_level(struct level *lvl);

#endif	/* LEVEL_H_ */
