#ifndef GAME_H_
#define GAME_H_

#include "level.h"

struct options {
	int width, height;
};

extern struct level lvl;
extern struct options opt;

int init_options(int argc, char **argv);

unsigned long get_msec(void);

#endif	/* GAME_H_ */
