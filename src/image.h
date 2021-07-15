#ifndef IMAGE_H_
#define IMAGE_H_

struct image {
	int width, height;
	unsigned int xmask, ymask, xshift;
	float *pixels;
};

int load_image(struct image *img, const char *fname);
void destroy_image(struct image *img);

int add_image(const char *name, struct image *img);
struct image *get_image(const char *name);

#endif	/* IMAGE_H_ */
