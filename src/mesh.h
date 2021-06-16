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
};

int load_mesh(struct mesh *m, const char *fname);
void draw_mesh(struct mesh *m);

#endif	/* MESH_H_ */
