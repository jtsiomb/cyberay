#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "optcfg.h"

struct optcfg {
	struct optcfg_option *optlist;

	optcfg_opt_callback opt_func;
	void *opt_cls;
	optcfg_arg_callback arg_func;
	void *arg_cls;

	int err_abort;

	/* argument parsing state */
	char **argv;
	int argidx;
	int disable_opt;

	/* config file parsing state */
	const char *cfg_fname;
	int cfg_nline;
	char *cfg_value;
};

static int get_opt(struct optcfg *oc, const char *s, int *disable_opt);
static char *skip_spaces(char *s);
static void strip_comments(char *s);
static void strip_trailing_spaces(char *s);
static char *parse_keyval(char *line);


struct optcfg *optcfg_init(struct optcfg_option *optv)
{
	struct optcfg *oc;

	if(!(oc = calloc(1, sizeof *oc))) {
		return 0;
	}
	oc->optlist = optv;
	oc->err_abort = 1;
	return oc;
}

void optcfg_destroy(struct optcfg *oc)
{
	memset(oc, 0, sizeof *oc);
	free(oc);
}

void optcfg_set_opt_callback(struct optcfg *oc, optcfg_opt_callback func, void *cls)
{
	oc->opt_func = func;
	oc->opt_cls = cls;
}

void optcfg_set_arg_callback(struct optcfg *oc, optcfg_arg_callback func, void *cls)
{
	oc->arg_func = func;
	oc->arg_cls = cls;
}

void optcfg_set_error_action(struct optcfg *oc, int act)
{
	if(act == OPTCFG_ERROR_FAIL) {
		oc->err_abort = 1;
	} else if(act == OPTCFG_ERROR_IGNORE) {
		oc->err_abort = 0;
	}
}

int optcfg_parse_args(struct optcfg *oc, int argc, char **argv)
{
	int i;

	oc->argv = argv;

	for(i=1; i<argc; i++) {
		oc->argidx = i;

		if(argv[i][0] == '-') {
			if(oc->opt_func) {
				int o = get_opt(oc, argv[i], &oc->disable_opt);
				if(o == -1 || oc->opt_func(oc, o, oc->opt_cls) == -1) {
					if(oc->err_abort) {
						fprintf(stderr, "unexpected option: %s\n", argv[i]);
						return -1;
					}
				}
			} else {
				fprintf(stderr, "unexpected option: %s\n", argv[i]);
				if(oc->err_abort) {
					return -1;
				}
			}
		} else {
			if(oc->arg_func) {
				if(oc->arg_func(oc, argv[i], oc->arg_cls) == -1) {
					if(oc->err_abort) {
						fprintf(stderr, "unexpected argument: %s\n", argv[i]);
						return -1;
					}
				}
			} else {
				fprintf(stderr, "unexpected argument: %s\n", argv[i]);
				if(oc->err_abort) {
					return -1;
				}
			}
		}

		i = oc->argidx;
	}

	oc->argidx = 0;	/* done parsing args */
	return 0;
}

int optcfg_parse_config_file(struct optcfg *oc, const char *fname)
{
	int res;
	FILE *fp = fopen(fname, "rb");
	if(!fp) {
		return -1;
	}

	oc->cfg_fname = fname;
	res = optcfg_parse_config_stream(oc, fp);
	oc->cfg_fname = 0;
	fclose(fp);
	return res;
}

int optcfg_parse_config_stream(struct optcfg *oc, FILE *fp)
{
	char buf[512];

	oc->cfg_nline = 0;
	while(fgets(buf, sizeof buf, fp)) {
		++oc->cfg_nline;

		if(optcfg_parse_config_line(oc, buf) == -1) {
			if(oc->err_abort) {
				return -1;
			}
		}
	}
	return 0;
}

int optcfg_parse_config_line(struct optcfg *oc, const char *line)
{
	int opt, len;
	char *start, *val, *buf;

	len = strlen(line);
	buf = alloca(len + 1);
	memcpy(buf, line, len + 1);

	start = skip_spaces(buf);
	strip_comments(start);
	strip_trailing_spaces(start);
	if(!*start) {
		return 0;
	}

	if(!(val = parse_keyval(start))) {
		fprintf(stderr, "error parsing %s line %d: invalid syntax\n", oc->cfg_fname ? oc->cfg_fname : "", oc->cfg_nline);
		return -1;
	}
	oc->cfg_value = val;
	if((opt = get_opt(oc, start, 0)) == -1) {
		fprintf(stderr, "error parsing %s line %d: unknown option: %s\n", oc->cfg_fname ? oc->cfg_fname : "",
				oc->cfg_nline, start);
		return -1;
	}

	if(oc->opt_func) {
		if(oc->opt_func(oc, opt, oc->opt_cls) == -1) {
			return -1;
		}
	}
	oc->cfg_value = 0;
	return 0;
}

