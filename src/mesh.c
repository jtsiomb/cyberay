#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <cgmath/cgmath.h>
#include "mesh.h"

struct facevertex {
	int vidx, tidx, nidx;
};

struct objmtl {
	char name[64];
	cgm_vec3 ka, kd, ks;
	float shin;
	float alpha;
	float ior;
	struct objmtl *next;
};

static void calc_face_normal(struct triangle *tri);
static char *cleanline(char *s);
static char *parse_idx(char *ptr, int *idx, int arrsz);
static char *parse_face_vert(char *ptr, struct facevertex *fv, int numv, int numt, int numn);

static struct objmtl *load_mtllib(const char *objfname, const char *mtlfname);
static void free_mtllist(struct objmtl *mtl);
static void conv_mtl(struct material *mm, struct objmtl *om);

#define GROW_ARRAY(arr, sz)	\
	do { \
		int newsz = (sz) ? (sz) * 2 : 16; \
		void *tmp = realloc(arr, newsz * sizeof *(arr)); \
		if(!tmp) { \
			fprintf(stderr, "failed to grow array to %d\n", newsz); \
			goto fail; \
		} \
		arr = tmp; \
		sz = newsz; \
	} while(0)


int load_scenefile(struct scenefile *scn, const char *fname)
{
	int i, nlines, total_faces = 0, res = -1;
	FILE *fp;
	char buf[256], *line, *ptr;
	int varr_size, varr_max, narr_size, narr_max, tarr_size, tarr_max, max_faces;
	cgm_vec3 v, *varr = 0, *narr = 0;
	cgm_vec2 *tarr = 0;
	struct facevertex fv[4];
	int numfv;
	struct mesh *mesh;
	struct triangle *tri;
	static const cgm_vec2 def_tc = {0, 0};
	struct objmtl curmtl, *mtl, *mtllist = 0;


	varr_size = varr_max = narr_size = narr_max = tarr_size = tarr_max = 0;
	varr = narr = 0;
	tarr = 0;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "load_scenefile: failed to open %s\n", fname);
		return -1;
	}

	if(!(mesh = calloc(1, sizeof *mesh))) {
		fprintf(stderr, "failed to allocate mesh\n");
		fclose(fp);
		return -1;
	}
	max_faces = 0;

	scn->meshlist = 0;
	scn->num_meshes = 0;

	/* default material: white diffuse */
	memset(&curmtl, 0, sizeof curmtl);
	cgm_vcons(&curmtl.kd, 1.0f, 1.0f, 1.0f);
	curmtl.alpha = curmtl.ior = 1.0f;

	nlines = 0;
	while(fgets(buf, sizeof buf, fp)) {
		nlines++;
		if(!(line = cleanline(buf))) {
			continue;
		}

		switch(line[0]) {
		case 'v':
			v.x = v.y = v.z = 0.0f;
			if(sscanf(line + 2, "%f %f %f", &v.x, &v.y, &v.z) < 2) {
				break;
			}
			if(isspace(line[1])) {
				if(varr_size >= varr_max) {
					GROW_ARRAY(varr, varr_max);
				}
				varr[varr_size++] = v;
			} else if(line[1] == 't' && isspace(line[2])) {
				if(tarr_size >= tarr_max) {
					GROW_ARRAY(tarr, tarr_max);
				}
				tarr[tarr_size++] = *(cgm_vec2*)&v;
			} else if(line[1] == 'n' && isspace(line[2])) {
				if(narr_size >= narr_max) {
					GROW_ARRAY(narr, narr_max);
				}
				narr[narr_size++] = v;
			}
			break;

		case 'f':
			if(!isspace(line[1])) break;

			ptr = line + 2;

			numfv = 0;
			for(i=0; i<4; i++) {
				if(!(ptr = parse_face_vert(ptr, fv + i, varr_size, tarr_size, narr_size))) {
					break;
				}
				numfv++;
			}
			if(numfv < 3) break;

			if(mesh->num_faces >= max_faces - 1) {
				GROW_ARRAY(mesh->faces, max_faces);
			}
			tri = mesh->faces + mesh->num_faces++;
			tri->mtl = &mesh->mtl;

			tri->v[0].pos = varr[fv[0].vidx];
			tri->v[1].pos = varr[fv[1].vidx];
			tri->v[2].pos = varr[fv[2].vidx];
			calc_face_normal(tri);
			for(i=0; i<3; i++) {
				tri->v[i].norm = fv[i].nidx >= 0 ? narr[fv[i].nidx] : tri->norm;
				tri->v[i].tex = fv[i].tidx >= 0 ? tarr[fv[i].tidx] : def_tc;
			}

			if(numfv > 3) {
				tri++;
				mesh->num_faces++;
				tri->mtl = &mesh->mtl;
				tri->norm = tri[-1].norm;
				tri->v[0] = tri[-1].v[0];
				tri->v[1] = tri[-1].v[1];

				tri->v[2].pos = varr[fv[3].vidx];
				tri->v[2].norm = fv[3].nidx >= 0 ? narr[fv[3].nidx] : tri->norm;
				tri->v[2].tex = fv[3].tidx >= 0 ? tarr[fv[3].tidx] : def_tc;
			}
			break;

		case 'o':
		case 'g':
			if(mesh->num_faces) {
				conv_mtl(&mesh->mtl, &curmtl);
				total_faces += mesh->num_faces;
				mesh->next = scn->meshlist;
				scn->meshlist = mesh;
				scn->num_meshes++;

				printf("added mesh with mtl: %s\n", curmtl.name);

				if(!(mesh = calloc(1, sizeof *mesh))) {
					fprintf(stderr, "failed to allocate mesh\n");
					goto fail;
				}
				max_faces = 0;
			}
			break;

		case 'm':
			if(memcmp(line, "mtllib", 6) == 0 && (line = cleanline(line + 6))) {
				free_mtllist(mtllist);
				mtllist = load_mtllib(fname, line);
			}
			break;

		case 'u':
			if(memcmp(line, "usemtl", 6) == 0 && (line = cleanline(line + 6))) {
				mtl = mtllist;
				while(mtl) {
					if(strcmp(mtl->name, line) == 0) {
						curmtl = *mtl;
						break;
					}
					mtl = mtl->next;
				}
			}
			break;

		default:
			break;
		}
	}

	if(mesh->num_faces) {
		conv_mtl(&mesh->mtl, &curmtl);
		total_faces += mesh->num_faces;
		mesh->next = scn->meshlist;
		scn->meshlist = mesh;
		scn->num_meshes++;

		printf("added mesh with mtl: %s\n", curmtl.name);
	} else {
		free(mesh);
	}
	mesh = 0;

	printf("load_scenefile: loaded %d meshes, %d vertices, %d triangles\n", scn->num_meshes,
			varr_size, total_faces);

	res = 0;

