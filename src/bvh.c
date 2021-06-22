#include <stdlib.h>
#include <float.h>
#include "bvh.h"

#define SPLIT_BUCKETS	8

int build_bvh_sah(struct bvhnode *tree)
{
	int i, j, best;
	float sp, sp0, spgap, area, ext[3];
	float spcost[SPLIT_BUCKETS];
	int part[SPLIT_BUCKETS];
	struct triangle **tribuf, **src;

	if(tree->num_faces < 16 || tree->left || tree->right) return 0;

	ext[0] = tree->aabb.vmax.x - tree->aabb.vmin.x;
	ext[1] = tree->aabb.vmax.y - tree->aabb.vmin.y;
	ext[2] = tree->aabb.vmax.z - tree->aabb.vmin.z;

	if((area = surf_area(ext[0], ext[1], ext[2])) <= 0.0f) return 0;

	tree->axis = ext[0] > ext[1] ? (ext[0] > ext[2] ? 0 : 2) : (ext[1] > ext[2] ? 1 : 2);

	spgap = ext[tree->axis] / SPLIT_BUCKETS;
	sp0 = cgm_velem(&tree->aabb.vmin, tree->axis) + spgap / 2.0f;

	/* allocate N arrays worth of triangle pointers, one for each bucket */
	if(!(tribuf = malloc(tree->num_faces * SPLIT_BUCKETS * sizeof *tribuf))) {
		fprintf(stderr, "failed to allocate buffer for BVH construction\n");
		return -1;
	}

	for(i=0; i<SPLIT_BUCKETS; i++) {
		sp = sp0 + i * spgap;
		spcost[i] = eval_cost(tree, area, sp, tribuf + i * tree->num_faces, part + i);
	}

	best = 0;
	for(i=1; i<SPLIT_BUCKETS; i++) {
		if(spcost[i] < spcost[best]) {
			best = i;
		}
	}

	/* found the best split, allocate child nodes, split the original array and copy
	 * the pointers from tribuf to each part
	 */
	if(!(tree->left = calloc(1, sizeof *tree->left)) || !(tree->right = calloc(1, sizeof *tree->right))) {
		fprintf(stderr, "failed to allocate tree nodes during BVH construction\n");
		free(tree->left);
		free(tribuf);
		return -1;
	}

	memcpy(tree->faces, tribuf + best * tree->num_faces, tree->num_faces * sizeof *tribuf);

	tree->left->faces = tree->faces;
	tree->left->num_faces = part[best];
	tree->right->faces = tree->faces + part[best];
	tree->right->num_faces = tree->faces - tree->left->faces;

	/* TODO: calc aabox for each child node */
	free(tribuf);
}

static float eval_cost(struct bvhnode *node, float area, float sp, struct triangle **tribuf, int *part)
{
	int i;
	float cost;
	struct triangle *tri;
	struct triangle **pleft, **pright;

	/* partition on sp */
	pleft = tribuf;
	pright = tribuf + node->num_faces - 1;

	for(i=0; i<node->num_faces; i++) {
		tri = node->faces[i];
		if(cgm_velem(tri->v, node->axis) < sp) {
			*pleft++ = tri;
		} else {
			*pright-- = tri;
		}
	}

	/* TODO calc cost */

	*part = pleft - tribuf;
	return cost;
}

void free_bvh_tree(struct bvhnode *tree)
{
	if(!tree) return;

	if(tree->max_faces) {
		/* only nodes with max_faces != 0 have ownership of the faces array. all
		 * the rest of the nodes only have pointers in the same array belonging
		 * to an ancestor node
		 */
		free(tree->faces);
	}
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
			if(ray_triangle(ray, bn->faces[i], tmax, 0)) {
				return 1;
			}
		}
		return 0;
	}

	hit0.t = FLT_MAX;
	for(i=0; i<bn->num_faces; i++) {
		if(ray_triangle(ray, bn->faces[i], tmax, hit) && hit->t < hit0.t) {
			hit0 = *hit;
			res = 1;
		}
	}
	*hit = hit0;
	return res;
}
