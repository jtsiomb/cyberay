#include "game.h"
#include "optcfg.h"

enum { OPT_SIZE, OPT_SCALE, OPT_NTHREADS, OPT_TILESZ, OPT_ITER, OPT_SAMPLES, OPT_GAMMA, OPT_HELP };

static struct optcfg_option options[] = {
	{'s', "size", OPT_SIZE, "rendering resolution (WxH)"},
	{0, "scale", OPT_SCALE, "output scale factor"},
	{'t', "threads", OPT_NTHREADS, "number of worker threads to use for rendering (0 means auto-detect)"},
	{'T', "tile", OPT_TILESZ, "render tile size"},
	{0, "iter", OPT_ITER, "maximum recursion depth"},
	{'S', "samples", OPT_SAMPLES, "number of samples per pixel"},
	{0, "gamma", OPT_GAMMA, "output gamma"},
	{'h', "help", OPT_HELP, "print usage and exit"},
	OPTCFG_OPTIONS_END
};

static int opt_handler(struct optcfg *o, int opt, void *cls);
static int arg_handler(struct optcfg *o, const char *arg, void *cls);

int init_options(int argc, char **argv)
{
	struct optcfg *optcfg;

	opt.width = 1280;
	opt.height = 800;
	opt.scale = 1.0f;
	opt.nthreads = 0;
	opt.tilesz = 32;
	opt.max_iter = 6;
	opt.nsamples = 2;
	opt.gamma = 2.2;

	optcfg = optcfg_init(options);
	optcfg_set_opt_callback(optcfg, opt_handler, argv[0]);
	optcfg_set_arg_callback(optcfg, arg_handler, argv[0]);
	optcfg_parse_config_file(optcfg, "cyberay.conf");
	if(optcfg_parse_args(optcfg, argc, argv) == -1) {
		return -1;
	}

	optcfg_destroy(optcfg);
	return 0;
}

static int opt_handler(struct optcfg *o, int optid, void *cls)
{
	char *val;

	switch(optid) {
	case OPT_SIZE:
		if(!(val = optcfg_next_value(o)) || sscanf(val, "%dx%d", &opt.width, &opt.height) != 2) {
			fprintf(stderr, "size: expected <width>x<height>\n");
			return -1;
		}
		break;

	case OPT_SCALE:
		if(!(val = optcfg_next_value(o)) || optcfg_float_value(val, &opt.scale) == -1 ||
				opt.scale <= 0.0f) {
			fprintf(stderr, "scale: expected a positive number\n");
			return -1;
		}
		break;

	case OPT_NTHREADS:
		if(!(val = optcfg_next_value(o)) || optcfg_int_value(val, &opt.nthreads) == -1 ||
				opt.nthreads < 0) {
			fprintf(stderr, "threads: expected a positive number or 0 for auto-detect\n");
			return -1;
		}
		break;

	case OPT_TILESZ:
		if(!(val = optcfg_next_value(o)) || optcfg_int_value(val, &opt.tilesz) == -1 ||
				opt.tilesz <= 0) {
			fprintf(stderr, "tile: expected a render tile size value\n");
			return -1;
		}
		break;

	case OPT_ITER:
		if(!(val = optcfg_next_value(o)) || optcfg_int_value(val, &opt.max_iter) == -1 ||
				opt.max_iter <= 0) {
			fprintf(stderr, "iter: expected the maximum number of iterations\n");
			return -1;
		}
		break;

	case OPT_SAMPLES:
		if(!(val = optcfg_next_value(o)) || optcfg_int_value(val, &opt.nsamples) == -1 ||
				opt.nsamples <= 0) {
			fprintf(stderr, "samples: expected the number of samples per pixel\n");
			return -1;
		}
		break;

	case OPT_GAMMA:
		if(!(val = optcfg_next_value(o)) || optcfg_float_value(val, &opt.gamma) == -1 ||
				opt.gamma <= 0.0f) {
			fprintf(stderr, "gamma: expected output gamma value\n");
			return -1;
		}
		break;

	case OPT_HELP:
		printf("Usage: %s [options]\n", (char*)cls);
		printf("Options:\n");
		optcfg_print_options(o);
		exit(0);
	}
	return 0;
}

static int arg_handler(struct optcfg *o, const char *arg, void *cls)
{
	if(opt.lvlfile) {
		fprintf(stderr, "unexpected argument: %s\n", arg);
		return -1;
	}
	opt.lvlfile = arg;
	return 0;
}
