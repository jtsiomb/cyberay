#include "opengl.h"
#include "cgmath/cgmath.h"
#include "disp.h"
#include "rt.h"
#include "sdr.h"
#include "game.h"


static unsigned int tex;
static int tex_width, tex_height;
static int tex_intfmt;
static float tex_xform[16];

static unsigned int sdr;
static int uloc_inv_gamma, uloc_exposure;

static unsigned int nextpow2(unsigned int x);


int init_display(void)
{
	float inv_gamma;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	tex_intfmt = GL_RGBA16F;

	if(!(sdr = create_program_load("sdr/vertex.glsl", "sdr/pixel.glsl"))) {
		return -1;
	}

	glUseProgram(sdr);
	if((uloc_inv_gamma = glGetUniformLocation(sdr, "inv_gamma")) != -1) {
		inv_gamma = 1.0f / opt.gamma;
		glUniform3f(uloc_inv_gamma, inv_gamma, inv_gamma, inv_gamma);
	}
	if((uloc_exposure = glGetUniformLocation(sdr, "exposure")) != -1) {
		exposure = 1.0f;
		glUniform1f(uloc_exposure, 1.0f);
	}
	glUseProgram(0);

	return 0;
}

void cleanup_display(void)
{
	glDeleteTextures(1, &tex);
	free_program(sdr);
}

void resize_display(int x, int y)
{
	if(x > tex_width || y > tex_height) {
		tex_width = nextpow2(x);
		tex_height = nextpow2(y);

		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, tex_intfmt, tex_width, tex_height, 0, GL_RGBA, GL_FLOAT, 0);
	}

	cgm_mscaling(tex_xform, (float)x / tex_width, (float)y / tex_height, 1.0f);

	glMatrixMode(GL_TEXTURE);
	glLoadMatrixf(tex_xform);
}

void display(void)
{
	static float prev_exp = 1.0f;

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fb.width, fb.height, GL_RGBA, GL_FLOAT, fb.pixels);
	glEnable(GL_TEXTURE_2D);

	bind_program(sdr);

	if(exposure != prev_exp) {
		glUniform1f(uloc_exposure, exposure);
		prev_exp = exposure;
	}

	glBegin(GL_QUADS);
	glColor3f(1, 1, 1);
	glTexCoord2f(0, 1);
	glVertex2f(-1, -1);
	glTexCoord2f(1, 1);
	glVertex2f(1, -1);
	glTexCoord2f(1, 0);
	glVertex2f(1, 1);
	glTexCoord2f(0, 0);
	glVertex2f(-1, 1);
	glEnd();

	bind_program(0);
}

static unsigned int nextpow2(unsigned int x)
{
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}