int optcfg_enabled_value(struct optcfg *oc, int *enabledp)
{
	if(oc->argidx) {
		*enabledp = ~oc->disable_opt & 1;
	} else {
		char *val = optcfg_next_value(oc);
		if(optcfg_bool_value(val, enabledp) == -1) {
			return -1;
		}
	}
	return 0;
}


char *optcfg_next_value(struct optcfg *oc)
{
	if(oc->argidx) {	/* we're in the middle of parsing arguments, so get the next one */
		return oc->argv[++oc->argidx];
	}
	if(oc->cfg_value) {
		char *val = oc->cfg_value;
		oc->cfg_value = 0;
		return val;
	}
	return 0;
}

void optcfg_print_options(struct optcfg *oc)
{
	int i;
	for(i=0; oc->optlist[i].opt != -1; i++) {
		struct optcfg_option *opt = oc->optlist + i;

		if(opt->c) {
			printf(" -%c", opt->c);
		} else {
			printf("   ");
		}
		if(opt->s) {
			printf("%c-%s: ", opt->c ? ',' : ' ', opt->s);
		} else {
			printf(": ");
		}
		printf("%s\n", opt->desc ? opt->desc : "undocumented");
	}
}

int optcfg_bool_value(char *s, int *valret)
{
	if(strcasecmp(s, "yes") == 0 || strcasecmp(s, "true") == 0 || strcmp(s, "1") == 0) {
		*valret = 1;
		return 0;
	}
	if(strcasecmp(s, "no") == 0 || strcasecmp(s, "false") == 0 || strcmp(s, "0") == 0) {
		*valret = 0;
		return 0;
	}
	return -1;
}

int optcfg_int_value(char *str, int *valret)
{
	char *endp;
	*valret = strtol(str, &endp, 0);
	if(endp == str) {
		return -1;
	}
	return 0;
}

int optcfg_float_value(char *str, float *valret)
{
	char *endp;
	*valret = strtod(str, &endp);
	if(endp == str) {
		return -1;
	}
	return 0;
}



static int get_opt(struct optcfg *oc, const char *arg, int *disable_opt)
{
	int i, ndashes = 0;

	while(*arg && *arg == '-') {
		++ndashes;
		++arg;
	}

	if(ndashes > 2) {
		arg -= ndashes;
		ndashes = 0;
	}

	if(disable_opt) {
		if(ndashes && (strstr(arg, "no-") == arg || strstr(arg, "disable-") == arg)) {
			*disable_opt = 1;
			arg = strchr(arg, '-') + 1;	/* guaranteed to exist at this point */
		} else {
			*disable_opt = 0;
		}
	}

	if(arg[1]) {	/* match long options */
		for(i=0; oc->optlist[i].opt != -1; i++) {
			if(strcmp(arg, oc->optlist[i].s) == 0) {
				return i;
			}
		}
	} else {
		for(i=0; oc->optlist[i].opt != -1; i++) {
			if(arg[0] == oc->optlist[i].c) {
				return i;
			}
		}
	}
	return -1;
}

static char *skip_spaces(char *s)
{
	while(*s && isspace(*s)) ++s;
	return s;
}

static void strip_comments(char *s)
{
	while(*s && *s != '#') ++s;
	if(*s == '#') *s = 0;
}

static void strip_trailing_spaces(char *s)
{
	char *end = s + strlen(s) - 1;
	while(end >= s && isspace(*end)) {
		*end-- = 0;
	}
}

static char *parse_keyval(char *line)
{
	char *val;
	char *eq = strchr(line, '=');
	if(!eq) return 0;

	*eq = 0;
	strip_trailing_spaces(line);
	val = skip_spaces(eq + 1);
	strip_trailing_spaces(val);

	if(!*line || !*val) {
		return 0;
	}
	return val;
}
