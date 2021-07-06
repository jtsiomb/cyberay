#ifndef GAME_H_
#define GAME_H_

#include "level.h"
#include "drawtext.h"

struct options {
	int width, height;
	float scale;
	int nthreads;
	int max_iter;
	int nsamples;
};

int win_width, win_height;
float win_aspect;

struct level lvl;
struct options opt;

#define UIFONT_SZ	22
struct dtx_font *uifont;

int init_options(int argc, char **argv);

unsigned long get_msec(void);

#endif	/* GAME_H_ */
