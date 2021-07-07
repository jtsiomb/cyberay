#include <stdlib.h>
#include <float.h>
#include <assert.h>
#include "rt.h"
#include "game.h"

struct tile {
	int x, y, width, height;
	int sample;
	cgm_vec4 *fbptr;
};

float vfov = M_PI / 4;

static float aspect;
static struct tile *tiles;
static int num_tiles;

static void render_tile(struct tile *tile);
static void ray_trace(cgm_vec3 *color, cgm_ray *ray, float energy, int max_iter);
static void bgcolor(cgm_vec3 *color, cgm_ray *ray);
static void shade(cgm_vec3 *color, struct rayhit *hit, float energy, int max_iter);
static void primary_ray(cgm_ray *ray, int x, int y, int sample);

int fbsize(int width, int height)
{
	int i, j, x, y, xtiles, ytiles;
	cgm_vec4 *fbptr;
	struct tile *tileptr;

	if(!(fbptr = malloc(width * height * sizeof *fb.pixels))) {
		return -1;
	}
	xtiles = (width + opt.tilesz - 1) / opt.tilesz;
	ytiles = (height + opt.tilesz - 1) / opt.tilesz;
	if(!(tileptr = malloc(xtiles * ytiles * sizeof *tiles))) {
		free(fbptr);
		return -1;
	}

	free(fb.pixels);
	fb.pixels = fbptr;
	fb.width = width;
	fb.height = height;

	free(tiles);
	tiles = tileptr;
	num_tiles = xtiles * ytiles;

	aspect = (float)fb.width / (float)fb.height;

	y = 0;
	for(i=0; i<ytiles; i++) {
		x = 0;
		for(j=0; j<xtiles; j++) {
			tileptr->x = x;
			tileptr->y = y;
			tileptr->width = width - x < opt.tilesz ? width - x : opt.tilesz;
			tileptr->height = height - y < opt.tilesz ? height - y : opt.tilesz;
			tileptr->fbptr = fbptr + x;
			tileptr->sample = 0;
			tileptr++;

			x += opt.tilesz;
		}
		fbptr += width * opt.tilesz;
		y += opt.tilesz;
	}

	return 0;
}

void render(int samplenum)
{
	int i;

	for(i=0; i<num_tiles; i++) {
		tiles[i].sample = samplenum;
		tpool_enqueue(tpool, tiles + i, (tpool_callback)render_tile, 0);
	}
	tpool_wait(tpool);
}

static void render_tile(struct tile *tile)
{
	int i, j;
	cgm_ray ray;
	cgm_vec3 col;
	cgm_vec4 *fbptr = tile->fbptr;

	for(i=0; i<tile->height; i++) {
		for(j=0; j<tile->width; j++) {
			primary_ray(&ray, tile->x + j, tile->y + i, tile->sample);
			if(tile->sample) {
				ray_trace(&col, &ray, 1.0f, opt.max_iter);
				fbptr[j].x += col.x;
				fbptr[j].y += col.y;
				fbptr[j].z += col.z;
				fbptr[j].w++;
			} else {
				ray_trace((cgm_vec3*)(fbptr + j), &ray, 1.0f, opt.max_iter);
				fbptr[j].w = 1;
			}
		}
		fbptr += fb.width;
	}
}

static void ray_trace(cgm_vec3 *color, cgm_ray *ray, float energy, int max_iter)
{
	struct rayhit hit;

	if(max_iter && ray_level(ray, &lvl, FLT_MAX, &hit)) {
		shade(color, &hit, energy, max_iter);
	} else {
		bgcolor(color, ray);
	}
}

static void bgcolor(cgm_vec3 *color, cgm_ray *ray)
{
	color->x = color->y = color->z = 100.0f;
}

static void shade(cgm_vec3 *color, struct rayhit *hit, float energy, int max_iter)
{
	cgm_vec3 v, n;
	float pdiff, pspec, inv_pdiff, inv_pspec, rval, re;
	cgm_vec3 rcol;
	cgm_ray ray;
	cgm_vec3 *mtlcol = &hit->mtl->color;

	if(cgm_vdot(&hit->ray.dir, &hit->v.norm) > 0.0f) {
		n.x = -hit->v.norm.x;
		n.y = -hit->v.norm.y;
		n.z = -hit->v.norm.z;
	} else {
		n = hit->v.norm;
	}

	*color = hit->mtl->emit;

	pdiff = (mtlcol->x + mtlcol->y + mtlcol->z) / 3.0f;

	rval = (float)rand() / (float)RAND_MAX;

	if(rval <= (re = energy * pdiff)) {
		cgm_vnormalize(&n);

		/* pick diffuse direction with a cosine-weighted probability */
		cgm_sphrand(&ray.dir, 0.98f);
		ray.dir.x += n.x;
		ray.dir.y += n.y;
		ray.dir.z += n.z;
		cgm_vnormalize(&ray.dir);

		if(cgm_vdot(&ray.dir, &n) < 0.0f) {
			ray.dir.x = -ray.dir.x;
			ray.dir.y = -ray.dir.y;
			ray.dir.z = -ray.dir.z;
		}

		ray.origin = hit->v.pos;
		ray_trace(&rcol, &ray, re, max_iter - 1);

		inv_pdiff = 1.0f / pdiff;

		color->x += rcol.x * mtlcol->x * inv_pdiff;
		color->y += rcol.y * mtlcol->y * inv_pdiff;
		color->z += rcol.z * mtlcol->z * inv_pdiff;
	}
}

static void primary_ray(cgm_ray *ray, int x, int y, int sample)
{
	ray->origin.x = ray->origin.y = ray->origin.z = 0.0f;
	ray->dir.x = (2.0f * (float)x / (float)fb.width - 1.0f) * aspect;
	ray->dir.y = 1.0f - 2.0f * (float)y / (float)fb.height;
	ray->dir.z = -1.0f / tan(vfov / 2.0f);
	cgm_vnormalize(&ray->dir);

	/* TODO jitter */

	cgm_rmul_mr(ray, view_xform);
}
