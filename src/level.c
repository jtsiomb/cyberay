#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "level.h"
#include "treestore.h"
#include "mesh.h"

static struct material *add_material(struct level *lvl, struct material *mtl);
static int append_polygons(struct bvhnode *bnode, struct triangle *faces, int num_faces, struct material *mtl);

int load_level(struct level *lvl, const char *fname)
{
	char *dirname, *ptr;
	char path[256];
	struct ts_node *root, *node;
	struct scenefile scn;
	struct mesh *mesh;
	struct material *mtl;

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
			mesh = scn.meshlist;
			while(mesh) {
				mtl = add_material(lvl, &mesh->mtl);
				append_polygons(lvl->st_root, mesh->faces, mesh->num_faces, mtl);
				mesh = mesh->next;
			}

			destroy_scenefile(&scn);
		}
cont:	node = node->next;
	}

	ts_free_tree(root);
	return 0;
}

void destroy_level(struct level *lvl)
{
	free_bvh_tree(lvl->st_root);
	free_bvh_tree(lvl->dyn_root);
	free(lvl->mtls);
}


int ray_level(cgm_ray *ray, struct level *lvl, float tmax, struct rayhit *hit)
{
	return 0;
}

static void draw_level_rec(struct bvhnode *bn)
{
	int i, j;
	struct triangle *tri;
	struct material *curmtl;
	float color[4] = {0, 0, 0, 1};

	if(bn->faces) {
		tri = bn->faces;
		curmtl = tri->mtl;

		glBegin(GL_TRIANGLES);
		for(i=0; i<bn->num_faces; i++) {
			if(tri->mtl != curmtl) {
				glEnd();
				color[0] = tri->mtl->color.x;
				color[1] = tri->mtl->color.y;
				color[2] = tri->mtl->color.z;
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
				curmtl = tri->mtl;
				glBegin(GL_TRIANGLES);
			}

			for(j=0; j<3; j++) {
				glNormal3fv(&tri->v[j].norm.x);
				glTexCoord2fv(&tri->v[j].tex.x);
				glVertex3fv(&tri->v[j].pos.x);
			}
			tri++;
		}
		glEnd();
	}

	bn = bn->sub;
	while(bn) {
		draw_level_rec(bn);
		bn = bn->next;
	}
}

void draw_level(struct level *lvl)
{
	draw_level_rec(lvl->st_root);
	draw_level_rec(lvl->dyn_root);
}

static struct material *add_material(struct level *lvl, struct material *mtl)
{
	int i, newsz;
	struct material *tmp;

	for(i=0; i<lvl->num_mtls; i++) {
		if(memcmp(lvl->mtls + i, mtl, sizeof *mtl) == 0) {
			return lvl->mtls + i;
		}
	}

	if(lvl->num_mtls >= lvl->max_mtls) {
		newsz = lvl->max_mtls ? lvl->max_mtls * 2 : 16;
		if(!(tmp = realloc(lvl->mtls, newsz * sizeof *lvl->mtls))) {
			fprintf(stderr, "add_material: failed to resize materials array to %d\n", newsz);
			return 0;
		}
		lvl->mtls = tmp;
		lvl->max_mtls = newsz;
	}
	lvl->mtls[lvl->num_mtls] = *mtl;

	return lvl->mtls + lvl->num_mtls++;
}

static int append_polygons(struct bvhnode *bnode, struct triangle *faces, int num_faces, struct material *mtl)
{
	int i, j, newsz;
	struct triangle *tri;

	newsz = bnode->num_faces + num_faces;
	if(!(tri = realloc(bnode->faces, newsz * sizeof *bnode->faces))) {
		fprintf(stderr, "append_polygons: failed to resize faces array to %d\n", newsz);
		return -1;
	}
	bnode->faces = tri;
	tri += bnode->num_faces;
	bnode->num_faces = newsz;

	for(i=0; i<num_faces; i++) {
		*tri = *faces++;
		tri->mtl = mtl;

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
