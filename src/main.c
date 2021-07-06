#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cgmath/cgmath.h>
#include "opengl.h"
#include "miniglut.h"
#include "game.h"
#include "disp.h"
#include "level.h"
#include "rt.h"
#include "statui.h"

enum {
	KEY_F1		= GLUT_KEY_F1 | 0x100,
	KEY_F2		= GLUT_KEY_F2 | 0x100,
	KEY_F3		= GLUT_KEY_F3 | 0x100,
	KEY_F4		= GLUT_KEY_F4 | 0x100,
	KEY_F5		= GLUT_KEY_F5 | 0x100,
	KEY_F6		= GLUT_KEY_F6 | 0x100,
	KEY_F7		= GLUT_KEY_F7 | 0x100,
	KEY_F8		= GLUT_KEY_F8 | 0x100,
	KEY_F9		= GLUT_KEY_F9 | 0x100,
	KEY_F10		= GLUT_KEY_F10 | 0x100,
	KEY_F11		= GLUT_KEY_F11 | 0x100,
	KEY_F12		= GLUT_KEY_F12 | 0x100,
	KEY_LEFT	= GLUT_KEY_LEFT | 0x100,
	KEY_UP		= GLUT_KEY_UP | 0x100,
	KEY_RIGHT	= GLUT_KEY_RIGHT | 0x100,
	KEY_DOWN	= GLUT_KEY_DOWN | 0x100,
	KEY_PGUP	= GLUT_KEY_PAGE_UP | 0x100,
	KEY_PGDN	= GLUT_KEY_PAGE_DOWN | 0x100,
	KEY_HOME	= GLUT_KEY_HOME | 0x100,
	KEY_END		= GLUT_KEY_END | 0x100,
	KEY_INS		= GLUT_KEY_INSERT | 0x100
};

enum { INP_FWD, INP_BACK, INP_RIGHT, INP_LEFT, INP_FIRE, NUM_INPUTS };

static int init(void);
static void cleanup(void);
static void disp(void);
static void idle(void);
static void reshape(int x, int y);
static void resizefb(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void keyup(unsigned char key, int x, int y);
static void skeydown(int key, int x, int y);
static void skeyup(int key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);

static int cur_sample;

static float cam_theta, cam_phi;
static cgm_vec3 cam_pos = {0, 1.6, 0};

static int mouse_x, mouse_y;
static int bnstate[8];

static int inpstate[NUM_INPUTS];

static int keymap[NUM_INPUTS][2] = {
	{'w', KEY_UP},
	{'s', KEY_DOWN},
	{'d', KEY_RIGHT},
	{'a', KEY_LEFT},
	{' ', 0}
};

static int auto_res;

static unsigned long nframes;
static unsigned long start_time;


int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	if(init_options(argc, argv) == -1) {
		return 1;
	}

	glutInitWindowSize(opt.width * opt.scale, opt.height * opt.scale);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("cyberay");

	glutDisplayFunc(disp);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(skeydown);
	glutSpecialUpFunc(skeyup);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(motion);

	if(init() == -1) {
		return 1;
	}
	atexit(cleanup);

	start_time = glutGet(GLUT_ELAPSED_TIME);

	glutMainLoop();
	return 0;
}

unsigned long get_msec(void)
{
	return glutGet(GLUT_ELAPSED_TIME) - start_time;
}

static int init(void)
{
	if(init_opengl() == -1) {
		return -1;
	}
	if(!glcaps.sdr) {
		fprintf(stderr, "OpenGL shader support required\n");
		return -1;
	}

	if(!(uifont = dtx_open_font_glyphmap("data/serif.glyphmap"))) {
		fprintf(stderr, "failed to load font\n");
		return -1;
	}

	if(!(tpool = tpool_create(opt.nthreads))) {
		fprintf(stderr, "failed to create thread pool\n");
		return -1;
	}

	glEnable(GL_CULL_FACE);

	if(init_display() == -1) {
		return -1;
	}

	if(load_level(&lvl, "data/test.lvl") == -1) {
		return -1;
	}

	resizefb(opt.width, opt.height);
	init_statui();
	return 0;
}

