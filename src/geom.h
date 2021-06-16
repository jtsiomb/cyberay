#ifndef GEOM_H_
#define GEOM_H_

#include <cgmath/cgmath.h>
#include "rt.h"

struct vertex {
	cgm_vec3 pos;
	cgm_vec3 norm;
	cgm_vec2 tex;
};

struct triangle {
	struct vertex v[3];
	cgm_vec3 norm;
	struct material *mtl;
};

struct aabox {
	cgm_vec3 p, r;
};

struct bvhnode {
	struct aabox aabb;

	struct triangle **faces;
	int num_faces, max_faces;

	struct bvhnode *sub;
	int num_sub;
};

struct rayhit {
	float t;
	struct vertex v;
	cgm_ray ray;
	struct material *mtl;
};

int ray_triangle(cgm_ray *ray, struct triangle *tri, float tmax, struct rayhit *hit);
int ray_aabox_any(cgm_ray *ray, struct aabox *box, float tmax);
int ray_bvhnode(cgm_ray *ray, struct bvhnode *bn, float tmax, struct rayhit *hit);

#endif	/* GEOM_H_ */
