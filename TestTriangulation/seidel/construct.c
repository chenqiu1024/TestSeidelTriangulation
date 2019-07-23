#include "triangulate.h"
#include <math.h>
#include <string.h>

/* Return a new node to be added into the query tree */
static int newnode(SeidelTriangulator* state)
{
  if (state->q_idx < state->qSize)
    return state->q_idx++;
  else
    {
      fprintf(stderr, "newnode(state): Query-table overflow\n");
      return -1;
    }
}

/* Return a free trapezoid */
static int newtrap(SeidelTriangulator* state)
{
  if (state->tr_idx < state->trSize)
    {
      state->tr[state->tr_idx].lseg = -1;
      state->tr[state->tr_idx].rseg = -1;
      state->tr[state->tr_idx].state = ST_VALID;
      return state->tr_idx++;
    }
  else
    {
      fprintf(stderr, "newtrap: Trapezoid-table overflow\n");
      return -1;
    }
}


/* Return the maximum of the two points into the yval structure */
static int _max(yval, v0, v1)
     point_t *yval;
     point_t *v0;
     point_t *v1;
{
  if (v0->y > v1->y + C_EPS)
    *yval = *v0;
  else if (FP_EQUAL(v0->y, v1->y))
    {
      if (v0->x > v1->x + C_EPS)
	*yval = *v0;
      else
	*yval = *v1;
    }
  else
    *yval = *v1;
  
  return 0;
}


/* Return the minimum of the two points into the yval structure */
static int _min(yval, v0, v1)
     point_t *yval;
     point_t *v0;
     point_t *v1;
{
  if (v0->y < v1->y - C_EPS)
    *yval = *v0;
  else if (FP_EQUAL(v0->y, v1->y))
    {
      if (v0->x < v1->x)
	*yval = *v0;
      else
	*yval = *v1;
    }
  else
    *yval = *v1;
  
  return 0;
}


int _greater_than(v0, v1)
     point_t *v0;
     point_t *v1;
{
  if (v0->y > v1->y + C_EPS)
    return TRUE;
  else if (v0->y < v1->y - C_EPS)
    return FALSE;
  else
    return (v0->x > v1->x);
}


int _equal_to(v0, v1)
     point_t *v0;
     point_t *v1;
{
  return (FP_EQUAL(v0->y, v1->y) && FP_EQUAL(v0->x, v1->x));
}

int _greater_than_equal_to(v0, v1)
     point_t *v0;
     point_t *v1;
{
  if (v0->y > v1->y + C_EPS)
    return TRUE;
  else if (v0->y < v1->y - C_EPS)
    return FALSE;
  else
    return (v0->x >= v1->x);
}

int _less_than(v0, v1)
     point_t *v0;
     point_t *v1;
{
  if (v0->y < v1->y - C_EPS)
    return TRUE;
  else if (v0->y > v1->y + C_EPS)
    return FALSE;
  else
    return (v0->x < v1->x);
}


/* Initilialise the query structure (Q) and the trapezoid table (T) 
 * when the first segment is added to start the trapezoidation. The
 * query-tree starts out with 4 trapezoids, one S-node and 2 Y-nodes
 *    
 *                4
 *   -----------------------------------
 *  		  \
 *  	1	   \        2
 *  		    \
 *   -----------------------------------
 *                3
 */

