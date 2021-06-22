#ifndef BVH_H_
#define BVH_H_

#include "geom.h"

struct bvhnode {
	struct aabox aabb;
	int axis;

	struct triangle *faces;
	int num_faces, max_faces;

	struct bvhnode *left, *right;
};

/* build_bvh* needs to be called with a pointer to a single-node tree,
 * containing all the faces, left/right as null, and a pre-computed aabb
 */
int build_bvh_sah(struct bvhnode *tree);
void free_bvh_tree(struct bvhnode *tree);

int ray_bvhnode(cgm_ray *ray, struct bvhnode *bn, float tmax, struct rayhit *hit);

#endif	/* BVH_H_ */
