#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <GL/glut.h>
#include <cgmath/cgmath.h>
#include "mesh.h"

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
static void display(void);
static void idle(void);
static void reshape(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void keyup(unsigned char key, int x, int y);
static void skeydown(int key, int x, int y);
static void skeyup(int key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);


static long start_time;

static float cam_theta, cam_phi;
static cgm_vec3 cam_pos = {0, -1.6, 0};
static float pxform[16];

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

static struct mesh mesh;

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(1280, 800);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("cyberay");

	glutDisplayFunc(display);
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

	glutMainLoop();
	return 0;
}

static int init(void)
{
	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	if(load_mesh(&mesh, "data/testlvl.obj") == -1) {
		return -1;
	}

	start_time = glutGet(GLUT_ELAPSED_TIME);
	return 0;
}

static void cleanup(void)
{
	destroy_mesh(&mesh);
}

#define WALK_SPEED 3.0f
static void update(void)
{
	static unsigned int prev_upd;
	unsigned int msec;
	float dt, vfwd, vright;

	msec = glutGet(GLUT_ELAPSED_TIME) - start_time;
	dt = (float)(msec - prev_upd) / 1000.0f;
	prev_upd = msec;

	vfwd = vright = 0;

	if(inpstate[INP_FWD]) {
		vfwd -= WALK_SPEED * dt;
	}
	if(inpstate[INP_BACK]) {
		vfwd += WALK_SPEED * dt;
	}
	if(inpstate[INP_RIGHT]) {
		vright -= WALK_SPEED * dt;
	}
	if(inpstate[INP_LEFT]) {
		vright += WALK_SPEED * dt;
	}

	cam_pos.x += cos(cam_theta) * vright + sin(cam_theta) * vfwd;
	cam_pos.z += sin(cam_theta) * vright - cos(cam_theta) * vfwd;

	cgm_midentity(pxform);
	cgm_mtranslate(pxform, cam_pos.x, cam_pos.y, cam_pos.z);
	cgm_mrotate_y(pxform, cam_theta);
	cgm_mrotate_x(pxform, cam_phi);
}

static void display(void)
{
	update();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(pxform);

	glFrontFace(GL_CW);
	glutSolidTeapot(1.0);
	glFrontFace(GL_CCW);

	draw_mesh(&mesh);

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
}

static void idle(void)
{
	glutPostRedisplay();
}

static void reshape(int x, int y)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50.0, (float)x / (float)y, 0.5, 500.0);
}

static void keyb(int key, int press)
{
	int i;

	for(i=0; i<NUM_INPUTS; i++) {
		if(keymap[i][0] == key || keymap[i][1] == key) {
			inpstate[i] = press;
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
		cam_theta += dx * 0.01;
		cam_phi += dy * 0.01;

		if(cam_phi < -M_PI) cam_phi = -M_PI;
		if(cam_phi > M_PI) cam_phi = M_PI;
	}
}
