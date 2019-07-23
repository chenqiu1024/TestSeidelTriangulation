#ifndef __interface_h
#define __interface_h

#include "triangulate.h"

#define TRUE 1
#define FALSE 0

#ifdef __cplusplus
extern "C" {
#endif

SeidelTriangulator* SeidelTriangulatorCreate(int n);
void SeidelTriangulatorRelease(SeidelTriangulator* state);

void triangulate_polygon(SeidelTriangulator**, int, int *, double (*)[2], int (*)[3]);
int is_point_inside_polygon(double *);

#ifdef __cplusplus
}
#endif

#endif /* __interface_h */
