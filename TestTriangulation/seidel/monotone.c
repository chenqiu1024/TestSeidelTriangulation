#include "triangulate.h"
#include <math.h>
#include <string.h>

#define CROSS_SINE(v0, v1) ((v0).x * (v1).y - (v1).x * (v0).y)
#define LENGTH(v0) (sqrt((v0).x * (v0).x + (v0).y * (v0).y))


static int triangulate_single_polygon(SeidelTriangulator*, int, int, int, int (*)[3]);
static int traverse_polygon(SeidelTriangulator*, int, int, int, int);

/* Function returns TRUE if the trapezoid lies inside the polygon */
static int inside_polygon(SeidelTriangulator* state, trap_t* t)
{
  int rseg = t->rseg;

  if (t->state == ST_INVALID)
    return 0;

  if ((t->lseg <= 0) || (t->rseg <= 0))
    return 0;
  
  if (((t->u0 <= 0) && (t->u1 <= 0)) || 
      ((t->d0 <= 0) && (t->d1 <= 0))) /* triangle */
    return (_greater_than(&state->seg[rseg].v1, &state->seg[rseg].v0));
  
  return 0;
}


/* return a new mon structure from the table */
static int newmon(SeidelTriangulator* state)
{
  return ++state->mon_idx;
}


/* return a new chain element from the table */
static int new_chain_element(SeidelTriangulator* state)
{
  return ++state->chain_idx;
}


static double get_angle(vp0, vpnext, vp1)
     point_t *vp0;
     point_t *vpnext;
     point_t *vp1;
{
  point_t v0, v1;
  
  v0.x = vpnext->x - vp0->x;
  v0.y = vpnext->y - vp0->y;

  v1.x = vp1->x - vp0->x;
  v1.y = vp1->y - vp0->y;

  if (CROSS_SINE(v0, v1) >= 0)	/* sine is positive */
    return DOT(v0, v1)/LENGTH(v0)/LENGTH(v1);
  else
    return (-1.0 * DOT(v0, v1)/LENGTH(v0)/LENGTH(v1) - 2);
}


/* (v0, v1) is the new diagonal to be added to the polygon. Find which */
/* chain to use and return the positions of v0 and v1 in p and q */ 
static int get_vertex_positions(SeidelTriangulator* state, int v0, int v1, int* ip, int* iq)
{
  vertexchain_t *vp0, *vp1;
  register int i;
  double angle, temp;
  int tp = 0, tq = 0;

  vp0 = &state->vert[v0];
  vp1 = &state->vert[v1];
  
  /* p is identified as follows. Scan from (v0, v1) rightwards till */
  /* you hit the first segment starting from v0. That chain is the */
  /* chain of our interest */
  
  angle = -4.0;
  for (i = 0; i < 4; i++)
    {
      if (vp0->vnext[i] <= 0)
	continue;
      if ((temp = get_angle(&vp0->pt, &(state->vert[vp0->vnext[i]].pt),
			    &vp1->pt)) > angle)
	{
	  angle = temp;
	  tp = i;
	}
    }

  *ip = tp;

  /* Do similar actions for q */

  angle = -4.0;
  for (i = 0; i < 4; i++)
    {
      if (vp1->vnext[i] <= 0)
	continue;      
      if ((temp = get_angle(&vp1->pt, &(state->vert[vp1->vnext[i]].pt),
			    &vp0->pt)) > angle)
	{
	  angle = temp;
	  tq = i;
	}
    }

  *iq = tq;

  return 0;
}

  
/* v0 and v1 are specified in anti-clockwise order with respect to 
 * the current monotone polygon mcur. Split the current polygon into 
 * two polygons using the diagonal (v0, v1) 
 */
