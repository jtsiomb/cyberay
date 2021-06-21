#include <float.h>
#include "rt.h"
#include "game.h"

#define TILESZ	32

struct tile {
	int x, y, width, height;
	int sample;
	cgm_vec3 *fbptr;
};

struct framebuffer fb;
struct thread_pool *tpool;
float view_xform[16];
float vfov = M_PI / 4;

static float aspect;
static struct tile *tiles;
static int num_tiles;

static void render_tile(struct tile *tile);
static void ray_trace(cgm_vec3 *color, cgm_ray *ray);
static void bgcolor(cgm_vec3 *color, cgm_ray *ray);
static void primary_ray(cgm_ray *ray, int x, int y, int sample);

int fbsize(int width, int height)
{
	int i, j, x, y, xtiles, ytiles;
	cgm_vec3 *fbptr;
	struct tile *tileptr;

	if(!(fbptr = malloc(width * height * sizeof *fb.pixels))) {
		return -1;
	}
	xtiles = width / TILESZ;
	ytiles = height / TILESZ;
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
			ray_trace(fbptr + j, &ray);
		}
		fbptr += fb.width;
	}
}

static void ray_trace(cgm_vec3 *color, cgm_ray *ray)
{
	struct rayhit hit;

	if(ray_level(ray, &lvl, FLT_MAX, &hit)) {
		color->x = hit.v.norm.x * 0.5 + 0.5;
		color->y = hit.v.norm.y * 0.5 + 0.5;
		color->z = hit.v.norm.z * 0.5 + 0.5;
	} else {
		bgcolor(color, ray);
	}
}

static void bgcolor(cgm_vec3 *color, cgm_ray *ray)
{
	color->x = color->y = color->z = 1.0f;
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