static int init_query_structure(SeidelTriangulator* state, int segnum)
{
  int i1, i2, i3, i4, i5, i6, i7, root;
  int t1, t2, t3, t4;
  segment_t *s = &state->seg[segnum];

  state->q_idx = state->tr_idx = 1;
  memset((void *)state->tr, 0, sizeof(state->tr[0]) * state->trSize);
  memset((void *)state->qs, 0, sizeof(state->qs[0]) * state->qSize);

  i1 = newnode(state);
  state->qs[i1].nodetype = T_Y;
  _max(&state->qs[i1].yval, &s->v0, &s->v1); /* root */
  root = i1;

  state->qs[i1].right = i2 = newnode(state);
  state->qs[i2].nodetype = T_SINK;
  state->qs[i2].parent = i1;

  state->qs[i1].left = i3 = newnode(state);
  state->qs[i3].nodetype = T_Y;
  _min(&state->qs[i3].yval, &s->v0, &s->v1); /* root */
  state->qs[i3].parent = i1;
  
  state->qs[i3].left = i4 = newnode(state);
  state->qs[i4].nodetype = T_SINK;
  state->qs[i4].parent = i3;
  
  state->qs[i3].right = i5 = newnode(state);
  state->qs[i5].nodetype = T_X;
  state->qs[i5].segnum = segnum;
  state->qs[i5].parent = i3;
  
  state->qs[i5].left = i6 = newnode(state);
  state->qs[i6].nodetype = T_SINK;
  state->qs[i6].parent = i5;

  state->qs[i5].right = i7 = newnode(state);
  state->qs[i7].nodetype = T_SINK;
  state->qs[i7].parent = i5;

  t1 = newtrap(state);		/* middle left */
  t2 = newtrap(state);		/* middle right */
  t3 = newtrap(state);		/* bottom-most */
  t4 = newtrap(state);		/* topmost */

  state->tr[t1].hi = state->tr[t2].hi = state->tr[t4].lo = state->qs[i1].yval;
  state->tr[t1].lo = state->tr[t2].lo = state->tr[t3].hi = state->qs[i3].yval;
  state->tr[t4].hi.y = (double) (INFINITY);
  state->tr[t4].hi.x = (double) (INFINITY);
  state->tr[t3].lo.y = (double) -1* (INFINITY);
  state->tr[t3].lo.x = (double) -1* (INFINITY);
  state->tr[t1].rseg = state->tr[t2].lseg = segnum;
  state->tr[t1].u0 = state->tr[t2].u0 = t4;
  state->tr[t1].d0 = state->tr[t2].d0 = t3;
  state->tr[t4].d0 = state->tr[t3].u0 = t1;
  state->tr[t4].d1 = state->tr[t3].u1 = t2;
  
  state->tr[t1].sink = i6;
  state->tr[t2].sink = i7;
  state->tr[t3].sink = i4;
  state->tr[t4].sink = i2;

  state->tr[t1].state = state->tr[t2].state = ST_VALID;
  state->tr[t3].state = state->tr[t4].state = ST_VALID;

  state->qs[i2].trnum = t4;
  state->qs[i4].trnum = t3;
  state->qs[i6].trnum = t1;
  state->qs[i7].trnum = t2;

  s->is_inserted = TRUE;
  return root;
}


/* Retun TRUE if the vertex v is to the left of line segment no.
 * segnum. Takes care of the degenerate cases when both the vertices
 * have the same y--cood, etc.
 */

static int is_left_of(SeidelTriangulator* state, int segnum, point_t *v)
{
  segment_t *s = &state->seg[segnum];
  double area;
  
  if (_greater_than(&s->v1, &s->v0)) /* seg. going upwards */
    {
      if (FP_EQUAL(s->v1.y, v->y))
	{
	  if (v->x < s->v1.x)
	    area = 1.0;
	  else
	    area = -1.0;
	}
      else if (FP_EQUAL(s->v0.y, v->y))
	{
	  if (v->x < s->v0.x)
	    area = 1.0;
	  else
	    area = -1.0;
	}
      else
	area = CROSS(s->v0, s->v1, (*v));
    }
  else				/* v0 > v1 */
    {
      if (FP_EQUAL(s->v1.y, v->y))
	{
	  if (v->x < s->v1.x)
	    area = 1.0;
	  else
	    area = -1.0;
	}
      else if (FP_EQUAL(s->v0.y, v->y))
	{
	  if (v->x < s->v0.x)
	    area = 1.0;
	  else
	    area = -1.0;
	}
      else
	area = CROSS(s->v1, s->v0, (*v));
    }
  
  if (area > 0.0)
    return TRUE;
  else 
    return FALSE;
}



/* Returns true if the corresponding endpoint of the given segment is */
/* already inserted into the segment tree. Use the simple test of */
/* whether the segment which shares this endpoint is already inserted */

static int inserted(SeidelTriangulator* state, int segnum, int whichpt)
{
  if (whichpt == FIRSTPT)
    return state->seg[state->seg[segnum].prev].is_inserted;
  else
    return state->seg[state->seg[segnum].next].is_inserted;
}

/* This is query routine which determines which trapezoid does the 
 * point v lie in. The return value is the trapezoid number. 
 */

int locate_endpoint(SeidelTriangulator* state, point_t *v, point_t *vo, int r)
{
  node_t *rptr = &state->qs[r];
  
  switch (rptr->nodetype)
    {
    case T_SINK:
      return rptr->trnum;
      
    case T_Y:
      if (_greater_than(v, &rptr->yval)) /* above */
	return locate_endpoint(state, v, vo, rptr->right);
      else if (_equal_to(v, &rptr->yval)) /* the point is already */
	{			          /* inserted. */
	  if (_greater_than(vo, &rptr->yval)) /* above */
	    return locate_endpoint(state, v, vo, rptr->right);
	  else 
	    return locate_endpoint(state, v, vo, rptr->left); /* below */
	}
      else
	return locate_endpoint(state, v, vo, rptr->left); /* below */

    case T_X:
      if (_equal_to(v, &state->seg[rptr->segnum].v0) ||
	       _equal_to(v, &state->seg[rptr->segnum].v1))
	{
	  if (FP_EQUAL(v->y, vo->y)) /* horizontal segment */
	    {
	      if (vo->x < v->x)
		return locate_endpoint(state, v, vo, rptr->left); /* left */
	      else
		return locate_endpoint(state, v, vo, rptr->right); /* right */
	    }

	  else if (is_left_of(state, rptr->segnum, vo))
	    return locate_endpoint(state, v, vo, rptr->left); /* left */
	  else
	    return locate_endpoint(state, v, vo, rptr->right); /* right */
	}
      else if (is_left_of(state, rptr->segnum, v))
	return locate_endpoint(state, v, vo, rptr->left); /* left */
      else
	return locate_endpoint(state, v, vo, rptr->right); /* right */

    default:
      fprintf(stderr, "Haggu !!!!!\n");
      break;
    }
    return -1;
}