static int make_new_monotone_poly(SeidelTriangulator* state, int mcur, int v0, int v1)
{
  int p, q, ip, iq;
  int mnew = newmon(state);
  int i, j, nf0, nf1;
  vertexchain_t *vp0, *vp1;
  
  vp0 = &state->vert[v0];
  vp1 = &state->vert[v1];

  get_vertex_positions(state, v0, v1, &ip, &iq);

  p = vp0->vpos[ip];
  q = vp1->vpos[iq];

  /* At this stage, we have got the positions of v0 and v1 in the */
  /* desired chain. Now modify the linked lists */

  i = new_chain_element(state);	/* for the new list */
  j = new_chain_element(state);

  state->mchain[i].vnum = v0;
  state->mchain[j].vnum = v1;

  state->mchain[i].next = state->mchain[p].next;
  state->mchain[state->mchain[p].next].prev = i;
  state->mchain[i].prev = j;
  state->mchain[j].next = i;
  state->mchain[j].prev = state->mchain[q].prev;
  state->mchain[state->mchain[q].prev].next = j;

  state->mchain[p].next = q;
  state->mchain[q].prev = p;

  nf0 = vp0->nextfree;
  nf1 = vp1->nextfree;

  vp0->vnext[ip] = v1;

  vp0->vpos[nf0] = i;
  vp0->vnext[nf0] = state->mchain[state->mchain[i].next].vnum;
  vp1->vpos[nf1] = j;
  vp1->vnext[nf1] = v0;

  vp0->nextfree++;
  vp1->nextfree++;

#ifdef DEBUG_SEIDEL
  fprintf(stderr, "make_poly: mcur = %d, (v0, v1) = (%d, %d)\n", 
	  mcur, v0, v1);
  fprintf(stderr, "next posns = (p, q) = (%d, %d)\n", p, q);
#endif

  state->mon[mcur] = p;
  state->mon[mnew] = i;
  return mnew;
}

/* Main routine to get monotone polygons from the trapezoidation of 
 * the polygon.
 */

int monotonate_trapezoids(SeidelTriangulator* state, int n)
{
  register int i;
  int tr_start;

  memset((void *)state->vert, 0, sizeof(state->vert));
  memset((void *)state->visited, 0, sizeof(state->visited));
  memset((void *)state->mchain, 0, sizeof(state->mchain));
  memset((void *)state->mon, 0, sizeof(state->mon));
  
  /* First locate a trapezoid which lies inside the polygon */
  /* and which is triangular */
  for (i = 0; i < state->trSize; i++)
    if (inside_polygon(state, &state->tr[i]))
      break;
  tr_start = i;
  
  /* Initialise the mon data-structure and start spanning all the */
  /* trapezoids within the polygon */

#if 0
  for (i = 1; i <= n; i++)
    {
      state->mchain[i].prev = i - 1;
      state->mchain[i].next = i + 1;
      state->mchain[i].vnum = i;
      state->vert[i].pt = state->seg[i].v0;
      state->vert[i].vnext[0] = i + 1;	/* next vertex */
      state->vert[i].vpos[0] = i;	/* locn. of next vertex */
      state->vert[i].nextfree = 1;
    }
  state->mchain[1].prev = n;
  state->mchain[n].next = 1;
  state->vert[n].vnext[0] = 1;
  state->vert[n].vpos[0] = n;
  chain_idx = n;
  mon_idx = 0;
  state->mon[0] = 1;			/* position of any vertex in the first */
				/* chain  */

#else

  for (i = 1; i <= n; i++)
    {
      state->mchain[i].prev = state->seg[i].prev;
      state->mchain[i].next = state->seg[i].next;
      state->mchain[i].vnum = i;
      state->vert[i].pt = state->seg[i].v0;
      state->vert[i].vnext[0] = state->seg[i].next; /* next vertex */
      state->vert[i].vpos[0] = i;	/* locn. of next vertex */
      state->vert[i].nextfree = 1;
    }

  state->chain_idx = n;
  state->mon_idx = 0;
  state->mon[0] = 1;			/* position of any vertex in the first */
				/* chain  */

#endif
  
  /* traverse the polygon */
  if (state->tr[tr_start].u0 > 0)
    traverse_polygon(state, 0, tr_start, state->tr[tr_start].u0, TR_FROM_UP);
  else if (state->tr[tr_start].d0 > 0)
    traverse_polygon(state, 0, tr_start, state->tr[tr_start].d0, TR_FROM_DN);
  
  /* return the number of polygons created */
  return newmon(state);
}