fail:
	fclose(fp);
	free(mesh);
	free(varr);
	free(narr);
	free(tarr);
	free_mtllist(mtllist);
	return res;
}

void destroy_scenefile(struct scenefile *scn)
{
	struct mesh *m;
	while(scn->meshlist) {
		m = scn->meshlist;
		scn->meshlist = scn->meshlist->next;
		free(m);
	}
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
		glNormal3fv((float*)&m->faces[i].v[0].norm);
		glVertex3fv((float*)&m->faces[i].v[0].pos);

		glNormal3fv((float*)&m->faces[i].v[1].norm);
		glVertex3fv((float*)&m->faces[i].v[1].pos);

		glNormal3fv((float*)&m->faces[i].v[2].norm);
		glVertex3fv((float*)&m->faces[i].v[2].pos);
	}
	glEnd();
}

static void calc_face_normal(struct triangle *tri)
{
	cgm_vec3 va, vb;

	va = tri->v[1].pos;
	cgm_vsub(&va, &tri->v[0].pos);
	vb = tri->v[2].pos;
	cgm_vsub(&vb, &tri->v[0].pos);

	cgm_vcross(&tri->norm, &va, &vb);
	cgm_vnormalize(&tri->norm);
}

static char *cleanline(char *s)
{
	char *ptr;

	if((ptr = strchr(s, '#'))) *ptr = 0;

	while(*s && isspace(*s)) s++;
	ptr = s + strlen(s) - 1;
	while(ptr >= s && isspace(*ptr)) *ptr-- = 0;

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
	fv->tidx = fv->nidx = -1;

	if(!(ptr = parse_idx(ptr, &fv->vidx, numv)))
		return 0;
	if(*ptr != '/') return (!*ptr || isspace(*ptr)) ? ptr : 0;

	if(*++ptr == '/') {	/* no texcoord */
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

static struct objmtl *load_mtllib(const char *objfname, const char *mtlfname)
{
	FILE *fp;
	char *sep;
	char buf[256], *line;
	struct objmtl *mlist = 0, *m = 0;

	strcpy(buf, objfname);
	if((sep = strrchr(buf, '/'))) {
		sep[1] = 0;
	} else {
		buf[0] = 0;
	}
	strcat(buf, mtlfname);

	if(!(fp = fopen(buf, "rb"))) {
		return 0;
	}

	while(fgets(buf, sizeof buf, fp)) {
		if(!(line = cleanline(buf))) {
			continue;
		}

		if(memcmp(line, "newmtl", 6) == 0) {
			if(m) {
				m->next = mlist;
				mlist = m;
			}
			if((m = calloc(1, sizeof *m))) {
				if((line = cleanline(line + 6))) {
					strcpy(m->name, line);
				}
			}
		} else if(memcmp(line, "Kd", 2) == 0) {
			if(m) {
				sscanf(line + 3, "%f %f %f", &m->kd.x, &m->kd.y, &m->kd.z);
			}
		}
	}

	if(m) {
		m->next = mlist;
		mlist = m;
	}

	fclose(fp);
	return mlist;
}

static void free_mtllist(struct objmtl *mtl)
{
	while(mtl) {
		void *tmp = mtl;
		mtl = mtl->next;
		free(tmp);
	}
}

static void conv_mtl(struct material *mm, struct objmtl *om)
{
	mm->color = om->kd;
	/* TODO */
}
