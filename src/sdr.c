#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include "opengl.h"

#if defined(unix) || defined(__unix__)
#include <unistd.h>
#include <sys/stat.h>
#endif	/* unix */

#include "sdr.h"

static const char *sdrtypestr(unsigned int sdrtype);
static int sdrtypeidx(unsigned int sdrtype);


unsigned int create_vertex_shader(const char *src)
{
	return create_shader(src, GL_VERTEX_SHADER);
}

unsigned int create_pixel_shader(const char *src)
{
	return create_shader(src, GL_FRAGMENT_SHADER);
}

unsigned int create_tessctl_shader(const char *src)
{
#ifdef GL_TESS_CONTROL_SHADER
	return create_shader(src, GL_TESS_CONTROL_SHADER);
#else
	return 0;
#endif
}

unsigned int create_tesseval_shader(const char *src)
{
#ifdef GL_TESS_EVALUATION_SHADER
	return create_shader(src, GL_TESS_EVALUATION_SHADER);
#else
	return 0;
#endif
}

unsigned int create_geometry_shader(const char *src)
{
#ifdef GL_GEOMETRY_SHADER
	return create_shader(src, GL_GEOMETRY_SHADER);
#else
	return 0;
#endif
}

unsigned int create_shader(const char *src, unsigned int sdr_type)
{
	unsigned int sdr;
	int success, info_len;
	char *info_str = 0;
	const char *src_str[3], *header, *footer;
	int src_str_count = 0;
	GLenum err;

	if((header = get_shader_header(sdr_type))) {
		src_str[src_str_count++] = header;
	}
	src_str[src_str_count++] = src;
	if((footer = get_shader_footer(sdr_type))) {
		src_str[src_str_count++] = footer;
	}

	sdr = glCreateShader(sdr_type);
	assert(glGetError() == GL_NO_ERROR);
	glShaderSource(sdr, src_str_count, src_str, 0);
	err = glGetError();
	assert(err == GL_NO_ERROR);
	glCompileShader(sdr);
	assert(glGetError() == GL_NO_ERROR);

	glGetShaderiv(sdr, GL_COMPILE_STATUS, &success);
	assert(glGetError() == GL_NO_ERROR);
	glGetShaderiv(sdr, GL_INFO_LOG_LENGTH, &info_len);
	assert(glGetError() == GL_NO_ERROR);

	if(info_len) {
		if((info_str = malloc(info_len + 1))) {
			glGetShaderInfoLog(sdr, info_len, 0, info_str);
			assert(glGetError() == GL_NO_ERROR);
			info_str[info_len] = 0;
		}
	}

	if(success) {
		fprintf(stderr, info_str ? "done: %s\n" : "done\n", info_str);
	} else {
		fprintf(stderr, info_str ? "failed: %s\n" : "failed\n", info_str);
		glDeleteShader(sdr);
		sdr = 0;
	}

	free(info_str);
	return sdr;
}

void free_shader(unsigned int sdr)
{
	glDeleteShader(sdr);
}

unsigned int load_vertex_shader(const char *fname)
{
	return load_shader(fname, GL_VERTEX_SHADER);
}

unsigned int load_pixel_shader(const char *fname)
{
	return load_shader(fname, GL_FRAGMENT_SHADER);
}

unsigned int load_tessctl_shader(const char *fname)
{
#ifdef GL_TESS_CONTROL_SHADER
	return load_shader(fname, GL_TESS_CONTROL_SHADER);
#else
	return 0;
#endif
}

unsigned int load_tesseval_shader(const char *fname)
{
#ifdef GL_TESS_EVALUATION_SHADER
	return load_shader(fname, GL_TESS_EVALUATION_SHADER);
#else
	return 0;
#endif
}

unsigned int load_geometry_shader(const char *fname)
{
#ifdef GL_GEOMETRY_SHADER
	return load_shader(fname, GL_GEOMETRY_SHADER);
#else
	return 0;
#endif
}

unsigned int load_shader(const char *fname, unsigned int sdr_type)
{
	unsigned int sdr;
	size_t filesize;
	FILE *fp;
	char *src;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to open shader %s: %s\n", fname, strerror(errno));
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(!(src = malloc(filesize + 1))) {
		fclose(fp);
		return 0;
	}
	fread(src, 1, filesize, fp);
	src[filesize] = 0;
	fclose(fp);

	fprintf(stderr, "compiling %s shader: %s... ", sdrtypestr(sdr_type), fname);
	sdr = create_shader(src, sdr_type);

	free(src);
	return sdr;
}


/* ---- gpu programs ---- */

unsigned int create_program(void)
{
	unsigned int prog = glCreateProgram();
	assert(glGetError() == GL_NO_ERROR);
	return prog;
}

