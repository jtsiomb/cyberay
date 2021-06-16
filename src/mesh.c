#include <stdio.h>
#include <stdlib.h>
#include "mesh.h"

static char *cleanline(char *s)
{
	char *ptr;

	if((ptr = strchr(s, '#'))) *ptr = 0;

	while(*s && isspace(*s)) s++;
	ptr = s + strlen(s) - 1;
	while(ptr >= s && isspace(*s)) *ptr-- = 0;

	return *s ? s : 0;
}

#define APPEND(prefix)	\
	do { \
		if(prefix##count >= prefix##max) { \
			int newsz = prefix##max ? prefix##max * 2 : 8; \
			void *ptr = realloc(prefix##arr, newsz * sizeof(cgm_vec3)); \
			if(!ptr) { \
				fprintf(stderr, "load_mesh: failed to resize array to %d elements\n", newsz); \
				return -1; \
			} \
			prefix##arr = ptr; \
			prefix##max = newsz; \
		} \
	} while(0)


int load_mesh(struct mesh *m, const char *fname)
{
	int num, nline;
	FILE *fp;
	cgm_vec3 v;
	cgm_vec3 *varr, *narr, *tarr;
	int vcount, ncount, tcount, vmax, nmax, tmax;
	char linebuf[256], *line;

	varr = narr = tarr = 0;
	vcount = ncount = tcount = vmax = nmax = tmax = 0;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "load_mesh: failed to open: %s\n", fname);
		return -1;
	}

	nline = 0;
	while(fgets(linebuf, sizeof linebuf, fp)) {
		nline++;
		if(!(line = cleanline(linebuf))) {
			continue;
		}

		switch(line[0]) {
		case 'v':
			v.y = v.z = 0;
			if((num = sscanf(line + 1, "%f %f %f", &v.x, &v.y, &v.z)) < 2) {
verr:			fprintf(stderr, "load_mesh: ignoring malformed attribute at %d: %s\n", nline, line);
				continue;
			}
			if(isspace(line[1])) {
				APPEND(v);
				varr[vcount++] = v;
			} else if(line[1] == 'n') {
				APPEND(n);
				narr[ncount++] = v;
			} else if(line[1] == 't') {
				APPEND(t);
				tarr[ncount].x = v.x;
				tarr[ncount++].y = v.y;
			} else {
				goto verr;
			}
			break;

			/* TODO cont */
		}

	}

	fclose(fp);
	return 0;
}

void draw_mesh(struct mesh *m)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof *m->faces, &m->faces->v[0].pos.x);
	glNormalPointer(GL_FLOAT, sizeof *m->faces, &m->faces->v[0].norm.x);
	glTexCoordPointer(2, GL_FLOAT, sizeof *m->faces, &m->faces->v[0].tex.x);

	glDrawArrays(GL_TRIANGLES, 0, m->num_faces * 3);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}
