#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "level.h"
#include "game.h"
#include "treestore.h"
#include "mesh.h"

static int add_mesh_faces(struct bvhnode *bnode, struct mesh *mesh);
static void proc_edits(struct ts_node *snode, struct scenefile *scn);
static int edit_mtl(struct ts_node *node, const char *mtlname, const char *mtlprop, struct scenefile *scn);

int load_level(struct level *lvl, const char *fname)
{
	char *dirname, *ptr;
	char path[256];
	struct ts_node *root, *node;
	struct scenefile scn;
	struct mesh *mesh, *tail;
	unsigned long start_time;
	float *vec;

	memset(lvl, 0, sizeof *lvl);
	if(!(lvl->st_root = calloc(1, sizeof *lvl->st_root)) ||
			!(lvl->dyn_root = calloc(1, sizeof *lvl->dyn_root))) {
		free(lvl->st_root);
		fprintf(stderr, "load_level: failed to allocate bvh root nodes\n");
		return -1;
	}

	cgm_vcons(&lvl->st_root->aabb.vmin, FLT_MAX, FLT_MAX, FLT_MAX);
	cgm_vcons(&lvl->st_root->aabb.vmax, -FLT_MAX, -FLT_MAX, -FLT_MAX);
	lvl->dyn_root->aabb = lvl->st_root->aabb;

	dirname = alloca(strlen(fname) + 1);
	strcpy(dirname, fname);
	if((ptr = strrchr(dirname, '/'))) {
		ptr[1] = 0;
	} else {
		*dirname = 0;
	}

	if(!(root = ts_load(fname))) {
		fprintf(stderr, "load_level: failed to load: %s\n", fname);
		return -1;
	}
	if(strcmp(root->name, "level") != 0) {
		fprintf(stderr, "load_level: invalid level file %s, root is not \"level\"\n", fname);
		ts_free_tree(root);
		return -1;
	}

	/* lookup environment properties */
	if((vec = ts_lookup_vec(root, "level.env.color", 0))) {
		lvl->bgcolor.x = vec[0];
		lvl->bgcolor.y = vec[1];
		lvl->bgcolor.z = vec[2];
	}

	/* load scene files */
	node = root->child_list;
	while(node) {
		if(strcmp(node->name, "scene") == 0) {
			if(!(fname = ts_get_attr_str(node, "file", 0))) {
				fprintf(stderr, "load_level: ignoring \"scene\" without a \"file\" attribute\n");
				goto cont;
			}
			snprintf(path, sizeof path, "%s%s", dirname, fname);
			printf("loading scene file: %s\n", path);

			if(load_scenefile(&scn, path) == -1) {
				goto cont;
			}

			/* perform any edits on the loaded scene */
			proc_edits(node, &scn);

			mesh = tail = scn.meshlist;
			while(mesh) {
				add_mesh_faces(lvl->st_root, mesh);
				tail = mesh;
				mesh = mesh->next;
			}

			if(tail) {
				tail->next = lvl->meshlist;
				lvl->meshlist = scn.meshlist;
				scn.meshlist = 0;
			}
			destroy_scenefile(&scn);
		}
cont:	node = node->next;
	}
	ts_free_tree(root);

	printf("Building static BVH tree\n");
	start_time = get_msec();
	if(build_bvh_sah(lvl->st_root) == -1) {
		return -1;
	}
	printf("BVH construction took: %lu msec\n", get_msec() - start_time);
	return 0;
}

void destroy_level(struct level *lvl)
{
	struct mesh *mesh;

	free_bvh_tree(lvl->st_root);
	free_bvh_tree(lvl->dyn_root);

	while(lvl->meshlist) {
		mesh = lvl->meshlist;
		lvl->meshlist = lvl->meshlist->next;
		destroy_mesh(mesh);
	}
}