/* recursively visit all the trapezoids */
static int traverse_polygon(SeidelTriangulator* state, int mcur, int trnum, int from, int dir)
{
  trap_t *t = &state->tr[trnum];
  int howsplit, mnew;
  int v0, v1, v0next, v1next;
  int retval, tmp;
  int do_switch = FALSE;

  if ((trnum <= 0) || state->visited[trnum])
    return 0;

  state->visited[trnum] = TRUE;
  
  /* We have much more information available here. */
  /* rseg: goes upwards   */
  /* lseg: goes downwards */

  /* Initially assume that dir = TR_FROM_DN (from the left) */
  /* Switch v0 and v1 if necessary afterwards */


  /* special cases for triangles with cusps at the opposite ends. */
  /* take care of this first */
  if ((t->u0 <= 0) && (t->u1 <= 0))
    {
      if ((t->d0 > 0) && (t->d1 > 0)) /* downward opening triangle */
	{
	  v0 = state->tr[t->d1].lseg;
	  v1 = t->lseg;
	  if (from == t->d1)
	    {
	      do_switch = TRUE;
	      mnew = make_new_monotone_poly(state, mcur, v1, v0);
	      traverse_polygon(state, mcur, t->d1, trnum, TR_FROM_UP);
	      traverse_polygon(state, mnew, t->d0, trnum, TR_FROM_UP);
	    }
	  else
	    {
	      mnew = make_new_monotone_poly(state, mcur, v0, v1);
	      traverse_polygon(state, mcur, t->d0, trnum, TR_FROM_UP);
	      traverse_polygon(state, mnew, t->d1, trnum, TR_FROM_UP);
	    }
	}
      else
	{
	  retval = SP_NOSPLIT;	/* Just traverse all neighbours */
	  traverse_polygon(state, mcur, t->u0, trnum, TR_FROM_DN);
	  traverse_polygon(state, mcur, t->u1, trnum, TR_FROM_DN);
	  traverse_polygon(state, mcur, t->d0, trnum, TR_FROM_UP);
	  traverse_polygon(state, mcur, t->d1, trnum, TR_FROM_UP);
	}
    }
  
  else if ((t->d0 <= 0) && (t->d1 <= 0))
    {
      if ((t->u0 > 0) && (t->u1 > 0)) /* upward opening triangle */
	{
	  v0 = t->rseg;
	  v1 = state->tr[t->u0].rseg;
	  if (from == t->u1)
	    {
	      do_switch = TRUE;
	      mnew = make_new_monotone_poly(state, mcur, v1, v0);
	      traverse_polygon(state, mcur, t->u1, trnum, TR_FROM_DN);
	      traverse_polygon(state, mnew, t->u0, trnum, TR_FROM_DN);
	    }
	  else
	    {
	      mnew = make_new_monotone_poly(state, mcur, v0, v1);
	      traverse_polygon(state, mcur, t->u0, trnum, TR_FROM_DN);
	      traverse_polygon(state, mnew, t->u1, trnum, TR_FROM_DN);
	    }
	}
      else
	{
	  retval = SP_NOSPLIT;	/* Just traverse all neighbours */
	  traverse_polygon(state, mcur, t->u0, trnum, TR_FROM_DN);
	  traverse_polygon(state, mcur, t->u1, trnum, TR_FROM_DN);
	  traverse_polygon(state, mcur, t->d0, trnum, TR_FROM_UP);
	  traverse_polygon(state, mcur, t->d1, trnum, TR_FROM_UP);
	}
    }
  
  else if ((t->u0 > 0) && (t->u1 > 0)) 
    {
      if ((t->d0 > 0) && (t->d1 > 0)) /* downward + upward cusps */
	{
	  v0 = state->tr[t->d1].lseg;
	  v1 = state->tr[t->u0].rseg;
	  retval = SP_2UP_2DN;
	  if (((dir == TR_FROM_DN) && (t->d1 == from)) ||
	      ((dir == TR_FROM_UP) && (t->u1 == from)))
	    {
	      do_switch = TRUE;
	      mnew = make_new_monotone_poly(state, mcur, v1, v0);
	      traverse_polygon(state, mcur, t->u1, trnum, TR_FROM_DN);
	      traverse_polygon(state, mcur, t->d1, trnum, TR_FROM_UP);
	      traverse_polygon(state, mnew, t->u0, trnum, TR_FROM_DN);
	      traverse_polygon(state, mnew, t->d0, trnum, TR_FROM_UP);
	    }
	  else
	    {
	      mnew = make_new_monotone_poly(state, mcur, v0, v1);
	      traverse_polygon(state, mcur, t->u0, trnum, TR_FROM_DN);
	      traverse_polygon(state, mcur, t->d0, trnum, TR_FROM_UP);
	      traverse_polygon(state, mnew, t->u1, trnum, TR_FROM_DN);
	      traverse_polygon(state, mnew, t->d1, trnum, TR_FROM_UP);
	    }
	}
      else			/* only downward cusp */
	{
	  if (_equal_to(&t->lo, &state->seg[t->lseg].v1))
	    {
	      v0 = state->tr[t->u0].rseg;
	      v1 = state->seg[t->lseg].next;

	      retval = SP_2UP_LEFT;
	      if ((dir == TR_FROM_UP) && (t->u0 == from))
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(state, mcur, v1, v0);
		  traverse_polygon(state, mcur, t->u0, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->d0, trnum, TR_FROM_UP);
		  traverse_polygon(state, mnew, t->u1, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->d1, trnum, TR_FROM_UP);
		}
	      else
		{
		  mnew = make_new_monotone_poly(state, mcur, v0, v1);
		  traverse_polygon(state, mcur, t->u1, trnum, TR_FROM_DN);
		  traverse_polygon(state, mcur, t->d0, trnum, TR_FROM_UP);
		  traverse_polygon(state, mcur, t->d1, trnum, TR_FROM_UP);
		  traverse_polygon(state, mnew, t->u0, trnum, TR_FROM_DN);
		}
	    }
	  else
	    {
	      v0 = t->rseg;
	      v1 = state->tr[t->u0].rseg;
	      retval = SP_2UP_RIGHT;
	      if ((dir == TR_FROM_UP) && (t->u1 == from))
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(state, mcur, v1, v0);
		  traverse_polygon(state, mcur, t->u1, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->d1, trnum, TR_FROM_UP);
		  traverse_polygon(state, mnew, t->d0, trnum, TR_FROM_UP);
		  traverse_polygon(state, mnew, t->u0, trnum, TR_FROM_DN);
		}
	      else
		{
		  mnew = make_new_monotone_poly(state, mcur, v0, v1);
		  traverse_polygon(state, mcur, t->u0, trnum, TR_FROM_DN);
		  traverse_polygon(state, mcur, t->d0, trnum, TR_FROM_UP);
		  traverse_polygon(state, mcur, t->d1, trnum, TR_FROM_UP);
		  traverse_polygon(state, mnew, t->u1, trnum, TR_FROM_DN);
		}
	    }
	}
    }
  else if ((t->u0 > 0) || (t->u1 > 0)) /* no downward cusp */
    {
      if ((t->d0 > 0) && (t->d1 > 0)) /* only upward cusp */
	{
	  if (_equal_to(&t->hi, &state->seg[t->lseg].v0))
	    {
	      v0 = state->tr[t->d1].lseg;
	      v1 = t->lseg;
	      retval = SP_2DN_LEFT;
	      if (!((dir == TR_FROM_DN) && (t->d0 == from)))
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(state, mcur, v1, v0);
		  traverse_polygon(state, mcur, t->u1, trnum, TR_FROM_DN);
		  traverse_polygon(state, mcur, t->d1, trnum, TR_FROM_UP);
		  traverse_polygon(state, mcur, t->u0, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->d0, trnum, TR_FROM_UP);
		}
	      else
		{
		  mnew = make_new_monotone_poly(state, mcur, v0, v1);
		  traverse_polygon(state, mcur, t->d0, trnum, TR_FROM_UP);
		  traverse_polygon(state, mnew, t->u0, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->u1, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->d1, trnum, TR_FROM_UP);
		}
	    }
	  else
	    {
	      v0 = state->tr[t->d1].lseg;
	      v1 = state->seg[t->rseg].next;

	      retval = SP_2DN_RIGHT;	    
	      if ((dir == TR_FROM_DN) && (t->d1 == from))
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(state, mcur, v1, v0);
		  traverse_polygon(state, mcur, t->d1, trnum, TR_FROM_UP);
		  traverse_polygon(state, mnew, t->u1, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->u0, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->d0, trnum, TR_FROM_UP);
		}
	      else
		{
		  mnew = make_new_monotone_poly(state, mcur, v0, v1);
		  traverse_polygon(state, mcur, t->u0, trnum, TR_FROM_DN);
		  traverse_polygon(state, mcur, t->d0, trnum, TR_FROM_UP);
		  traverse_polygon(state, mcur, t->u1, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->d1, trnum, TR_FROM_UP);
		}
	    }
	}
      else			/* no cusp */
	{
	  if (_equal_to(&t->hi, &state->seg[t->lseg].v0) &&
	      _equal_to(&t->lo, &state->seg[t->rseg].v0))
	    {
	      v0 = t->rseg;
	      v1 = t->lseg;
	      retval = SP_SIMPLE_LRDN;
	      if (dir == TR_FROM_UP)
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(state, mcur, v1, v0);
		  traverse_polygon(state, mcur, t->u0, trnum, TR_FROM_DN);
		  traverse_polygon(state, mcur, t->u1, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->d1, trnum, TR_FROM_UP);
		  traverse_polygon(state, mnew, t->d0, trnum, TR_FROM_UP);
		}
	      else
		{
		  mnew = make_new_monotone_poly(state, mcur, v0, v1);
		  traverse_polygon(state, mcur, t->d1, trnum, TR_FROM_UP);
		  traverse_polygon(state, mcur, t->d0, trnum, TR_FROM_UP);
		  traverse_polygon(state, mnew, t->u0, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->u1, trnum, TR_FROM_DN);
		}
	    }
	  else if (_equal_to(&t->hi, &state->seg[t->rseg].v1) &&
		   _equal_to(&t->lo, &state->seg[t->lseg].v1))
	    {
	      v0 = state->seg[t->rseg].next;
	      v1 = state->seg[t->lseg].next;

	      retval = SP_SIMPLE_LRUP;
	      if (dir == TR_FROM_UP)
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(state, mcur, v1, v0);
		  traverse_polygon(state, mcur, t->u0, trnum, TR_FROM_DN);
		  traverse_polygon(state, mcur, t->u1, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->d1, trnum, TR_FROM_UP);
		  traverse_polygon(state, mnew, t->d0, trnum, TR_FROM_UP);
		}
	      else
		{
		  mnew = make_new_monotone_poly(state, mcur, v0, v1);
		  traverse_polygon(state, mcur, t->d1, trnum, TR_FROM_UP);
		  traverse_polygon(state, mcur, t->d0, trnum, TR_FROM_UP);
		  traverse_polygon(state, mnew, t->u0, trnum, TR_FROM_DN);
		  traverse_polygon(state, mnew, t->u1, trnum, TR_FROM_DN);
		}
	    }
	  else			/* no split possible */
	    {
	      retval = SP_NOSPLIT;
	      traverse_polygon(state, mcur, t->u0, trnum, TR_FROM_DN);
	      traverse_polygon(state, mcur, t->d0, trnum, TR_FROM_UP);
	      traverse_polygon(state, mcur, t->u1, trnum, TR_FROM_DN);
	      traverse_polygon(state, mcur, t->d1, trnum, TR_FROM_UP);
	    }
	}
    }

  return retval;
}


