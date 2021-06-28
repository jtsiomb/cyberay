#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <GL/gl.h>
#include "statui.h"
#include "tpool.h"
#include "drawtext.h"
#include "game.h"

#define VSCR_HEIGHT		600
#define VSCR_WIDTH		(VSCR_HEIGHT * win_aspect)

static int *usage;
static int nproc;
static pthread_t updthr;
static int quit, enabled;

static float font_height, label_width;

static void *update_stat(void *cls);

int init_statui(void)
{
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
}

void show_statui(int show)
{
	enabled = show;
}

void draw_statui(void)
{
	int i;
	float x, y, vscr_width;

	if(!enabled) return;

	vscr_width = VSCR_WIDTH;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, VSCR_WIDTH, 0, VSCR_HEIGHT, -1, 1);

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);

	glBegin(GL_QUADS);
	glColor3f(0.5, 0.5, 0.5);
	glVertex2f(0, 0);
	glVertex2f(VSCR_WIDTH, 0);
	glVertex2f(VSCR_WIDTH, VSCR_HEIGHT);
	glVertex2f(0, VSCR_HEIGHT);
	glEnd();

	glDisable(GL_BLEND);

	glColor3f(1, 1, 1);
	dtx_use_font(uifont, UIFONT_SZ);

	x = 10;
	y = VSCR_HEIGHT - font_height;
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

enum { USER, NICE, SYS, IDLE, IOWAIT, IRQ, SOFTIRQ, MAX_STATS };

struct cpustat {
	int64_t cnt[2][MAX_STATS];
};

static void *update_stat(void *cls)
{
	int i, cpu, cur = 0;
	struct cpustat *cpustat, *st;
	long delta[MAX_STATS], sum;
	char buf[256], *ptr, *endp;
	FILE *fp;

	cpustat = alloca(nproc * sizeof *cpustat);
	memset(cpustat, 0, nproc * sizeof *cpustat);

	while(!quit) {
		if(enabled) {
			usleep(500000);
		} else {
			usleep(1000000);
			continue;
		}

		if(!(fp = fopen("/proc/stat", "r"))) {
			return 0;
		}

		while(fgets(buf, sizeof buf, fp)) {
			if(sscanf(buf, "cpu%d", &cpu) == 1 && cpu >= 0 && cpu < nproc) {
				ptr = buf;
				while(*ptr && !isspace(*ptr)) ptr++;

				st = cpustat + cpu;

				sum = 0;
				for(i=0; i<MAX_STATS; i++) {
					st->cnt[cur][i] = strtoll(ptr, &endp, 10);
					ptr = endp;

					delta[i] = st->cnt[cur][i] - st->cnt[cur ^ 1][i];
					if(delta[i] < 0) delta[i] = 0;
					sum += delta[i];
				}

				if(sum) {
					usage[cpu] = 100 * (sum - delta[IDLE]) / sum;
				}

				if(cpu == nproc - 1) break;
			}
		}

		cur ^= 1;
		fclose(fp);
	}

	return 0;
}