/* Thread in the segment into the existing trapezoidation. The 
 * limiting trapezoids are given by tfirst and tlast (which are the
 * trapezoids containing the two endpoints of the segment. Merges all
 * possible trapezoids which flank this segment and have been recently
 * divided because of its insertion
 */

static int merge_trapezoids(SeidelTriangulator* state, int segnum, int tfirst, int tlast, int side)
{
  int t, tnext, cond;
  int ptnext;

  /* First merge polys on the LHS */
  t = tfirst;
  while ((t > 0) && _greater_than_equal_to(&state->tr[t].lo, &state->tr[tlast].lo))
    {
      if (side == S_LEFT)
	cond = ((((tnext = state->tr[t].d0) > 0) && (state->tr[tnext].rseg == segnum)) ||
		(((tnext = state->tr[t].d1) > 0) && (state->tr[tnext].rseg == segnum)));
      else
	cond = ((((tnext = state->tr[t].d0) > 0) && (state->tr[tnext].lseg == segnum)) ||
		(((tnext = state->tr[t].d1) > 0) && (state->tr[tnext].lseg == segnum)));
      
      if (cond)
	{
	  if ((state->tr[t].lseg == state->tr[tnext].lseg) &&
	      (state->tr[t].rseg == state->tr[tnext].rseg)) /* good neighbours */
	    {			              /* merge them */
	      /* Use the upper node as the new node i.e. t */
	      
	      ptnext = state->qs[state->tr[tnext].sink].parent;
	      
	      if (state->qs[ptnext].left == state->tr[tnext].sink)
		state->qs[ptnext].left = state->tr[t].sink;
	      else
		state->qs[ptnext].right = state->tr[t].sink;	/* redirect parent */
	      
	      
	      /* Change the upper neighbours of the lower trapezoids */
	      
	      if ((state->tr[t].d0 = state->tr[tnext].d0) > 0)
		if (state->tr[state->tr[t].d0].u0 == tnext)
		  state->tr[state->tr[t].d0].u0 = t;
		else if (state->tr[state->tr[t].d0].u1 == tnext)
		  state->tr[state->tr[t].d0].u1 = t;
	      
	      if ((state->tr[t].d1 = state->tr[tnext].d1) > 0)
		if (state->tr[state->tr[t].d1].u0 == tnext)
		  state->tr[state->tr[t].d1].u0 = t;
		else if (state->tr[state->tr[t].d1].u1 == tnext)
		  state->tr[state->tr[t].d1].u1 = t;
	      
	      state->tr[t].lo = state->tr[tnext].lo;
	      state->tr[tnext].state = ST_INVALID; /* invalidate the lower */
				            /* trapezium */
	    }
	  else		    /* not good neighbours */
	    t = tnext;
	}
      else		    /* do not satisfy the outer if */
	t = tnext;
      
    } /* end-while */
       
  return 0;
}


/* Add in the new segment into the trapezoidation and update Q and T
 * structures. First locate the two endpoints of the segment in the
 * Q-structure. Then start from the topmost trapezoid and go down to
 * the  lower trapezoid dividing all the trapezoids in between .
 */

