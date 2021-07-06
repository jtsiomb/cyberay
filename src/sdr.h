#ifndef SDR_H_
#define SDR_H_

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* ---- shaders ---- */
unsigned int create_vertex_shader(const char *src);
unsigned int create_pixel_shader(const char *src);
unsigned int create_tessctl_shader(const char *src);
unsigned int create_tesseval_shader(const char *src);
unsigned int create_geometry_shader(const char *src);
unsigned int create_shader(const char *src, unsigned int sdr_type);
void free_shader(unsigned int sdr);

unsigned int load_vertex_shader(const char *fname);
unsigned int load_pixel_shader(const char *fname);
unsigned int load_tessctl_shader(const char *fname);
unsigned int load_tesseval_shader(const char *fname);
unsigned int load_geometry_shader(const char *fname);
unsigned int load_shader(const char *src, unsigned int sdr_type);

int add_shader(const char *fname, unsigned int sdr);
int remove_shader(const char *fname);

/* ---- gpu programs ---- */
unsigned int create_program(void);
unsigned int create_program_link(unsigned int sdr0, ...);
unsigned int create_program_load(const char *vfile, const char *pfile);
void free_program(unsigned int sdr);

void attach_shader(unsigned int prog, unsigned int sdr);
int link_program(unsigned int prog);
int bind_program(unsigned int prog);

int get_uniform_loc(unsigned int prog, const char *name);

int set_uniform_int(unsigned int prog, const char *name, int val);
int set_uniform_float(unsigned int prog, const char *name, float val);
int set_uniform_float2(unsigned int prog, const char *name, float x, float y);
int set_uniform_float3(unsigned int prog, const char *name, float x, float y, float z);
int set_uniform_float4(unsigned int prog, const char *name, float x, float y, float z, float w);
int set_uniform_matrix4(unsigned int prog, const char *name, const float *mat);
int set_uniform_matrix4_transposed(unsigned int prog, const char *name, const float *mat);

int get_attrib_loc(unsigned int prog, const char *name);
void set_attrib_float3(int attr_loc, float x, float y, float z);

/* ---- shader composition ---- */

/* clear shader header/footer text.
 * pass the shader type to clear, or 0 to clear all types */
void clear_shader_header(unsigned int type);
void clear_shader_footer(unsigned int type);
/* append text to the header/footer of a specific shader type
 * or use type 0 to add it to all shade types */
void add_shader_header(unsigned int type, const char *s);
void add_shader_footer(unsigned int type, const char *s);
/* get the current header/footer text for a specific shader type */
const char *get_shader_header(unsigned int type);
const char *get_shader_footer(unsigned int type);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* SDR_H_ */