/* For each monotone polygon, find the ymax and ymin (to determine the */
/* two y-monotone chains) and pass on this monotone polygon for greedy */
/* triangulation. */
/* Take care not to triangulate duplicate monotone polygons */

int triangulate_monotone_polygons(SeidelTriangulator* state, int nvert, int nmonpoly, int op[][3])
{
  register int i;
  point_t ymax, ymin;
  int p, vfirst, posmax, posmin, v;
  int vcount, processed;

#ifdef DEBUG_SEIDEL
  for (i = 0; i < nmonpoly; i++)
    {
      fprintf(stderr, "\n\nPolygon %d: ", i);
      vfirst = state->mchain[state->mon[i]].vnum;
      p = state->mchain[state->mon[i]].next;
      fprintf (stderr, "%d ", state->mchain[state->mon[i]].vnum);
      while (state->mchain[p].vnum != vfirst)
	{
	  fprintf(stderr, "%d ", state->mchain[p].vnum);
	  p = state->mchain[p].next;
	}
    }
  fprintf(stderr, "\n");
#endif

  state->op_idx = 0;
  for (i = 0; i < nmonpoly; i++)
    {
      vcount = 1;
      processed = FALSE;
      vfirst = state->mchain[state->mon[i]].vnum;
      ymax = ymin = state->vert[vfirst].pt;
      posmax = posmin = state->mon[i];
      state->mchain[state->mon[i]].marked = TRUE;
      p = state->mchain[state->mon[i]].next;
      while ((v = state->mchain[p].vnum) != vfirst)
	{
	 if (state->mchain[p].marked)
	   {
	     processed = TRUE;
	     break;		/* break from while */
	   }
	 else
	   state->mchain[p].marked = TRUE;

	  if (_greater_than(&state->vert[v].pt, &ymax))
	    {
	      ymax = state->vert[v].pt;
	      posmax = p;
	    }
	  if (_less_than(&state->vert[v].pt, &ymin))
	    {
	      ymin = state->vert[v].pt;
	      posmin = p;
	    }
	  p = state->mchain[p].next;
	  vcount++;
       }

      if (processed)		/* Go to next polygon */
	continue;
      
      if (vcount == 3)		/* already a triangle */
	{
	  op[state->op_idx][0] = state->mchain[p].vnum;
	  op[state->op_idx][1] = state->mchain[state->mchain[p].next].vnum;
	  op[state->op_idx][2] = state->mchain[state->mchain[p].prev].vnum;
	  state->op_idx++;
	}
      else			/* triangulate the polygon */
	{
	  v = state->mchain[state->mchain[posmax].next].vnum;
	  if (_equal_to(&state->vert[v].pt, &ymin))
	    {			/* LHS is a single line */
	      triangulate_single_polygon(state, nvert, posmax, TRI_LHS, op);
	    }
	  else
	    triangulate_single_polygon(state, nvert, posmax, TRI_RHS, op);
	}
    }
  
#ifdef DEBUG_SEIDEL
  for (i = 0; i < state->op_idx; i++)
    fprintf(stderr, "tri #%d: (%d, %d, %d)\n", i, op[i][0], op[i][1],
	   op[i][2]);
#endif
  return state->op_idx;
}