unsigned int create_program_link(unsigned int sdr0, ...)
{
	unsigned int prog, sdr;
	va_list ap;

	if(!(prog = create_program())) {
		return 0;
	}

	attach_shader(prog, sdr0);
	if(glGetError()) {
		return 0;
	}

	va_start(ap, sdr0);
	while((sdr = va_arg(ap, unsigned int))) {
		attach_shader(prog, sdr);
		if(glGetError()) {
			return 0;
		}
	}
	va_end(ap);

	if(link_program(prog) == -1) {
		free_program(prog);
		return 0;
	}
	return prog;
}

unsigned int create_program_load(const char *vfile, const char *pfile)
{
	unsigned int vs = 0, ps = 0;

	if(vfile && *vfile && !(vs = load_vertex_shader(vfile))) {
		return 0;
	}
	if(pfile && *pfile && !(ps = load_pixel_shader(pfile))) {
		return 0;
	}
	return create_program_link(vs, ps, 0);
}

void free_program(unsigned int sdr)
{
	glDeleteProgram(sdr);
}

void attach_shader(unsigned int prog, unsigned int sdr)
{
	int err;

	if(prog && sdr) {
		assert(glGetError() == GL_NO_ERROR);
		glAttachShader(prog, sdr);
		if((err = glGetError()) != GL_NO_ERROR) {
			fprintf(stderr, "failed to attach shader %u to program %u (err: 0x%x)\n", sdr, prog, err);
			abort();
		}
	}
}

int link_program(unsigned int prog)
{
	int linked, info_len, retval = 0;
	char *info_str = 0;

	glLinkProgram(prog);
	assert(glGetError() == GL_NO_ERROR);
	glGetProgramiv(prog, GL_LINK_STATUS, &linked);
	assert(glGetError() == GL_NO_ERROR);
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &info_len);
	assert(glGetError() == GL_NO_ERROR);

	if(info_len) {
		if((info_str = malloc(info_len + 1))) {
			glGetProgramInfoLog(prog, info_len, 0, info_str);
			assert(glGetError() == GL_NO_ERROR);
			info_str[info_len] = 0;
		}
	}

	if(linked) {
		fprintf(stderr, info_str ? "linking done: %s\n" : "linking done\n", info_str);
	} else {
		fprintf(stderr, info_str ? "linking failed: %s\n" : "linking failed\n", info_str);
		retval = -1;
	}

	free(info_str);
	return retval;
}

int bind_program(unsigned int prog)
{
	GLenum err;

	glUseProgram(prog);
	if(prog && (err = glGetError()) != GL_NO_ERROR) {
		/* maybe the program is not linked, try linking first */
		if(err == GL_INVALID_OPERATION) {
			if(link_program(prog) == -1) {
				return -1;
			}
			glUseProgram(prog);
			return glGetError() == GL_NO_ERROR ? 0 : -1;
		}
		return -1;
	}
	return 0;
}

/* ugly but I'm not going to write the same bloody code over and over */
#define BEGIN_UNIFORM_CODE \
	int loc, curr_prog; \
	glGetIntegerv(GL_CURRENT_PROGRAM, &curr_prog); \
	if((unsigned int)curr_prog != prog && bind_program(prog) == -1) { \
		return -1; \
	} \
	if((loc = glGetUniformLocation(prog, name)) != -1)

#define END_UNIFORM_CODE \
	if((unsigned int)curr_prog != prog) { \
		bind_program(curr_prog); \
	} \
	return loc == -1 ? -1 : 0

int get_uniform_loc(unsigned int prog, const char *name)
{
	int loc, curr_prog;
	glGetIntegerv(GL_CURRENT_PROGRAM, &curr_prog);
	if((unsigned int)curr_prog != prog && bind_program(prog) == -1) {
		return -1;
	}
	loc = glGetUniformLocation(prog, name);
	if((unsigned int)curr_prog != prog) {
		bind_program(curr_prog);
	}
	return loc;
}

int set_uniform_int(unsigned int prog, const char *name, int val)
{
	BEGIN_UNIFORM_CODE {
		glUniform1i(loc, val);
	}
	END_UNIFORM_CODE;
}

int set_uniform_float(unsigned int prog, const char *name, float val)
{
	BEGIN_UNIFORM_CODE {
		glUniform1f(loc, val);
	}
	END_UNIFORM_CODE;
}

int set_uniform_float2(unsigned int prog, const char *name, float x, float y)
{
	BEGIN_UNIFORM_CODE {
		glUniform2f(loc, x, y);
	}
	END_UNIFORM_CODE;
}

int set_uniform_float3(unsigned int prog, const char *name, float x, float y, float z)
{
	BEGIN_UNIFORM_CODE {
		glUniform3f(loc, x, y, z);
	}
	END_UNIFORM_CODE;
}

int set_uniform_float4(unsigned int prog, const char *name, float x, float y, float z, float w)
{
	BEGIN_UNIFORM_CODE {
		glUniform4f(loc, x, y, z, w);
	}
	END_UNIFORM_CODE;
}

