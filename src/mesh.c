#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <cgmath/cgmath.h>
#include "mesh.h"

struct facevertex {
	int vidx, tidx, nidx;
};

static char *cleanline(char *s)
{
	char *ptr;

	if((ptr = strchr(s, '#'))) *ptr = 0;

	while(*s && isspace(*s)) s++;
	ptr = s + strlen(s) - 1;
	while(ptr >= s && isspace(*s)) *ptr-- = 0;

	return *s ? s : 0;
}

static char *parse_idx(char *ptr, int *idx, int arrsz)
{
	char *endp;
	int val = strtol(ptr, &endp, 10);
	if(endp == ptr) return 0;

	if(val < 0) {	/* convert negative indices */
		*idx = arrsz + val;
	} else {
		*idx = val - 1;	/* indices in obj are 1-based */
	}
	return endp;
}

/* possible face-vertex definitions:
 * 1. vertex
 * 2. vertex/texcoord
 * 3. vertex//normal
 * 4. vertex/texcoord/normal
 */
static char *parse_face_vert(char *ptr, struct facevertex *fv, int numv, int numt, int numn)
{
	if(!(ptr = parse_idx(ptr, &fv->vidx, numv)))
		return 0;
	if(*ptr != '/') return (!*ptr || isspace(*ptr)) ? ptr : 0;

	if(*++ptr == '/') {	/* no texcoord */
		fv->tidx = -1;
		++ptr;
	} else {
		if(!(ptr = parse_idx(ptr, &fv->tidx, numt)))
			return 0;
		if(*ptr != '/') return (!*ptr || isspace(*ptr)) ? ptr : 0;
		++ptr;
	}

	if(!(ptr = parse_idx(ptr, &fv->nidx, numn)))
		return 0;
	return (!*ptr || isspace(*ptr)) ? ptr : 0;
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
	int i, num, nline, sidx, didx, res = -1;
	FILE *fp;
	cgm_vec3 v, va, vb, fnorm;
	cgm_vec3 *varr, *narr, *tarr;
	int vcount, ncount, tcount, vmax, nmax, tmax, max_faces, newsz;
	char linebuf[256], *line, *ptr;
	struct facevertex fv[4];
	struct triangle *tri;
	void *tmpptr;
	static const int quadidx[] = { 0, 1, 2, 0, 1, 3 };

	varr = narr = tarr = 0;
	vcount = ncount = tcount = vmax = nmax = tmax = 0;

	m->faces = 0;
	m->num_faces = 0;
	max_faces = 0;

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
			if((num = sscanf(line + 2, "%f %f %f", &v.x, &v.y, &v.z)) < 2) {
verr:			fprintf(stderr, "load_mesh: ignoring malformed attribute at %d: %s\n", nline, line);
				continue;
			}
			if(isspace(line[1])) {
				APPEND(v);
				varr[vcount++] = v;
			} else if(line[1] == 'n' && isspace(line[2])) {
				APPEND(n);
				narr[ncount++] = v;
			} else if(line[1] == 't' && isspace(line[2])) {
				APPEND(t);
				tarr[tcount].x = v.x;
				tarr[tcount++].y = v.y;
			} else {
				goto verr;
			}
			break;

		case 'f':
			if(!isspace(line[1])) {
				break;
			}
			ptr = line + 2;
			for(i=0; i<4; i++) {
				fv[i].nidx = fv[i].tidx = -1;
				if(!(ptr = parse_face_vert(ptr, fv + i, vcount, tcount, ncount))) {
					break;
				}
			}

			if(i < 2) {
				fprintf(stderr, "load_mesh: invalid face definition at %d: %s\n", nline, line);
				break;
			}

			va = varr[fv[1].vidx];
			cgm_vsub(&va, varr + fv[0].vidx);
			vb = varr[fv[2].vidx];
			cgm_vsub(&vb, varr + fv[0].vidx);
			cgm_vcross(&fnorm, &va, &vb);
			cgm_vnormalize(&fnorm);

			if(m->num_faces >= max_faces - 1) {
				newsz = max_faces ? max_faces * 2 : 16;
				if(!(tmpptr = realloc(m->faces, newsz * sizeof *m->faces))) {
					fprintf(stderr, "load_mesh: failed to resize faces array to %d\n", newsz);
					goto end;
				}
				m->faces = tmpptr;
				max_faces = newsz;
			}

			num = i > 3 ? 6 : 3;
			for(i=0; i<num; i++) {
				if(i % 3 == 0) {
					tri = m->faces + m->num_faces++;
					tri->norm = fnorm;
					tri->mtl = &m->mtl;
				}
				sidx = quadidx[i];
				didx = i >= 3 ? i - 3 : i;
				tri->v[didx].pos = varr[fv[sidx].vidx];
				tri->v[didx].norm = fv[sidx].nidx >= 0 ? varr[fv[sidx].nidx] : fnorm;
				if(fv[sidx].tidx >= 0) {
					tri->v[didx].tex.x = tarr[fv[sidx].tidx].x;
					tri->v[didx].tex.y = tarr[fv[sidx].tidx].y;
				} else {
					tri->v[didx].tex.x = tri->v[sidx].tex.y = 0;
				}
			}
			break;
		}

	}

	res = 0;
end:
	free(varr);
	free(narr);
	free(tarr);
	fclose(fp);
	return res;
}

void destroy_mesh(struct mesh *m)
{
	free(m->faces);
	m->faces = 0;
}

void draw_mesh(struct mesh *m)
{
	int i;

	glBegin(GL_TRIANGLES);
	for(i=0; i<m->num_faces; i++) {
		glNormal3fv(&m->faces[i].v[0].norm);
		glVertex3fv(&m->faces[i].v[0].pos);

		glNormal3fv(&m->faces[i].v[1].norm);
		glVertex3fv(&m->faces[i].v[1].pos);

		glNormal3fv(&m->faces[i].v[2].norm);
		glVertex3fv(&m->faces[i].v[2].pos);
	}
	glEnd();
}