static int add_segment(SeidelTriangulator* state, int segnum)
{
  segment_t s;
  segment_t *so = &state->seg[segnum];
  int tu, tl, sk, tfirst, tlast, tnext;
  int tfirstr, tlastr, tfirstl, tlastl;
  int i1, i2, t, t1, t2, tn;
  point_t tpt;
  int tritop = 0, tribot = 0, is_swapped = 0;
  int tmptriseg;

  s = state->seg[segnum];
  if (_greater_than(&s.v1, &s.v0)) /* Get higher vertex in v0 */
    {
      int tmp;
      tpt = s.v0;
      s.v0 = s.v1;
      s.v1 = tpt;
      tmp = s.root0;
      s.root0 = s.root1;
      s.root1 = tmp;
      is_swapped = TRUE;
    }

  if ((is_swapped) ? !inserted(state, segnum, LASTPT) :
       !inserted(state, segnum, FIRSTPT))     /* insert v0 in the tree */
    {
      int tmp_d;

      tu = locate_endpoint(state, &s.v0, &s.v1, s.root0);
      tl = newtrap(state);		/* tl is the new lower trapezoid */
      state->tr[tl].state = ST_VALID;
      state->tr[tl] = state->tr[tu];
      state->tr[tu].lo.y = state->tr[tl].hi.y = s.v0.y;
      state->tr[tu].lo.x = state->tr[tl].hi.x = s.v0.x;
      state->tr[tu].d0 = tl;
      state->tr[tu].d1 = 0;
      state->tr[tl].u0 = tu;
      state->tr[tl].u1 = 0;

      if (((tmp_d = state->tr[tl].d0) > 0) && (state->tr[tmp_d].u0 == tu))
	state->tr[tmp_d].u0 = tl;
      if (((tmp_d = state->tr[tl].d0) > 0) && (state->tr[tmp_d].u1 == tu))
	state->tr[tmp_d].u1 = tl;

      if (((tmp_d = state->tr[tl].d1) > 0) && (state->tr[tmp_d].u0 == tu))
	state->tr[tmp_d].u0 = tl;
      if (((tmp_d = state->tr[tl].d1) > 0) && (state->tr[tmp_d].u1 == tu))
	state->tr[tmp_d].u1 = tl;

      /* Now update the query structure and obtain the sinks for the */
      /* two trapezoids */ 
      
      i1 = newnode(state);		/* Upper trapezoid sink */
      i2 = newnode(state);		/* Lower trapezoid sink */
      sk = state->tr[tu].sink;
      
      state->qs[sk].nodetype = T_Y;
      state->qs[sk].yval = s.v0;
      state->qs[sk].segnum = segnum;	/* not really reqd ... maybe later */
      state->qs[sk].left = i2;
      state->qs[sk].right = i1;

      state->qs[i1].nodetype = T_SINK;
      state->qs[i1].trnum = tu;
      state->qs[i1].parent = sk;

      state->qs[i2].nodetype = T_SINK;
      state->qs[i2].trnum = tl;
      state->qs[i2].parent = sk;

      state->tr[tu].sink = i1;
      state->tr[tl].sink = i2;
      tfirst = tl;
    }
  else				/* v0 already present */
    {       /* Get the topmost intersecting trapezoid */
      tfirst = locate_endpoint(state, &s.v0, &s.v1, s.root0);
      tritop = 1;
    }


  if ((is_swapped) ? !inserted(state, segnum, FIRSTPT) :
       !inserted(state, segnum, LASTPT))     /* insert v1 in the tree */
    {
      int tmp_d;

      tu = locate_endpoint(state, &s.v1, &s.v0, s.root1);

      tl = newtrap(state);		/* tl is the new lower trapezoid */
      state->tr[tl].state = ST_VALID;
      state->tr[tl] = state->tr[tu];
      state->tr[tu].lo.y = state->tr[tl].hi.y = s.v1.y;
      state->tr[tu].lo.x = state->tr[tl].hi.x = s.v1.x;
      state->tr[tu].d0 = tl;
      state->tr[tu].d1 = 0;
      state->tr[tl].u0 = tu;
      state->tr[tl].u1 = 0;

      if (((tmp_d = state->tr[tl].d0) > 0) && (state->tr[tmp_d].u0 == tu))
	state->tr[tmp_d].u0 = tl;
      if (((tmp_d = state->tr[tl].d0) > 0) && (state->tr[tmp_d].u1 == tu))
	state->tr[tmp_d].u1 = tl;

      if (((tmp_d = state->tr[tl].d1) > 0) && (state->tr[tmp_d].u0 == tu))
	state->tr[tmp_d].u0 = tl;
      if (((tmp_d = state->tr[tl].d1) > 0) && (state->tr[tmp_d].u1 == tu))
	state->tr[tmp_d].u1 = tl;
      
      /* Now update the query structure and obtain the sinks for the */
      /* two trapezoids */ 
      
      i1 = newnode(state);		/* Upper trapezoid sink */
      i2 = newnode(state);		/* Lower trapezoid sink */
      sk = state->tr[tu].sink;
      
      state->qs[sk].nodetype = T_Y;
      state->qs[sk].yval = s.v1;
      state->qs[sk].segnum = segnum;	/* not really reqd ... maybe later */
      state->qs[sk].left = i2;
      state->qs[sk].right = i1;

      state->qs[i1].nodetype = T_SINK;
      state->qs[i1].trnum = tu;
      state->qs[i1].parent = sk;

      state->qs[i2].nodetype = T_SINK;
      state->qs[i2].trnum = tl;
      state->qs[i2].parent = sk;

      state->tr[tu].sink = i1;
      state->tr[tl].sink = i2;
      tlast = tu;
    }
  else				/* v1 already present */
    {       /* Get the lowermost intersecting trapezoid */
      tlast = locate_endpoint(state, &s.v1, &s.v0, s.root1);
      tribot = 1;
    }
  
  /* Thread the segment into the query tree creating a new X-node */
  /* First, split all the trapezoids which are intersected by s into */
  /* two */

  t = tfirst;			/* topmost trapezoid */
  
  while ((t > 0) && 
	 _greater_than_equal_to(&state->tr[t].lo, &state->tr[tlast].lo))
				/* traverse from top to bot */
    {
      int t_sav, tn_sav;
      sk = state->tr[t].sink;
      i1 = newnode(state);		/* left trapezoid sink */
      i2 = newnode(state);		/* right trapezoid sink */
      
      state->qs[sk].nodetype = T_X;
      state->qs[sk].segnum = segnum;
      state->qs[sk].left = i1;
      state->qs[sk].right = i2;

      state->qs[i1].nodetype = T_SINK;	/* left trapezoid (use existing one) */
      state->qs[i1].trnum = t;
      state->qs[i1].parent = sk;

      state->qs[i2].nodetype = T_SINK;	/* right trapezoid (allocate new) */
      state->qs[i2].trnum = tn = newtrap(state);
      state->tr[tn].state = ST_VALID;
      state->qs[i2].parent = sk;

      if (t == tfirst)
          tfirstr = tn;
      if (_equal_to(&state->tr[t].lo, &state->tr[tlast].lo))
          tlastr = tn;

      state->tr[tn] = state->tr[t];
      state->tr[t].sink = i1;
      state->tr[tn].sink = i2;
      t_sav = t;
      tn_sav = tn;

      /* error */

      if ((state->tr[t].d0 <= 0) && (state->tr[t].d1 <= 0)) /* case cannot arise */
	{
	  fprintf(stderr, "add_segment: error\n");
	  break;
	}
      
      /* only one trapezoid below. partition t into two and make the */
      /* two resulting trapezoids t and tn as the upper neighbours of */
      /* the sole lower trapezoid */
      
      else if ((state->tr[t].d0 > 0) && (state->tr[t].d1 <= 0))
	{			/* Only one trapezoid below */
	  if ((state->tr[t].u0 > 0) && (state->tr[t].u1 > 0))
	    {			/* continuation of a chain from abv. */
	      if (state->tr[t].usave > 0) /* three upper neighbours */
		{
		  if (state->tr[t].uside == S_LEFT)
		    {
		      state->tr[tn].u0 = state->tr[t].u1;
		      state->tr[t].u1 = -1;
		      state->tr[tn].u1 = state->tr[t].usave;
		      
		      state->tr[state->tr[t].u0].d0 = t;
		      state->tr[state->tr[tn].u0].d0 = tn;
		      state->tr[state->tr[tn].u1].d0 = tn;
		    }
		  else		/* intersects in the right */
		    {
		      state->tr[tn].u1 = -1;
		      state->tr[tn].u0 = state->tr[t].u1;
		      state->tr[t].u1 = state->tr[t].u0;
		      state->tr[t].u0 = state->tr[t].usave;

		      state->tr[state->tr[t].u0].d0 = t;
		      state->tr[state->tr[t].u1].d0 = t;
		      state->tr[state->tr[tn].u0].d0 = tn;
		    }
		  
		  state->tr[t].usave = state->tr[tn].usave = 0;
		}
	      else		/* No usave.... simple case */
		{
		  state->tr[tn].u0 = state->tr[t].u1;
		  state->tr[t].u1 = state->tr[tn].u1 = -1;
		  state->tr[state->tr[tn].u0].d0 = tn;
		}
	    }
	  else 
	    {			/* fresh seg. or upward cusp */
	      int tmp_u = state->tr[t].u0;
	      int td0, td1;
	      if (((td0 = state->tr[tmp_u].d0) > 0) &&
		  ((td1 = state->tr[tmp_u].d1) > 0))
		{		/* upward cusp */
		  if ((state->tr[td0].rseg > 0) &&
		      !is_left_of(state, state->tr[td0].rseg, &s.v1))
		    {
		      state->tr[t].u0 = state->tr[t].u1 = state->tr[tn].u1 = -1;
		      state->tr[state->tr[tn].u0].d1 = tn;
		    }
		  else		/* cusp going leftwards */
		    { 
		      state->tr[tn].u0 = state->tr[tn].u1 = state->tr[t].u1 = -1;
		      state->tr[state->tr[t].u0].d0 = t;
		    }
		}
	      else		/* fresh segment */
		{
		  state->tr[state->tr[t].u0].d0 = t;
		  state->tr[state->tr[t].u0].d1 = tn;
		}	      
	    }
	  
	  if (FP_EQUAL(state->tr[t].lo.y, state->tr[tlast].lo.y) &&
	      FP_EQUAL(state->tr[t].lo.x, state->tr[tlast].lo.x) && tribot)
	    {		/* bottom forms a triangle */

	      if (is_swapped)	
		tmptriseg = state->seg[segnum].prev;
	      else
		tmptriseg = state->seg[segnum].next;
	      
	      if ((tmptriseg > 0) && is_left_of(state, tmptriseg, &s.v0))
		{
				/* L-R downward cusp */
		  state->tr[state->tr[t].d0].u0 = t;
		  state->tr[tn].d0 = state->tr[tn].d1 = -1;
		}
	      else
		{
				/* R-L downward cusp */
		  state->tr[state->tr[tn].d0].u1 = tn;
		  state->tr[t].d0 = state->tr[t].d1 = -1;
		}
	    }
	  else
	    {
	      if ((state->tr[state->tr[t].d0].u0 > 0) && (state->tr[state->tr[t].d0].u1 > 0))
		{
		  if (state->tr[state->tr[t].d0].u0 == t) /* passes thru LHS */
		    {
		      state->tr[state->tr[t].d0].usave = state->tr[state->tr[t].d0].u1;
		      state->tr[state->tr[t].d0].uside = S_LEFT;
		    }
		  else
		    {
		      state->tr[state->tr[t].d0].usave = state->tr[state->tr[t].d0].u0;
		      state->tr[state->tr[t].d0].uside = S_RIGHT;
		    }		    
		}
	      state->tr[state->tr[t].d0].u0 = t;
	      state->tr[state->tr[t].d0].u1 = tn;
	    }
	  
	  t = state->tr[t].d0;
	}


      else if ((state->tr[t].d0 <= 0) && (state->tr[t].d1 > 0))
	{			/* Only one trapezoid below */
	  if ((state->tr[t].u0 > 0) && (state->tr[t].u1 > 0))
	    {			/* continuation of a chain from abv. */
	      if (state->tr[t].usave > 0) /* three upper neighbours */
		{
		  if (state->tr[t].uside == S_LEFT)
		    {
		      state->tr[tn].u0 = state->tr[t].u1;
		      state->tr[t].u1 = -1;
		      state->tr[tn].u1 = state->tr[t].usave;
		      
		      state->tr[state->tr[t].u0].d0 = t;
		      state->tr[state->tr[tn].u0].d0 = tn;
		      state->tr[state->tr[tn].u1].d0 = tn;
		    }
		  else		/* intersects in the right */
		    {
		      state->tr[tn].u1 = -1;
		      state->tr[tn].u0 = state->tr[t].u1;
		      state->tr[t].u1 = state->tr[t].u0;
		      state->tr[t].u0 = state->tr[t].usave;

		      state->tr[state->tr[t].u0].d0 = t;
		      state->tr[state->tr[t].u1].d0 = t;
		      state->tr[state->tr[tn].u0].d0 = tn;
		    }
		  
		  state->tr[t].usave = state->tr[tn].usave = 0;
		}
	      else		/* No usave.... simple case */
		{
		  state->tr[tn].u0 = state->tr[t].u1;
		  state->tr[t].u1 = state->tr[tn].u1 = -1;
		  state->tr[state->tr[tn].u0].d0 = tn;
		}
	    }
	  else 
	    {			/* fresh seg. or upward cusp */
	      int tmp_u = state->tr[t].u0;
	      int td0, td1;
	      if (((td0 = state->tr[tmp_u].d0) > 0) &&
		  ((td1 = state->tr[tmp_u].d1) > 0))
		{		/* upward cusp */
		  if ((state->tr[td0].rseg > 0) &&
		      !is_left_of(state, state->tr[td0].rseg, &s.v1))
		    {
		      state->tr[t].u0 = state->tr[t].u1 = state->tr[tn].u1 = -1;
		      state->tr[state->tr[tn].u0].d1 = tn;
		    }
		  else 
		    {
		      state->tr[tn].u0 = state->tr[tn].u1 = state->tr[t].u1 = -1;
		      state->tr[state->tr[t].u0].d0 = t;
		    }
		}
	      else		/* fresh segment */
		{
		  state->tr[state->tr[t].u0].d0 = t;
		  state->tr[state->tr[t].u0].d1 = tn;
		}
	    }
	  
	  if (FP_EQUAL(state->tr[t].lo.y, state->tr[tlast].lo.y) &&
	      FP_EQUAL(state->tr[t].lo.x, state->tr[tlast].lo.x) && tribot)
	    {		/* bottom forms a triangle */
	      int tmpseg;

	      if (is_swapped)	
		tmptriseg = state->seg[segnum].prev;
	      else
		tmptriseg = state->seg[segnum].next;

	      if ((tmpseg > 0) && is_left_of(state, tmpseg, &s.v0))
		{
		  /* L-R downward cusp */
		  state->tr[state->tr[t].d1].u0 = t;
		  state->tr[tn].d0 = state->tr[tn].d1 = -1;
		}
	      else
		{
		  /* R-L downward cusp */
		  state->tr[state->tr[tn].d1].u1 = tn;
		  state->tr[t].d0 = state->tr[t].d1 = -1;
		}
	    }		
	  else
	    {
	      if ((state->tr[state->tr[t].d1].u0 > 0) && (state->tr[state->tr[t].d1].u1 > 0))
		{
		  if (state->tr[state->tr[t].d1].u0 == t) /* passes thru LHS */
		    {
		      state->tr[state->tr[t].d1].usave = state->tr[state->tr[t].d1].u1;
		      state->tr[state->tr[t].d1].uside = S_LEFT;
		    }
		  else
		    {
		      state->tr[state->tr[t].d1].usave = state->tr[state->tr[t].d1].u0;
		      state->tr[state->tr[t].d1].uside = S_RIGHT;
		    }		    
		}
	      state->tr[state->tr[t].d1].u0 = t;
	      state->tr[state->tr[t].d1].u1 = tn;
	    }
	  
	  t = state->tr[t].d1;
	}

      /* two trapezoids below. Find out which one is intersected by */
      /* this segment and proceed down that one */
      
      else
	{
	  int tmpseg = state->tr[state->tr[t].d0].rseg;
	  double y0, yt;
	  point_t tmppt;
	  int tnext, i_d0, i_d1;

	  i_d0 = i_d1 = FALSE;
	  if (FP_EQUAL(state->tr[t].lo.y, s.v0.y))
	    {
	      if (state->tr[t].lo.x > s.v0.x)
		i_d0 = TRUE;
	      else
		i_d1 = TRUE;
	    }
	  else
	    {
	      tmppt.y = y0 = state->tr[t].lo.y;
	      yt = (y0 - s.v0.y)/(s.v1.y - s.v0.y);
	      tmppt.x = s.v0.x + yt * (s.v1.x - s.v0.x);
	      
	      if (_less_than(&tmppt, &state->tr[t].lo))
		i_d0 = TRUE;
	      else
		i_d1 = TRUE;
	    }
	  
	  /* check continuity from the top so that the lower-neighbour */
	  /* values are properly filled for the upper trapezoid */

	  if ((state->tr[t].u0 > 0) && (state->tr[t].u1 > 0))
	    {			/* continuation of a chain from abv. */
	      if (state->tr[t].usave > 0) /* three upper neighbours */
		{
		  if (state->tr[t].uside == S_LEFT)
		    {
		      state->tr[tn].u0 = state->tr[t].u1;
		      state->tr[t].u1 = -1;
		      state->tr[tn].u1 = state->tr[t].usave;
		      
		      state->tr[state->tr[t].u0].d0 = t;
		      state->tr[state->tr[tn].u0].d0 = tn;
		      state->tr[state->tr[tn].u1].d0 = tn;
		    }
		  else		/* intersects in the right */
		    {
		      state->tr[tn].u1 = -1;
		      state->tr[tn].u0 = state->tr[t].u1;
		      state->tr[t].u1 = state->tr[t].u0;
		      state->tr[t].u0 = state->tr[t].usave;

		      state->tr[state->tr[t].u0].d0 = t;
		      state->tr[state->tr[t].u1].d0 = t;
		      state->tr[state->tr[tn].u0].d0 = tn;
		    }
		  
		  state->tr[t].usave = state->tr[tn].usave = 0;
		}
	      else		/* No usave.... simple case */
		{
		  state->tr[tn].u0 = state->tr[t].u1;
		  state->tr[tn].u1 = -1;
		  state->tr[t].u1 = -1;
		  state->tr[state->tr[tn].u0].d0 = tn;
		}
	    }
	  else 
	    {			/* fresh seg. or upward cusp */
	      int tmp_u = state->tr[t].u0;
	      int td0, td1;
	      if (((td0 = state->tr[tmp_u].d0) > 0) &&
		  ((td1 = state->tr[tmp_u].d1) > 0))
		{		/* upward cusp */
		  if ((state->tr[td0].rseg > 0) &&
		      !is_left_of(state, state->tr[td0].rseg, &s.v1))
		    {
		      state->tr[t].u0 = state->tr[t].u1 = state->tr[tn].u1 = -1;
		      state->tr[state->tr[tn].u0].d1 = tn;
		    }
		  else 
		    {
		      state->tr[tn].u0 = state->tr[tn].u1 = state->tr[t].u1 = -1;
		      state->tr[state->tr[t].u0].d0 = t;
		    }
		}
	      else		/* fresh segment */
		{
		  state->tr[state->tr[t].u0].d0 = t;
		  state->tr[state->tr[t].u0].d1 = tn;
		}
	    }
	  
	  if (FP_EQUAL(state->tr[t].lo.y, state->tr[tlast].lo.y) &&
	      FP_EQUAL(state->tr[t].lo.x, state->tr[tlast].lo.x) && tribot)
	    {
	      /* this case arises only at the lowest trapezoid.. i.e.
		 tlast, if the lower endpoint of the segment is
		 already inserted in the structure */
	      
	      state->tr[state->tr[t].d0].u0 = t;
	      state->tr[state->tr[t].d0].u1 = -1;
	      state->tr[state->tr[t].d1].u0 = tn;
	      state->tr[state->tr[t].d1].u1 = -1;

	      state->tr[tn].d0 = state->tr[t].d1;
	      state->tr[t].d1 = state->tr[tn].d1 = -1;
	      
	      tnext = state->tr[t].d1;
	    }
	  else if (i_d0)
				/* intersecting d0 */
	    {
	      state->tr[state->tr[t].d0].u0 = t;
	      state->tr[state->tr[t].d0].u1 = tn;
	      state->tr[state->tr[t].d1].u0 = tn;
	      state->tr[state->tr[t].d1].u1 = -1;
	      
	      /* new code to determine the bottom neighbours of the */
	      /* newly partitioned trapezoid */
	      
	      state->tr[t].d1 = -1;

	      tnext = state->tr[t].d0;
	    }
	  else			/* intersecting d1 */
	    {
	      state->tr[state->tr[t].d0].u0 = t;
	      state->tr[state->tr[t].d0].u1 = -1;
	      state->tr[state->tr[t].d1].u0 = t;
	      state->tr[state->tr[t].d1].u1 = tn;

	      /* new code to determine the bottom neighbours of the */
	      /* newly partitioned trapezoid */
	      
	      state->tr[tn].d0 = state->tr[t].d1;
	      state->tr[tn].d1 = -1;
	      
	      tnext = state->tr[t].d1;
	    }	    
	  
	  t = tnext;
	}
      
      state->tr[t_sav].rseg = state->tr[tn_sav].lseg  = segnum;
    } /* end-while */
  
  /* Now combine those trapezoids which share common segments. We can */
  /* use the pointers to the parent to connect these together. This */
  /* works only because all these new trapezoids have been formed */
  /* due to splitting by the segment, and hence have only one parent */

  tfirstl = tfirst; 
  tlastl = tlast;
  merge_trapezoids(state, segnum, tfirstl, tlastl, S_LEFT);
  merge_trapezoids(state, segnum, tfirstr, tlastr, S_RIGHT);

  state->seg[segnum].is_inserted = TRUE;
  return 0;
}


