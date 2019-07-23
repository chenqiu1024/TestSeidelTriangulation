#include "triangulate.h"
#include <sys/time.h>
#include <string.h>

static int initialise(SeidelTriangulator* state, int n)
{
  register int i;

  for (i = 1; i <= n; i++)
    state->seg[i].is_inserted = FALSE;

  generate_random_ordering(state, n);
  
  return 0;
}

void SeidelTriangulatorReset(SeidelTriangulator* state) {
    memset(state->qs, 0, sizeof(node_t) * state->qSize);
    memset(state->tr, 0, sizeof(trap_t) * state->trSize);
    memset(state->seg, 0, sizeof(segment_t) * state->segSize);
        
    state->q_idx = 0;
    state->tr_idx = 0;
        
    state->choose_idx = 0;
    memset(state->permute, 0, sizeof(int) * state->segSize);
        
    memset(state->mchain, 0, sizeof(monchain_t) * state->trSize);
        
    memset(state->vert, 0, sizeof(vertexchain_t) * state->segSize);
        
    memset(state->mon, 0, sizeof(int) * state->segSize);
        
    memset(state->visited, 0, sizeof(int) * state->trSize);
        
    state->chain_idx = 0;
    state->op_idx = 0;
    state->mon_idx = 0;
}

SeidelTriangulator* SeidelTriangulatorCreate(int n) {
    SeidelTriangulator* ret = (SeidelTriangulator*) malloc(sizeof(SeidelTriangulator));
    /*
    ret->segSize = SEGSIZE;///!!!n + 1;
    ret->qSize = 8 * ret->segSize;
    ret->trSize = 4 * ret->segSize;
    /*/
    ret->segSize = SEGSIZE;
    ret->qSize = QSIZE;
    ret->trSize = TRSIZE;
    //*/
    ret->qs = (node_t*) malloc(sizeof(node_t) * ret->qSize);        /* Query structure */ //qSize
    ret->tr = (trap_t*) malloc(sizeof(trap_t) * ret->trSize);        /* Trapezoid structure */ //trSize
    ret->seg = (segment_t*) malloc(sizeof(segment_t) * ret->segSize);        /* Segment table */ //SEGSIZE
    
    ret->permute = (int*) malloc(sizeof(int) * ret->segSize);//SEGSIZE
    
    ret->mchain = (monchain_t*) malloc(sizeof(monchain_t) * ret->trSize); //TRSIZE /* Table to hold all the monotone */
    /* polygons . Each monotone polygon */
    /* is a circularly linked list */
    
    ret->vert = (vertexchain_t*) malloc(sizeof(vertexchain_t) * ret->segSize); //SEGSIZE /* chain init. information. This */
    /* is used to decide which */
    /* monotone polygon to split if */
    /* there are several other */
    /* polygons touching at the same */
    /* vertex  */
    
    ret->mon = (int*) malloc(sizeof(int) * ret->segSize); //SEGSIZE  /* contains position of any vertex in */
    /* the monotone chain for the polygon */
    
    ret->visited = (int*) malloc(sizeof(int) * ret->trSize); //TRSIZE
    
    SeidelTriangulatorReset(ret);
    
    return ret;
}

void SeidelTriangulatorRelease(SeidelTriangulator* state) {
    if (!state) return;
    free(state->qs);
    free(state->tr);
    free(state->seg);
    free(state->permute);
    free(state->mchain);
    free(state->vert);
    free(state->mon);
    free(state->visited);
    free(state);
}

#ifdef STANDALONE

int main(argc, argv)
     int argc;
     char *argv[];
{
  int n, nmonpoly, genus;
  int i, ntriangles;
//    int op[SEGSIZE][3];

  if ((argc < 2) || ((n = read_segments(argv[1], &genus)) < 0))
    {
      fprintf(stderr, "usage: triangulate <filename>\n");
      exit(1);
    }

  initialise(state, n);
  construct_trapezoids(state, n);
  nmonpoly = monotonate_trapezoids(state, n);
  ntriangles = triangulate_monotone_polygons(state, n, nmonpoly, op);

  for (i = 0; i < ntriangles; i++)
    printf("triangle #%d: %d %d %d\n", i, 
	   state->op[i][0], state->op[i][1], state->op[i][2]);

  return 0;
}


