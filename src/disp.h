#ifndef DISP_H_
#define DISP_H_

float exposure;

int init_display(void);
void cleanup_display(void);

void resize_display(int x, int y);

void display(void);

#endif	/* DISP_H_ */
