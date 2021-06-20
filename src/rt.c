#include "rt.h"

struct framebuffer fb;
struct thread_pool *tpool;
float view_xform[16];
float vfov = M_PI / 4;

static float aspect;

static void ray_trace(cgm_vec3 *color, cgm_ray *ray);
static void primary_ray(cgm_ray *ray, int x, int y, int sample);

int fbsize(int width, int height)
{
	void *tmp;

	if(!(tmp = malloc(width * height * sizeof *fb.pixels))) {
		return -1;
	}

	free(fb.pixels);
	fb.pixels = tmp;
	fb.width = width;
	fb.height = height;

	aspect = (float)fb.width / (float)fb.height;

	return 0;
}

void render(void)
{
	int i, j;
	cgm_ray ray;
	cgm_vec3 *fbptr = fb.pixels;

	for(i=0; i<fb.height; i++) {
		for(j=0; j<fb.width; j++) {
			primary_ray(&ray, j, i, 0);
			ray_trace(fbptr, &ray);
			fbptr++;
		}
	}
}

static void ray_trace(cgm_vec3 *color, cgm_ray *ray)
{
	color->x = ray->dir.x * 0.5f + 0.5f;
	color->y = ray->dir.y * 0.5f + 0.5f;
	color->z = 0.0f;
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
