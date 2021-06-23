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
	cgm_vec3 vmin, vmax;
};

struct rayhit {
	float t;
	struct vertex v;
	cgm_ray ray;
	struct material *mtl;
};

int ray_triangle(cgm_ray *ray, struct triangle *tri, float tmax, struct rayhit *hit);
int ray_aabox_any(cgm_ray *ray, struct aabox *box, float tmax);

void aabox_init(struct aabox *box);
void aabox_addface(struct aabox *box, struct triangle *tri);
void aabox_union(struct aabox *res, struct aabox *a, struct aabox *b);
float aabox_surf_area(struct aabox *box);
float surf_area(float dx, float dy, float dz);

#endif	/* GEOM_H_ */
