#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "bvh.h"

#define SPLIT_BUCKETS	8

static float eval_split_cost(struct bvhnode *node, float area, float sp, struct triangle **tribuf, int *part);

int build_bvh_sah(struct bvhnode *tree)
{
	int i, best;
	float sp, sp0, spgap, area, ext[3];
	float spcost[SPLIT_BUCKETS];
	int part[SPLIT_BUCKETS];
	struct triangle **tribuf;
	struct aabox *aabb = &tree->aabb;

	if(tree->left || tree->right) return 0;

	/* calculate the bounding box for this node */
	aabox_init(aabb);
	for(i=0; i<tree->num_faces; i++) {
		aabox_addface(aabb, tree->faces[i]);
	}

	if(tree->num_faces < 16) {
		return 0;
	}

	ext[0] = aabb->vmax.x - aabb->vmin.x;
	ext[1] = aabb->vmax.y - aabb->vmin.y;
	ext[2] = aabb->vmax.z - aabb->vmin.z;

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
		spcost[i] = eval_split_cost(tree, area, sp, tribuf + i * tree->num_faces, part + i);
	}

	best = 0;
	for(i=1; i<SPLIT_BUCKETS; i++) {
		if(spcost[i] < spcost[best]) {
			best = i;
		}
	}

	if(spcost[best] > tree->num_faces) {
		/* the best split cost is worst than the no-split case */
		free(tribuf);
		return 0;
	}

	printf("split (%d): %d (out of %d)\n", tree->axis, part[best], tree->num_faces);

	/* found the best split, allocate child nodes, split the original array and copy
	 * the pointers from tribuf to each part
	 */
	if(!(tree->left = calloc(1, sizeof *tree->left)) || !(tree->right = calloc(1, sizeof *tree->right))) {
		fprintf(stderr, "failed to allocate tree nodes during BVH construction\n");
		free(tree->left);
		free(tribuf);
		return -1;
	}

	memcpy(tree->faces, tribuf, tree->num_faces * sizeof *tribuf);
	free(tribuf);

	tree->left->faces = tree->faces;
	tree->left->num_faces = part[best];
	tree->right->faces = tree->faces + part[best];
	tree->right->num_faces = tree->num_faces - part[best];

	tree->num_faces = 0;

	build_bvh_sah(tree->left);
	build_bvh_sah(tree->right);
	return 0;
}

static float eval_split_cost(struct bvhnode *node, float area, float sp, struct triangle **tribuf, int *part)
{
	int i, nleft, nright;
	float x, sp3, cost, sa_left, sa_right, cost_left, cost_right;
	struct triangle *tri;
	struct triangle **pleft, **pright;
	struct aabox bbleft, bbright;

	sp3 = sp * 3.0f;

	aabox_init(&bbleft);
	aabox_init(&bbright);

	/* partition on sp */
	pleft = tribuf;
	pright = tribuf + node->num_faces - 1;

	for(i=0; i<node->num_faces; i++) {
		tri = node->faces[i];
		x = cgm_velem(&tri->v[0].pos, node->axis);
		x += cgm_velem(&tri->v[1].pos, node->axis);
		x += cgm_velem(&tri->v[2].pos, node->axis);
		if(x < sp3) {
			aabox_addface(&bbleft, tri);
			*pleft++ = tri;
		} else {
			aabox_addface(&bbright, tri);
			*pright-- = tri;
		}
	}

	nleft = pleft - tribuf;
	nright = node->num_faces - nleft;

	sa_left = aabox_surf_area(&bbleft);
	sa_right = aabox_surf_area(&bbright);

	/* intersection cost = 1, traversal cost = 0.2 * intesection cost */
	cost_left = sa_left * nleft;
	cost_right = sa_right * nright;
	cost = 0.125f + (cost_left + cost_right) / area;

	*part = nleft;
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

void print_bvh_tree(struct bvhnode *tree, int lvl)
{
	int i;
	if(!tree) return;

	for(i=0; i<lvl; i++) {
		fputs("  ", stdout);
	}

	printf("(%g, %g, %g) -> (%g, %g, %g) ", tree->aabb.vmin.x, tree->aabb.vmin.y,
			tree->aabb.vmin.z, tree->aabb.vmax.x, tree->aabb.vmax.y, tree->aabb.vmax.z);

	if(tree->num_faces) {
		printf("leaf: %d triangles\n", tree->num_faces);
	} else {
		putchar('\n');
		print_bvh_tree(tree->left, lvl + 1);
		print_bvh_tree(tree->right, lvl + 1);
	}
}

int ray_bvhnode(cgm_ray *ray, struct bvhnode *bn, float tmax, struct rayhit *hit)
{
	int i, res = 0;
	struct rayhit hit0;

	if(!bn || !ray_aabox_any(ray, &bn->aabb, tmax)) {
		return 0;
	}

	if(!hit) {
		if(bn->num_faces) {
			for(i=0; i<bn->num_faces; i++) {
				if(ray_triangle(ray, bn->faces[i], tmax, 0)) {
					return 1;
				}
			}
			return 0;
		}
		return ray_bvhnode(ray, bn->left, tmax, 0) || ray_bvhnode(ray, bn->right, tmax, 0);
	}

	hit0.t = FLT_MAX;

	if(bn->num_faces) {
		/* leaf node, intersect faces */
		for(i=0; i<bn->num_faces; i++) {
			if(ray_triangle(ray, bn->faces[i], tmax, hit) && hit->t < hit0.t) {
				hit0 = *hit;
				res = 1;
			}
		}
	} else {
		/* internal node, recurse */
		if(ray_bvhnode(ray, bn->left, tmax, hit)) {
			hit0 = *hit;
			res = 1;
		}
		if(ray_bvhnode(ray, bn->right, tmax, hit) && hit->t < hit0.t) {
			hit0 = *hit;
			res = 1;
		}
	}

	if(res) {
		*hit = hit0;
		return 1;
	}
	return 0;
}
