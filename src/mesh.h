#ifndef MESH_H_
#define MESH_H_

#include <GL/gl.h>
#include <cgmath/cgmath.h>
#include "geom.h"

struct mesh {
	struct triangle *faces;
	int num_faces;

	struct aabox aabb;

	struct material mtl;
	struct mesh *next;
};

struct scenefile {
	struct mesh *meshlist;
	int num_meshes;
};

int load_scenefile(struct scenefile *scn, const char *fname);
void destroy_scenefile(struct scenefile *scn);

void destroy_mesh(struct mesh *m);
void draw_mesh(struct mesh *m);

#endif	/* MESH_H_ */
