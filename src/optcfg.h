/* generic unified commandline option and config file parsing library */
#ifndef LIBOPTCFG_H_
#define LIBOPTCFG_H_

#include <stdio.h>

struct optcfg;

struct optcfg_option {
	char c;				/* short (optional): used only for argument parsing */
	const char *s;		/* long: used for long options and config files */
	int opt;			/* the corresponding option enumeration */
	const char *desc;	/* text description for printing usage information */
};

#define OPTCFG_OPTIONS_END	{0, 0, -1, 0}

typedef int (*optcfg_opt_callback)(struct optcfg *oc, int opt, void *cls);
typedef int (*optcfg_arg_callback)(struct optcfg *oc, const char *arg, void *cls);

#ifdef __cplusplus
extern "C" {
#endif

/* initialize the optcfg object with a valid option vector terminated by an
 * entry with an opt value of -1 (other fields ignored for termination purposes)
 *
 * Example:
 *   struct optcfg_option options[] = {
 *       {'f', "foo", OPT_FOO, "Makes sure the foo is bar"},
 *       {'h', "help", OPT_HELP, "Print usage information and exit"},
 *       OPTCFG_OPTIONS_END
 *   };
 *   struct optcfg *oc = optcfg_init(options);
 */
struct optcfg *optcfg_init(struct optcfg_option *optv);
void optcfg_destroy(struct optcfg *oc);

/* The parse_* functions call the option callback for each option.
 *
 * The option callback can then call optcfg_next_value to retrieve any
 * values attached to this option. When optcfg_next_value returns 0, there
 * are no more values available.
 * The option callback must return 0 for success, and -1 to abort parsing.
 */
void optcfg_set_opt_callback(struct optcfg *oc, optcfg_opt_callback func, void *cls);
/* the argument callback is only called from optcfg_parse_args(), when a non-option
 * argument is encountered (an argument not starting with a dash)
 */
void optcfg_set_arg_callback(struct optcfg *oc, optcfg_arg_callback func, void *cls);

enum { OPTCFG_ERROR_FAIL, OPTCFG_ERROR_IGNORE };
void optcfg_set_error_action(struct optcfg *oc, int act);

int optcfg_parse_args(struct optcfg *oc, int argc, char **argv);
int optcfg_parse_config_file(struct optcfg *oc, const char *fname);
int optcfg_parse_config_stream(struct optcfg *oc, FILE *fp);
int optcfg_parse_config_line(struct optcfg *oc, const char *line);
/* TODO custom I/O callback version of config file parsing */

/* special value function which returns if the option is enabled or disabled
 * For config files it works similar to calling optcfg_next_value, and
 * optcfg_bool_value in sequence.
 * For argument parsing however, it doesn't consume further arguments. Merely
 * the presence of the option makes it enabled, and its presence with a -no-
 * or -disable- prefix disables it.
 */
int optcfg_enabled_value(struct optcfg *oc, int *enabledp);

/* call optcfg_next_value in the option callback to retrieve the next value
 * of the current option. returns 0 if there is no next value.
 */
char *optcfg_next_value(struct optcfg *oc);

/* helper function which can be used to print the available options */
void optcfg_print_options(struct optcfg *oc);

/* helper functions to convert value strings to typed values
 * returns 0 for success and value is returned through the valret pointer,
 * otherwise it returns -1 for type mismatch, and valret contents are undefined
 */
int optcfg_bool_value(char *str, int *valret);	/* accepts yes/no, true/false, 1/0 */
int optcfg_int_value(char *str, int *valret);
int optcfg_float_value(char *str, float *valret);

#ifdef __cplusplus
}
#endif

#endif	/* LIBOPTCFG_H_ */