static void cleanup(void)
{
	float tsec;

	tsec = get_msec() / 1000.0f;
	printf("avg framerate: %.2f fps\n", (float)nframes / tsec);

	destroy_level(&lvl);

	cleanup_display();

	destroy_statui();

	tpool_destroy(tpool);

	dtx_close_font(uifont);
}

#define WALK_SPEED 3.0f
static void update(void)
{
	static unsigned int prev_upd;
	unsigned long msec;
	float dt, vfwd, vright;

	msec = get_msec();
	dt = (float)(msec - prev_upd) / 1000.0f;
	prev_upd = msec;

	vfwd = vright = 0;

	if(inpstate[INP_FWD]) {
		vfwd -= WALK_SPEED * dt;
		cur_sample = 0;
	}
	if(inpstate[INP_BACK]) {
		vfwd += WALK_SPEED * dt;
		cur_sample = 0;
	}
	if(inpstate[INP_RIGHT]) {
		vright += WALK_SPEED * dt;
		cur_sample = 0;
	}
	if(inpstate[INP_LEFT]) {
		vright -= WALK_SPEED * dt;
		cur_sample = 0;
	}

	cam_pos.x += cos(cam_theta) * vright + sin(cam_theta) * vfwd;
	cam_pos.z += -sin(cam_theta) * vright + cos(cam_theta) * vfwd;

	cgm_midentity(view_xform);
	cgm_mrotate_x(view_xform, cam_phi);
	cgm_mrotate_y(view_xform, cam_theta);
	cgm_mtranslate(view_xform, cam_pos.x, cam_pos.y, cam_pos.z);
}

static void disp(void)
{
	update();

	render(cur_sample++);
	display();
	draw_statui();

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);

	nframes++;
}

static void idle(void)
{
	glutPostRedisplay();
}

static void reshape(int x, int y)
{
	glViewport(0, 0, x, y);
	win_width = x;
	win_height = y;
	win_aspect = (float)x / (float)y;

	if(auto_res) {
		resizefb(x, y);
	}
}

static void resizefb(int x, int y)
{
	printf("resize framebuffer: %dx%d\n", x, y);

	resize_display(x, y);
	fbsize(x, y);
}

static void keyb(int key, int press)
{
	int i;
	static int showstat;

	for(i=0; i<NUM_INPUTS; i++) {
		if(keymap[i][0] == key || keymap[i][1] == key) {
			inpstate[i] = press;
		}
	}

	if(press) {
		switch(key) {
		case '=':
			resizefb(3 * fb.width / 2, 3 * fb.height / 2);
			if(win_width < fb.width || win_height < fb.height) {
				glutReshapeWindow(fb.width, fb.height);
			}
			break;

		case '-':
			resizefb(2 * fb.width / 3, 2 * fb.height / 3);
			break;

		case '0':
			auto_res ^= 1;
			printf("%s resolution\n", auto_res ? "auto" : "manual");
			if(auto_res && (fb.width != win_width || fb.height != win_height)) {
				resizefb(win_width, win_height);
			}
			break;

		case '`':
			showstat ^= 1;
			show_statui(showstat);
			break;
		}

	}
}

static void keydown(unsigned char key, int x, int y)
{
	if(key == 27) exit(0);
	keyb(key, 1);
}

static void keyup(unsigned char key, int x, int y)
{
	keyb(key, 0);
}

static void skeydown(int key, int x, int y)
{
	keyb(key | 0x100, 1);
}

static void skeyup(int key, int x, int y)
{
	keyb(key | 0x100, 0);
}

static void mouse(int bn, int st, int x, int y)
{
	mouse_x = x;
	mouse_y = y;
	bnstate[bn - GLUT_LEFT_BUTTON] = st == GLUT_DOWN ? 1 : 0;
}

static void motion(int x, int y)
{
	int dx = x - mouse_x;
	int dy = y - mouse_y;
	mouse_x = x;
	mouse_y = y;

	if(!(dx | dy)) return;

	if(bnstate[0]) {
		cam_theta -= dx * 0.01;
		cam_phi -= dy * 0.01;

		if(cam_phi < -M_PI) cam_phi = -M_PI;
		if(cam_phi > M_PI) cam_phi = M_PI;

		cur_sample = 0;
	}
}

