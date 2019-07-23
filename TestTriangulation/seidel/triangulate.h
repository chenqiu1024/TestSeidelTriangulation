#ifndef _triangulate_h
#define _triangulate_h

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
  double x, y;
} point_t, vector_t;


/* Segment attributes */

typedef struct {	
  point_t v0, v1;		/* two endpoints */
  int is_inserted;		/* inserted in trapezoidation yet ? */
  int root0, root1;		/* root nodes in Q */
  int next;			/* Next logical segment */
  int prev;			/* Previous segment */
} segment_t;


/* Trapezoid attributes */

typedef struct {
  int lseg, rseg;		/* two adjoining segments */
  point_t hi, lo;		/* max/min y-values */
  int u0, u1;
  int d0, d1;
  int sink;			/* pointer to corresponding in Q */
  int usave, uside;		/* I forgot what this means */
  int state;
} trap_t;


/* Node attributes for every node in the query structure */

typedef struct {
  int nodetype;			/* Y-node or S-node */
  int segnum;
  point_t yval;
  int trnum;
  int parent;			/* doubly linked DAG */
  int left, right;		/* children */
} node_t;


typedef struct {
  int vnum;
  int next;			/* Circularly linked list  */
  int prev;			/* describing the monotone */
  int marked;			/* polygon */
} monchain_t;			


typedef struct {
  point_t pt;
  int vnext[4];			/* next vertices for the 4 chains */
  int vpos[4];			/* position of v in the 4 chains */
  int nextfree;
} vertexchain_t;

#define SEGSIZE 200        /* max# of segments. Determines how */
/* many points can be specified as */
/* input. If your datasets have large */
/* number of points, increase this */
/* value accordingly. */

#define QSIZE   8*SEGSIZE    /* maximum table sizes */
#define TRSIZE  4*SEGSIZE    /* max# trapezoids */

//#define FIX_SIZED_ARRAY
//#define GLOBAL_TRIANGULATOR

typedef struct {
    int qSize;
    int trSize;
    int segSize;
#ifdef FIX_SIZED_ARRAY
    node_t qs[QSIZE];        /* Query structure */
    trap_t tr[TRSIZE];        /* Trapezoid structure */
    segment_t seg[SEGSIZE];        /* Segment table */
#else
    node_t* qs;//[QSIZE];        /* Query structure */
    trap_t* tr;//[TRSIZE];        /* Trapezoid structure */
    segment_t* seg;//[SEGSIZE];        /* Segment table */
#endif
    int q_idx;
    int tr_idx;
    
    int choose_idx;
#ifdef FIX_SIZED_ARRAY
    int permute[SEGSIZE];
        
    monchain_t mchain[TRSIZE]; /* Table to hold all the monotone */
    /* polygons . Each monotone polygon */
    /* is a circularly linked list */
        
    vertexchain_t vert[SEGSIZE]; /* chain init. information. This */
    /* is used to decide which */
    /* monotone polygon to split if */
    /* there are several other */
    /* polygons touching at the same */
    /* vertex  */
        
    int mon[SEGSIZE]; /* contains position of any vertex in */
    /* the monotone chain for the polygon */
    int visited[TRSIZE];
#else
    int* permute;//[SEGSIZE];
    
    monchain_t* mchain;//[TRSIZE]; /* Table to hold all the monotone */
    /* polygons . Each monotone polygon */
    /* is a circularly linked list */
    
    vertexchain_t* vert;//[SEGSIZE]; /* chain init. information. This */
    /* is used to decide which */
    /* monotone polygon to split if */
    /* there are several other */
    /* polygons touching at the same */
    /* vertex  */
    
    int* mon;//[SEGSIZE]; /* contains position of any vertex in */
    /* the monotone chain for the polygon */
    int* visited;//[TRSIZE];
#endif
    int chain_idx, op_idx, mon_idx;
    
} SeidelTriangulator;

#ifdef GLOBAL_TRIANGULATOR
extern SeidelTriangulator g_seidel;
#endif

/* Node types */

#define T_X     1
#define T_Y     2
#define T_SINK  3


#define TRUE  1
#define FALSE 0


#define FIRSTPT 1		/* checking whether pt. is inserted */ 
#define LASTPT  2


#define INFINITY 1<<30
#define C_EPS 1.0e-7		/* tolerance value: Used for making */
				/* all decisions about collinearity or */
				/* left/right of segment. Decrease */
				/* this value if the input points are */
				/* spaced very close together */


#define S_LEFT 1		/* for merge-direction */
#define S_RIGHT 2


#define ST_VALID 1		/* for trapezium state */
#define ST_INVALID 2


#define SP_SIMPLE_LRUP 1	/* for splitting trapezoids */
#define SP_SIMPLE_LRDN 2
#define SP_2UP_2DN     3
#define SP_2UP_LEFT    4
#define SP_2UP_RIGHT   5
#define SP_2DN_LEFT    6
#define SP_2DN_RIGHT   7
#define SP_NOSPLIT    -1	

#define TR_FROM_UP 1		/* for traverse-direction */
#define TR_FROM_DN 2

#define TRI_LHS 1
#define TRI_RHS 2


#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define CROSS(v0, v1, v2) (((v1).x - (v0).x)*((v2).y - (v0).y) - \
			   ((v1).y - (v0).y)*((v2).x - (v0).x))

#define DOT(v0, v1) ((v0).x * (v1).x + (v0).y * (v1).y)

#define FP_EQUAL(s, t) (fabs(s - t) <= C_EPS)


/* Functions */
#ifdef __cplusplus
extern "C" {
#endif

int monotonate_trapezoids(SeidelTriangulator*, int);
int triangulate_monotone_polygons(SeidelTriangulator*, int, int, int (*)[3]);

int _greater_than(point_t *, point_t *);
int _equal_to(point_t *, point_t *);
int _greater_than_equal_to(point_t *, point_t *);
int _less_than(point_t *, point_t *);
int locate_endpoint(SeidelTriangulator*, point_t *, point_t *, int);
int construct_trapezoids(SeidelTriangulator*, int);

int generate_random_ordering(SeidelTriangulator*, int);
int choose_segment(SeidelTriangulator*);
int read_segments(char *, int *);
int math_logstar_n(int);
int math_N(int, int);
    
#ifdef __cplusplus
}
#endif

#endif /* triangulate_h */
