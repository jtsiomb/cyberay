#include <stdlib.h>
#include <float.h>
#include "bvh.h"

int build_bvh_sah(struct bvhnode *tree)
{
	int i, j;
	float sarea;

	if(tree->num_faces < 5 || tree->left || tree->right) return 0;

	dx = tree->aabb.vmax.x - tree->aabb.vmin.x;
	dy = tree->aabb.vmax.y - tree->aabb.vmin.y;
	dz = tree->aabb.vmax.z - tree->aabb.vmin.z;

	if((sarea = surf_area(dx, dy, dz)) <= 0.0f) return 0;

	tree->axis = dx > dy ? (dx > dz ? dx : dz) : (dy > dz ? dy : dz);

	/* TODO */
}

void free_bvh_tree(struct bvhnode *tree)
{
	free(tree->faces);
	free_bvh_tree(tree->left);
	free_bvh_tree(tree->right);
	free(tree);
}

int ray_bvhnode(cgm_ray *ray, struct bvhnode *bn, float tmax, struct rayhit *hit)
{
	int i, res = 0;
	struct rayhit hit0;

	if(!ray_aabox_any(ray, &bn->aabb, tmax)) {
		return 0;
	}

	if(!hit) {
		for(i=0; i<bn->num_faces; i++) {
			if(ray_triangle(ray, bn->faces + i, tmax, 0)) {
				return 1;
			}
		}
		return 0;
	}

	hit0.t = FLT_MAX;
	for(i=0; i<bn->num_faces; i++) {
		if(ray_triangle(ray, bn->faces + i, tmax, hit) && hit->t < hit0.t) {
			hit0 = *hit;
			res = 1;
		}
	}
	*hit = hit0;
	return res;
}