int ray_level(cgm_ray *ray, struct level *lvl, float tmax, struct rayhit *hit)
{
	int found = 0;
	struct rayhit hit0;

	if(!hit) {
		if(ray_bvhnode(ray, lvl->st_root, tmax, 0)) return 1;
		if(ray_bvhnode(ray, lvl->dyn_root, tmax, 0)) return 1;
		return 0;
	}

	hit0.t = FLT_MAX;
	if(ray_bvhnode(ray, lvl->st_root, tmax, hit)) {
		hit0 = *hit;
		found = 1;
	}
	if(ray_bvhnode(ray, lvl->dyn_root, tmax, hit) && hit->t < hit0.t) {
		hit0 = *hit;
		found = 1;
	}

	if(found) {
		*hit = hit0;
		return 1;
	}
	return 0;
}

static void draw_level_rec(struct bvhnode *bn)
{
	int i, j;
	struct triangle *tri;
	struct material *curmtl;
	float color[4] = {0, 0, 0, 1};

	if(!bn) return;

	if(bn->faces) {
		curmtl = bn->faces[0]->mtl;

		glBegin(GL_TRIANGLES);
		for(i=0; i<bn->num_faces; i++) {
			tri = bn->faces[i];
			if(tri->mtl != curmtl) {
				glEnd();
				color[0] = tri->mtl->attr[MATTR_COLOR].value.x;
				color[1] = tri->mtl->attr[MATTR_COLOR].value.y;
				color[2] = tri->mtl->attr[MATTR_COLOR].value.z;
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
				curmtl = tri->mtl;
				glBegin(GL_TRIANGLES);
			}

			for(j=0; j<3; j++) {
				glNormal3fv(&tri->v[j].norm.x);
				glTexCoord2fv(&tri->v[j].tex.x);
				glVertex3fv(&tri->v[j].pos.x);
			}
		}
		glEnd();
	}

	draw_level_rec(bn->left);
	draw_level_rec(bn->right);
}

void draw_level(struct level *lvl)
{
	draw_level_rec(lvl->st_root);
	draw_level_rec(lvl->dyn_root);
}

static int add_mesh_faces(struct bvhnode *bnode, struct mesh *mesh)
{
	int i, j, newsz;
	void *tmp;
	struct triangle *tri, **triptr;

	newsz = bnode->num_faces + mesh->num_faces;
	if(!(tmp = realloc(bnode->faces, newsz * sizeof *bnode->faces))) {
		fprintf(stderr, "append_polygons: failed to resize faces array to %d\n", newsz);
		return -1;
	}
	bnode->faces = tmp;
	triptr = bnode->faces + bnode->num_faces;
	bnode->num_faces = newsz;

	tri = mesh->faces;
	for(i=0; i<mesh->num_faces; i++) {
		*triptr++ = tri;
		for(j=0; j<3; j++) {
			cgm_vec3 *p = &tri->v[j].pos;
			if(p->x < bnode->aabb.vmin.x) bnode->aabb.vmin.x = p->x;
			if(p->x > bnode->aabb.vmax.x) bnode->aabb.vmax.x = p->x;
			if(p->y < bnode->aabb.vmin.y) bnode->aabb.vmin.y = p->y;
			if(p->y > bnode->aabb.vmax.y) bnode->aabb.vmax.y = p->y;
			if(p->z < bnode->aabb.vmin.z) bnode->aabb.vmin.z = p->z;
			if(p->z > bnode->aabb.vmax.z) bnode->aabb.vmax.z = p->z;
		}
		tri++;
	}
	return 0;
}

static void proc_edits(struct ts_node *snode, struct scenefile *scn)
{
	const char *mtlname, *mtlprop;
	struct ts_node *node;

	node = snode->child_list;
	while(node) {
		if(strcmp(node->name, "mtledit") == 0) {
			if(!(mtlname = ts_get_attr_str(node, "name", 0))) {
				fprintf(stderr, "proc_edits: name attribute missing\n");
				goto next;
			}
			if(!(mtlprop = ts_get_attr_str(node, "prop", 0))) {
				fprintf(stderr, "proc_edits: prop attribute missing\n");
				goto next;
			}
			edit_mtl(node, mtlname, mtlprop, scn);
		}
next:	node = node->next;
	}
}

