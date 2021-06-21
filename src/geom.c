#include <float.h>
#include "geom.h"

void free_bvh_tree(struct bvhnode *tree)
{
	struct bvhnode *node, *tmp;

	free(tree->faces);

	free_bvh_tree(tree->left);
	free_bvh_tree(tree->right);
	free(tree);
}

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
		if(tmax <= tmin) return 0; \
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