/* A greedy corner-cutting algorithm to triangulate a y-monotone 
 * polygon in O(n) time.
 * Joseph O-Rourke, Computational Geometry in C.
 */
static int triangulate_single_polygon(SeidelTriangulator* state, int nvert, int posmax, int side, int op[][3])
{
  register int v;
  int ri = 0;	/* reflex chain */
  int* rc = (int*)malloc(sizeof(int) * state->segSize);
  int endv, tmp, vpos;
  
  if (side == TRI_RHS)		/* RHS segment is a single segment */
    {
      rc[0] = state->mchain[posmax].vnum;
      tmp = state->mchain[posmax].next;
      rc[1] = state->mchain[tmp].vnum;
      ri = 1;
      
      vpos = state->mchain[tmp].next;
      v = state->mchain[vpos].vnum;
      
      if ((endv = state->mchain[state->mchain[posmax].prev].vnum) == 0)
	endv = nvert;
    }
  else				/* LHS is a single segment */
    {
      tmp = state->mchain[posmax].next;
      rc[0] = state->mchain[tmp].vnum;
      tmp = state->mchain[tmp].next;
      rc[1] = state->mchain[tmp].vnum;
      ri = 1;

      vpos = state->mchain[tmp].next;
      v = state->mchain[vpos].vnum;

      endv = state->mchain[posmax].vnum;
    }
  
  while ((v != endv) || (ri > 1))
    {
      if (ri > 0)		/* reflex chain is non-empty */
	{
	  if (CROSS(state->vert[v].pt, state->vert[rc[ri - 1]].pt,
		    state->vert[rc[ri]].pt) > 0)
	    {			/* convex corner: cut if off */
	      op[state->op_idx][0] = rc[ri - 1];
	      op[state->op_idx][1] = rc[ri];
	      op[state->op_idx][2] = v;
	      state->op_idx++;
	      ri--;
	    }
	  else		/* non-convex */
	    {		/* add v to the chain */
	      ri++;
	      rc[ri] = v;
	      vpos = state->mchain[vpos].next;
	      v = state->mchain[vpos].vnum;
	    }
	}
      else			/* reflex-chain empty: add v to the */
	{			/* reflex chain and advance it  */
	  rc[++ri] = v;
	  vpos = state->mchain[vpos].next;
	  v = state->mchain[vpos].vnum;
	}
    } /* end-while */
  
  /* reached the bottom vertex. Add in the triangle formed */
  op[state->op_idx][0] = rc[ri - 1];
  op[state->op_idx][1] = rc[ri];
  op[state->op_idx][2] = v;
  state->op_idx++;
  ri--;
  free(rc);
  return 0;
}