#else  /* Not standalone. Use this as an interface routine */


/* Input specified as contours.
 * Outer contour must be anti-clockwise.
 * All inner contours must be clockwise.
 *  
 * Every contour is specified by giving all its points in order. No
 * point shoud be repeated. i.e. if the outer contour is a square,
 * only the four distinct endpoints shopudl be specified in order.
 *  
 * ncontours: #contours
 * cntr: An array describing the number of points in each
 *	 contour. Thus, cntr[i] = #points in the i'th contour.
 * vertices: Input array of vertices. Vertices for each contour
 *           immediately follow those for previous one. Array location
 *           vertices[0] must NOT be used (i.e. i/p starts from
 *           vertices[1] instead. The output triangles are
 *	     specified  w.r.t. the indices of these vertices.
 * triangles: Output array to hold triangles.
 *  
 * Enough space must be allocated for all the arrays before calling
 * this routine
 */
void triangulate_polygon(SeidelTriangulator** inoutTriangulatorPtr, int ncontours, int cntr[], double (*vertices)[2], int (*triangles)[3])
{
  register int i;
  int nmonpoly, ccount, npoints, genus;
  int n;

    SeidelTriangulator* state = *inoutTriangulatorPtr;
    if (NULL == state)
    {
        int vertexCount = 0;
        for (int c=0; c<ncontours; c++) vertexCount += cntr[c];
        state = SeidelTriangulatorCreate(vertexCount);
        *inoutTriangulatorPtr = state;
    }
    else
    {
        SeidelTriangulatorReset(state);
    }
    
  ccount = 0;
  i = 1;
  
  while (ccount < ncontours)
    {
      int j;
      int first, last;

      npoints = cntr[ccount];
      first = i;
      last = first + npoints - 1;
      for (j = 0; j < npoints; j++, i++)
	{
	  state->seg[i].v0.x = vertices[i][0];
	  state->seg[i].v0.y = vertices[i][1];

	  if (i == last)
	    {
	      state->seg[i].next = first;
	      state->seg[i].prev = i-1;
	      state->seg[i-1].v1 = state->seg[i].v0;
	    }
	  else if (i == first)
	    {
	      state->seg[i].next = i+1;
	      state->seg[i].prev = last;
	      state->seg[last].v1 = state->seg[i].v0;
	    }
	  else
	    {
	      state->seg[i].prev = i-1;
	      state->seg[i].next = i+1;
	      state->seg[i-1].v1 = state->seg[i].v0;
	    }
	  
	  state->seg[i].is_inserted = FALSE;
	}
      
      ccount++;
    }
  
  genus = ncontours - 1;
  n = i-1;

  initialise(state, n);
  construct_trapezoids(state, n);
  nmonpoly = monotonate_trapezoids(state, n);
  triangulate_monotone_polygons(state, n, nmonpoly, triangles);
}


/* This function returns TRUE or FALSE depending upon whether the 
 * vertex is inside the polygon or not. The polygon must already have
 * been triangulated before this routine is called.
 * This routine will always detect all the points belonging to the 
 * set (polygon-area - polygon-boundary). The return value for points 
 * on the boundary is not consistent!!!
 */

int is_point_inside_polygon(SeidelTriangulator* state, double vertex[2])
{
  point_t v;
  int trnum, rseg;
  trap_t *t;

  v.x = vertex[0];
  v.y = vertex[1];
  
  trnum = locate_endpoint(state, &v, &v, 1);
  t = &state->tr[trnum];
  
  if (t->state == ST_INVALID)
    return FALSE;
  
  if ((t->lseg <= 0) || (t->rseg <= 0))
    return FALSE;
  rseg = t->rseg;
  return _greater_than_equal_to(&state->seg[rseg].v1, &state->seg[rseg].v0);
}


#endif /* STANDALONE */
