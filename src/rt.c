#include <stdlib.h>
#include <float.h>
#include "rt.h"
#include "game.h"

#define TILESZ	32

struct tile {
	int x, y, width, height;
	int sample;
	cgm_vec3 *fbptr;
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
	cgm_vec3 *fbptr;
	struct tile *tileptr;

	if(!(fbptr = malloc(width * height * sizeof *fb.pixels))) {
		return -1;
	}
	xtiles = (width + TILESZ - 1) / TILESZ;
	ytiles = (height + TILESZ - 1) / TILESZ;
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
			tileptr->width = width - x < TILESZ ? width - x : TILESZ;
			tileptr->height = height - y < TILESZ ? height - y : TILESZ;
			tileptr->fbptr = fbptr + x;
			tileptr++;

			x += TILESZ;
		}
		fbptr += width * TILESZ;
		y += TILESZ;
	}

	return 0;
}

void render(void)
{
	int i;

	for(i=0; i<num_tiles; i++) {
		tpool_enqueue(tpool, tiles + i, (tpool_callback)render_tile, 0);
	}
	tpool_wait(tpool);
}

static void render_tile(struct tile *tile)
{
	int i, j;
	cgm_ray ray;
	cgm_vec3 *fbptr = tile->fbptr;

	for(i=0; i<tile->height; i++) {
		for(j=0; j<tile->width; j++) {
			primary_ray(&ray, tile->x + j, tile->y + i, tile->sample);
			ray_trace(fbptr + j, &ray, 1.0f, opt.max_iter);
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
	color->x = color->y = color->z = 1.0f;
}

static void shade(cgm_vec3 *color, struct rayhit *hit, float energy, int max_iter)
{
	cgm_vec3 v;
	float pdiff, pspec, dp, rval, re;
	cgm_vec3 rcol;
	cgm_ray ray;

	color->x = color->y = color->z = 0.0f;

	/* pick diffuse direction */
	cgm_sphrand(&v, 1.0f);
	v.x += hit->v.pos.x + hit->v.norm.x;
	v.y += hit->v.pos.y + hit->v.norm.y;
	v.z += hit->v.pos.z + hit->v.norm.z;
	dp = cgm_vdot(&v, &hit->v.norm);

	rval = (float)rand() / (float)RAND_MAX;

	if(rval < (re = dp * energy)) {
		ray.origin = hit->v.pos;
		ray.dir = v;
		ray_trace(&rcol, &ray, re, max_iter - 1);
		cgm_vadd(color, &rcol);
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