/* Update the roots stored for each of the endpoints of the segment.
 * This is done to speed up the location-query for the endpoint when
 * the segment is inserted into the trapezoidation subsequently
 */
static int find_new_roots(SeidelTriangulator* state, int segnum)
{
  segment_t *s = &state->seg[segnum];
  
  if (s->is_inserted)
    return 0;

  s->root0 = locate_endpoint(state, &s->v0, &s->v1, s->root0);
  s->root0 = state->tr[s->root0].sink;

  s->root1 = locate_endpoint(state, &s->v1, &s->v0, s->root1);
  s->root1 = state->tr[s->root1].sink;
  return 0;
}


/* Main routine to perform trapezoidation */
int construct_trapezoids(SeidelTriangulator* state, int nseg)
{
  register int i;
  int root, h;
  
  /* Add the first segment and get the query structure and trapezoid */
  /* list initialised */

  root = init_query_structure(state, choose_segment(state));

  for (i = 1; i <= nseg; i++)
    state->seg[i].root0 = state->seg[i].root1 = root;
  
  for (h = 1; h <= math_logstar_n(nseg); h++)
    {
      for (i = math_N(nseg, h -1) + 1; i <= math_N(nseg, h); i++)
	add_segment(state, choose_segment(state));
      
      /* Find a new root for each of the segment endpoints */
      for (i = 1; i <= nseg; i++)
	find_new_roots(state, i);
    }
  
  for (i = math_N(nseg, math_logstar_n(nseg)) + 1; i <= nseg; i++)
    add_segment(state, choose_segment(state));

  return 0;
}


