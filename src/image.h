#ifndef IMAGE_H_
#define IMAGE_H_

struct image {
	int width, height;
	float *pixels;
};

int load_image(struct image *img, const char *fname);
void destroy_image(struct image *img);

#endif	/* IMAGE_H_ */