int set_uniform_matrix4(unsigned int prog, const char *name, const float *mat)
{
	BEGIN_UNIFORM_CODE {
		glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
	}
	END_UNIFORM_CODE;
}

int set_uniform_matrix4_transposed(unsigned int prog, const char *name, const float *mat)
{
	BEGIN_UNIFORM_CODE {
		glUniformMatrix4fv(loc, 1, GL_TRUE, mat);
	}
	END_UNIFORM_CODE;
}

int get_attrib_loc(unsigned int prog, const char *name)
{
	int loc, curr_prog;

	glGetIntegerv(GL_CURRENT_PROGRAM, &curr_prog);
	if((unsigned int)curr_prog != prog && bind_program(prog) == -1) {
		return -1;
	}

	loc = glGetAttribLocation(prog, (char*)name);

	if((unsigned int)curr_prog != prog) {
		bind_program(curr_prog);
	}
	return loc;
}

void set_attrib_float3(int attr_loc, float x, float y, float z)
{
	glVertexAttrib3f(attr_loc, x, y, z);
}

/* ---- shader composition ---- */
struct string {
	char *text;
	int len;
};

#define NUM_SHADER_TYPES	5
static struct string header[NUM_SHADER_TYPES];
static struct string footer[NUM_SHADER_TYPES];

static void clear_string(struct string *str)
{
	free(str->text);
	str->text = 0;
	str->len = 0;
}

static void append_string(struct string *str, const char *s)
{
	int len, newlen;
	char *newstr;

	if(!s || !*s) return;

	len = strlen(s);
	newlen = str->len + len;
	if(!(newstr = malloc(newlen + 2))) {	/* leave space for a possible newline */
		fprintf(stderr, "shader composition: failed to append string of size %d\n", len);
		abort();
	}

	if(str->text) {
		memcpy(newstr, str->text, str->len);
	}
	memcpy(newstr + str->len, s, len + 1);

	if(s[len - 1] != '\n') {
		newstr[newlen] = '\n';
		newstr[newlen + 1] = 0;
	}

	free(str->text);
	str->text = newstr;
	str->len = newlen;
}

void clear_shader_header(unsigned int type)
{
	if(type) {
		int idx = sdrtypeidx(type);
		clear_string(&header[idx]);
	} else {
		int i;
		for(i=0; i<NUM_SHADER_TYPES; i++) {
			clear_string(&header[i]);
		}
	}
}

void clear_shader_footer(unsigned int type)
{
	if(type) {
		int idx = sdrtypeidx(type);
		clear_string(&footer[idx]);
	} else {
		int i;
		for(i=0; i<NUM_SHADER_TYPES; i++) {
			clear_string(&footer[i]);
		}
	}
}

void add_shader_header(unsigned int type, const char *s)
{
	if(type) {
		int idx = sdrtypeidx(type);
		append_string(&header[idx], s);
	} else {
		int i;
		for(i=0; i<NUM_SHADER_TYPES; i++) {
			append_string(&header[i], s);
		}
	}
}

void add_shader_footer(unsigned int type, const char *s)
{
	if(type) {
		int idx = sdrtypeidx(type);
		append_string(&footer[idx], s);
	} else {
		int i;
		for(i=0; i<NUM_SHADER_TYPES; i++) {
			append_string(&footer[i], s);
		}
	}
}

const char *get_shader_header(unsigned int type)
{
	int idx = sdrtypeidx(type);
	return header[idx].text;
}

const char *get_shader_footer(unsigned int type)
{
	int idx = sdrtypeidx(type);
	return footer[idx].text;
}

static const char *sdrtypestr(unsigned int sdrtype)
{
	switch(sdrtype) {
	case GL_VERTEX_SHADER:
		return "vertex";
	case GL_FRAGMENT_SHADER:
		return "pixel";
#ifdef GL_TESS_CONTROL_SHADER
	case GL_TESS_CONTROL_SHADER:
		return "tessellation control";
#endif
#ifdef GL_TESS_EVALUATION_SHADER
	case GL_TESS_EVALUATION_SHADER:
		return "tessellation evaluation";
#endif
#ifdef GL_GEOMETRY_SHADER
	case GL_GEOMETRY_SHADER:
		return "geometry";
#endif

	default:
		break;
	}
	return "<unknown>";
}

static int sdrtypeidx(unsigned int sdrtype)
{
	switch(sdrtype) {
	case GL_VERTEX_SHADER:
		return 0;
	case GL_FRAGMENT_SHADER:
		return 1;
	case GL_TESS_CONTROL_SHADER:
		return 2;
	case GL_TESS_EVALUATION_SHADER:
		return 3;
	case GL_GEOMETRY_SHADER:
		return 4;
	default:
		break;
	}
	return 0;
}