enum { OP_SET, OP_SCALE };

struct {
	char *name;
	int op;
} mtlops[] = {
	{"set", OP_SET},
	{"scale", OP_SCALE},
	{0, 0}
};

static void mtlop_vec(int op, float *vptr, float *data, int nelem)
{
	switch(op) {
	case OP_SET:
		while(nelem--) *vptr++ = *data++;
		break;
	case OP_SCALE:
		while(nelem--) *vptr++ *= *data++;
		break;
	}
}

static int edit_mtl(struct ts_node *node, const char *mtlname, const char *mtlprop, struct scenefile *scn)
{
	int i, op;
	char *name;
	struct mesh *mesh;
	struct material *mtl = 0;
	struct ts_attr *opattr = 0;
	float opval[3] = {0};

	for(i=0; mtlops[i].name; i++) {
		if((opattr = ts_get_attr(node, mtlops[i].name))) {
			op = mtlops[i].op;
			break;
		}
	}
	if(!opattr) {
		fprintf(stderr, "mtl edit: missing or invalid operation\n");
		return -1;
	}

	switch(opattr->val.type) {
	case TS_NUMBER:
		opval[0] = opval[1] = opval[2] = opattr->val.fnum;
		break;
	case TS_VECTOR:
		opval[0] = opattr->val.vec[0];
		opval[1] = opattr->val.vec[1];
		opval[2] = opattr->val.vec[2];
	default:
		break;
	}

	/* find the first instance of the named material */
	mesh = scn->meshlist;
	while(mesh) {
		if(strcmp(mesh->mtl.name, mtlname) == 0) {
			mtl = &mesh->mtl;
			break;
		}
		mesh = mesh->next;
	}
	if(!mtl) {
		fprintf(stderr, "mtl edit: no matching material: %s\n", mtlname);
		return -1;
	}

	if(strcmp(mtlprop, "color") == 0) {
		mtlop_vec(op, &mtl->attr[MATTR_COLOR].value.x, opval, 3);
	} else if(strcmp(mtlprop, "emit") == 0) {
		mtlop_vec(op, &mtl->attr[MATTR_EMIT].value.x, opval, 3);
	} else if(strcmp(mtlprop, "roughness") == 0) {
		mtlop_vec(op, &mtl->attr[MATTR_ROUGHNESS].value.x, opval, 1);
	} else if(strcmp(mtlprop, "ior") == 0) {
		mtlop_vec(op, &mtl->ior, opval, 1);
	} else if(strcmp(mtlprop, "transmit") == 0) {
		mtlop_vec(op, &mtl->attr[MATTR_TRANSMIT].value.x, opval, 1);
	} else if(strcmp(mtlprop, "metal") == 0) {
		if(op != OP_SET || opattr->val.type != TS_NUMBER) {
			fprintf(stderr, "mtl edit: invalid operation or attribute type for metal\n");
			return -1;
		}
		mtl->metal = opattr->val.inum != 0;
	} else if(strcmp(mtlprop, "texture") == 0) {
		if(op != OP_SET) {
			fprintf(stderr, "mtl edit: invalid operation for textures\n");
			return -1;
		}
		/* TODO: lookup in the texture db/load, and set pointer */
	}

	/* copy the modified material to every other instance */
	mesh = mesh->next;
	while(mesh) {
		if(strcmp(mesh->mtl.name, mtlname) == 0) {
			name = mesh->mtl.name;
			mesh->mtl = *mtl;
			mesh->mtl.name = name;	/* don't clobber the strdup'ed name */
		}
		mesh = mesh->next;
	}
	return 0;
}
