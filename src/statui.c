#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <GL/gl.h>
#include "statui.h"
#include "tpool.h"
#include "drawtext.h"
#include "game.h"

#define VSCR_HEIGHT		512
#define VSCR_WIDTH		(VSCR_HEIGHT * win_aspect)

static int *usage;
static int nproc;
static pthread_t updthr;
static int quit;
static FILE *fp;

static float font_height, label_width;

static void *update_stat(void *cls);

int init_statui(void)
{
	if(!(fp = fopen("/proc/stat", "r"))) {
		return -1;
	}

	nproc = tpool_num_processors();
	if(!(usage = calloc(nproc, sizeof *usage))) {
		perror("init_statui");
		return -1;
	}

	if(pthread_create(&updthr, 0, update_stat, 0) == -1) {
		fprintf(stderr, "failed to create CPU stat update thread\n");
		return -1;
	}

	dtx_use_font(uifont, UIFONT_SZ);
	font_height = dtx_line_height();
	label_width = dtx_string_width("cpu XX: 000%") * 1.2f;
	return 0;
}

void destroy_statui(void)
{
	free(usage);

	if(updthr) {
		quit = 1;
		pthread_join(updthr, 0);
	}

	if(fp) fclose(fp);
}

void draw_statui(void)
{
	int i;
	float x, y, vscr_width = VSCR_WIDTH;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, VSCR_WIDTH, 0, VSCR_HEIGHT, -1, 1);

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, font_height, 0);

	dtx_use_font(uifont, UIFONT_SZ);

	x = 10;
	y = VSCR_HEIGHT - font_height * 2;
	for(i=0; i<nproc; i++) {
		dtx_position(x, y);
		dtx_printf("cpu %2d: %3d%%", i, usage[i]);
		x += label_width;

		if(x >= vscr_width) {
			x = 10;
			y -= font_height * 2;
		}
	}
	dtx_flush();

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void *update_stat(void *cls)
{
	int i, cpu;
	unsigned long val[7], delta[7], sum;
	static unsigned long prev[7];
	char buf[256];

	while(!quit) {
		rewind(fp);

		while(fgets(buf, sizeof buf, fp)) {
			if(sscanf(buf, "cpu%d %lu %lu %lu %lu %lu %lu %lu", &cpu, val, val + 1,
						val + 2, val + 3, val + 4, val + 5, val + 6) == 8) {
				if(cpu < 0 || cpu >= nproc) continue;

				sum = 0;
				for(i=0; i<7; i++) {
					delta[i] = val[i] - prev[i];
					sum += delta[i];
				}

				if(sum) {
					usage[cpu] = 100 - delta[3] * 100 / sum;
				}

				if(cpu == nproc - 1) break;
			}
		}

		usleep(500000);
	}

	return 0;
}
