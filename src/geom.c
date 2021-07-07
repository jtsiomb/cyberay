#include <float.h>
#include "geom.h"

int ray_triangle(cgm_ray *ray, struct triangle *tri, float tmax, struct rayhit *hit)
{
	float t, ndotdir;
	cgm_vec3 vdir, bc, pos;

	if(fabs(ndotdir = cgm_vdot(&ray->dir, &tri->norm)) <= 1e-6) {
		return 0;
	}

	vdir = tri->v[0].pos;
	cgm_vsub(&vdir, &ray->origin);

	if((t = cgm_vdot(&tri->norm, &vdir) / ndotdir) <= 1e-6 || t > tmax) {
		return 0;
	}

	cgm_raypos(&pos, ray, t);
	cgm_bary(&bc, &tri->v[0].pos, &tri->v[1].pos, &tri->v[2].pos, &pos);

	if(bc.x < 0.0f || bc.x > 1.0f) return 0;
	if(bc.y < 0.0f || bc.y > 1.0f) return 0;
	if(bc.z < 0.0f || bc.z > 1.0f) return 0;

	if(hit) {
		hit->t = t;
		hit->ray = *ray;
		hit->mtl = tri->mtl;

		hit->v.pos = pos;

		hit->v.norm.x = tri->v[0].norm.x * bc.x + tri->v[1].norm.x * bc.y + tri->v[2].norm.x * bc.z;
		hit->v.norm.y = tri->v[0].norm.y * bc.x + tri->v[1].norm.y * bc.y + tri->v[2].norm.y * bc.z;
		hit->v.norm.z = tri->v[0].norm.z * bc.x + tri->v[1].norm.z * bc.y + tri->v[2].norm.z * bc.z;
		/* cgm_vnormalize(&hit->v.norm); */

		hit->v.tex.x = tri->v[0].tex.x * bc.x + tri->v[1].tex.x * bc.y + tri->v[2].tex.x * bc.z;
		hit->v.tex.y = tri->v[0].tex.y * bc.x + tri->v[1].tex.y * bc.y + tri->v[2].tex.y * bc.z;
	}
	return 1;
}

#define SLABCHECK(dim)	\
	do { \
		invdir = 1.0f / ray->dir.dim;	\
		t0 = (box->vmin.dim - ray->origin.dim) * invdir;	\
		t1 = (box->vmax.dim - ray->origin.dim) * invdir;	\
		if(invdir < 0.0f) {	\
			tmp = t0;	\
			t0 = t1;	\
			t1 = tmp;	\
		}	\
		tmin = t0 > tmin ? t0 : tmin;	\
		tmax = t1 < tmax ? t1 : tmax;	\
		if(tmax < tmin) return 0; \
	} while(0)

int ray_aabox_any(cgm_ray *ray, struct aabox *box, float tmax)
{
	float invdir, t0, t1, tmp;
	float tmin = 0.0f;

	SLABCHECK(x);
	SLABCHECK(y);
	SLABCHECK(z);

	return 1;
}

void aabox_init(struct aabox *box)
{
	box->vmin.x = box->vmin.y = box->vmin.z = FLT_MAX;
	box->vmax.x = box->vmax.y = box->vmax.z = -FLT_MAX;
}

void aabox_addface(struct aabox *box, struct triangle *tri)
{
	int i;
	for(i=0; i<3; i++) {
		if(tri->v[i].pos.x < box->vmin.x) box->vmin.x = tri->v[i].pos.x;
		if(tri->v[i].pos.x > box->vmax.x) box->vmax.x = tri->v[i].pos.x;
		if(tri->v[i].pos.y < box->vmin.y) box->vmin.y = tri->v[i].pos.y;
		if(tri->v[i].pos.y > box->vmax.y) box->vmax.y = tri->v[i].pos.y;
		if(tri->v[i].pos.z < box->vmin.z) box->vmin.z = tri->v[i].pos.z;
		if(tri->v[i].pos.z > box->vmax.z) box->vmax.z = tri->v[i].pos.z;
	}
}

void aabox_union(struct aabox *res, struct aabox *a, struct aabox *b)
{
	res->vmin.x = a->vmin.x < b->vmin.x ? a->vmin.x : b->vmin.x;
	res->vmax.x = a->vmax.x > b->vmax.x ? a->vmax.x : b->vmax.x;
	res->vmin.y = a->vmin.y < b->vmin.y ? a->vmin.y : b->vmin.y;
	res->vmax.y = a->vmax.y > b->vmax.y ? a->vmax.y : b->vmax.y;
	res->vmin.z = a->vmin.z < b->vmin.z ? a->vmin.z : b->vmin.z;
	res->vmax.z = a->vmax.z > b->vmax.z ? a->vmax.z : b->vmax.z;
}

float aabox_surf_area(struct aabox *box)
{
	float dx, dy, dz;

	dx = box->vmax.x - box->vmin.x;
	dy = box->vmax.y - box->vmin.y;
	dz = box->vmax.z - box->vmin.z;

	return surf_area(dx, dy, dz);
}

float surf_area(float dx, float dy, float dz)
{
	if(dx < 0.0f) dx = 0.0f;
	if(dy < 0.0f) dy = 0.0f;
	if(dz < 0.0f) dz = 0.0f;

	return (dx * dy + dx * dz + dy * dz) * 2.0f;
}
