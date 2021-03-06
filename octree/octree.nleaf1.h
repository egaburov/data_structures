#ifndef __OCTREE_H__
#define __OCTREE_H__

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <stack>
#include <list>
#include "vector3.h"
#include "boundary.h"

typedef float real;
typedef vector3<real> vec3;
typedef Boundary<real> boundary;

template<class T>
static inline T __min(const T &a, const T &b) {return a < b ? a : b;}
template<class T>
static inline T __max(const T &a, const T &b) {return a > b ? a : b;}


#define SQR(x) ((x)*(x))


#ifdef __mySSE__
typedef int    v4si  __attribute__((vector_size(16)));
typedef float  v4sf  __attribute__((vector_size(16)));
#endif

struct Particle
{
  typedef std::vector<Particle> Vector;
  vec3 pos;
  int  id;

  Particle(const vec3 &_pos, const int _id) :
    pos(_pos), id(_id) {}
};



struct Octree
{
  enum {NLEAF = 16};
  enum {EMPTY = -1};
  enum {BODYX = -2};

  struct Body
  {
    typedef std::vector<Body> Vector;
    private:
#ifdef __mySSE__
    v4sf packed_pos;
#else
    vec3 _pos;
    int  _idx;
#endif

    public:
#ifdef __mySSE__
    Body(const vec3 &pos, const int idx) 
    {
      packed_pos = (v4sf){pos.x, pos.y, pos.z, (float)idx};
    }
    Body(const Particle &p, const int idx) 
    {
      packed_pos = (v4sf){p.pos.x, p.pos.y, p.pos.z, (float)idx};
    }
    int idx() const {return (int)__builtin_ia32_vec_ext_v4sf(packed_pos, 3);}
    v4sf pos() const {return packed_pos;}
#else
    Body(const vec3 &__pos, const int __idx) : _pos(__pos), _idx(__idx) {}
    Body(const Particle &p, const int __idx) : _pos(p.pos), _idx(__idx) {}
    int idx() const {return _idx;}
    const vec3& pos() const {return _pos;}
#endif

  };

  typedef std::pair<int, int> Int2;

  vec3 root_centre;
  real root_size;
  int depth;
  int ncell;
  int nnode;
  std::vector<int> node_list;
  std::vector<boundary> innerBnd;

  Octree(const vec3 &_centre, const real _size, const int n_nodes) :
    root_centre(_centre), root_size(_size), depth(0), ncell(0), nnode(n_nodes)
  {
    node_list.resize(n_nodes<<3);
    innerBnd.resize(n_nodes<<3);
    for (std::vector<int>::iterator it = node_list.begin(); it != node_list.end(); it++)
      *it = -1;
  }

  void clear()
  {
    depth = ncell = 0;
    for (std::vector<int>::iterator it = node_list.begin(); it != node_list.end(); it++)
      *it = -1;
  }

  static inline int reverse_int(const int a) {return BODYX-a;}

#ifdef __mySSE__
  static inline int Octant(const v4sf lhs, const v4sf rhs) 
  {
    int mask = __builtin_ia32_movmskps(
        __builtin_ia32_cmpgeps(rhs, lhs));
    return 7 & mask;
  }
#else
  static inline int Octant(const vec3 &lhs, const vec3 &rhs) 
  {
    return
      (((lhs.x <= rhs.x) ? 1 : 0) +  
       ((lhs.y <= rhs.y) ? 2 : 0) + 
       ((lhs.z <= rhs.z) ? 4 : 0));
  }
#endif

#ifdef __mySSE__
  static inline v4sf compute_centre(const v4sf centre, const real dummy, const int oct)
  {
    static const v4sf off[8] = {
      {-0.25f, -0.25f, -0.25f, -0.5f},
      {+0.25f, -0.25f, -0.25f, -0.5f},
      {-0.25f, +0.25f, -0.25f, -0.5f},
      {+0.25f, +0.25f, -0.25f, -0.5f},
      {-0.25f, -0.25f, +0.25f, -0.5f},
      {+0.25f, -0.25f, +0.25f, -0.5f},
      {-0.25f, +0.25f, +0.25f, -0.5f},
      {+0.25f, +0.25f, +0.25f, -0.5f},
    };
    const v4sf len = __builtin_ia32_shufps(centre, centre, 0xff);
    return centre + len * off[oct];
  }
#else
  static inline vec3 compute_centre(const vec3 &c, real s, const int oct)
  {
    assert(oct >= 0);
    assert(oct  < 8);
    s *= (real)0.5;
    return vec3(
        c.x + s * ((oct&1) ? (real)1.0 : (real)-1.0),
        c.y + s * ((oct&2) ? (real)1.0 : (real)-1.0),
        c.z + s * ((oct&4) ? (real)1.0 : (real)-1.0)
        );
  }
#endif


  /********/


  void push(const Body &body, const int idx, const Body::Vector &bodies)
  {
    int child_idx = 0;   /* child idx inside a node */
    int child     = 0;   /* child */
    int locked    = 0;   /* cell that needs to be updated */
    int depth     = 0;   /* depth */ 
    const int _n_nodes = node_list.size();

#ifdef __mySSE__
    v4sf centre = {root_centre.x, root_centre.y, root_centre.z, root_size};
#else
    vec3 centre = root_centre;
#endif
    real hsize  = root_size*(real)0.5; /* not being used in SSE version */

    /* walk the tree to find the first leaf or empty cell */
    while (child > EMPTY)  /* if non-negative, means it is a tree-node */
    {
      const int node = child;
      depth++;

      child_idx  = Octant(centre, body.pos());
      centre     = compute_centre(centre, hsize, child_idx);
      hsize     *= (real)0.5;

      locked = node + child_idx;
      assert(locked < _n_nodes);
      child  = node_list[locked];
    }

    /* locked on the cell that needs to be updated */

    if (child != EMPTY)
      while((child = node_list[locked]) != EMPTY)
      {
        assert(child < EMPTY);
        depth++;
        ncell++;

        const int cfirst = ncell<<3;
        assert(cfirst+7 < _n_nodes);
        node_list[locked] = cfirst;

        const int body_id = reverse_int(child);
        assert(body_id >= 0);
        child_idx = Octant(centre, bodies[body_id].pos());
        node_list[cfirst + child_idx] = child;

        child_idx = Octant(centre, body.pos());
        centre    = compute_centre(centre, hsize, child_idx);
        hsize    *= (real)0.5;
        locked    = cfirst + child_idx;
      }
    assert(node_list[locked] == EMPTY);
    node_list[locked] = reverse_int(idx);

    this->depth = __max(this->depth, depth);
  }
 
  /**************/

  template<const bool ROOT>  /* must be ROOT = true on the root node (first call) */
    boundary inner_boundary(const Body::Vector &bodies, const int node = 0)
    {
      boundary bnd;
      if (ROOT)
      {
        for (int k = 0; k < 8; k++)
        {
          innerBnd[k] = inner_boundary<false>(bodies, k);
          bnd.merge(innerBnd[k]);
        }
        return bnd;
      }
      else
      {
        assert(node < (int)node_list.size());
        const int cell = node_list[node];
        if (cell == EMPTY) return boundary();

        innerBnd[node] = boundary();

        if (cell > EMPTY)
        {
          for (int k = 0; k < 8; k++)
            if (node_list[cell+k] != EMPTY) 
              innerBnd[node].merge(inner_boundary<false>(bodies, cell+k));
        }
        else
        {
          const vec3 jpos = bodies[reverse_int(cell)].pos();
          innerBnd[node] = boundary(jpos);
        }
        return innerBnd[node];
      }
    }

  /**************/

  template<const bool ROOT>  /* must be ROOT = true on the root node (first call) */
    int sanity_check(const Body::Vector &bodies, const int node = 0, int nb = 0) const
    {
      if (ROOT)
      {
        for (int k = 0; k < 8; k++)
          nb = sanity_check<false>(bodies, k, nb);
      }
      else
      {
        assert(node < (int)node_list.size());
        const int cell = node_list[node];
        if (cell == EMPTY) return nb;

        if (cell > EMPTY)
        {
          for (int k = 0; k < 8; k++)
            if (node_list[cell+k] != EMPTY) 
              nb = sanity_check<false>(bodies, cell+k, nb);
        }
        else
        {
          const vec3 jpos = bodies[reverse_int(cell)].pos();
          assert(overlapped(innerBnd[node], jpos));
          nb++;
        }
      }
      return nb;
    }

  /**************/

  template<const bool ROOT>  /* must be ROOT = true on the root node (first call) */
    void tree_dump(std::vector<int> &list, const int node = 0) const
    {
      if (ROOT)
      {
        for (int k = 0; k < 8; k++)
          tree_dump<false>(list, k);
      }
      else
      {
        assert(node < (int)node_list.size());
        const int cell = node_list[node];
        if (cell == EMPTY) return;
        if (cell >  EMPTY)
        {
          for (int k = 0; k < 8; k++)
            tree_dump<false>(list, cell+k);
        }
        else
          list.push_back(reverse_int(cell));
      }
    }

  /**************/

  template<const bool ROOT>  /* must be ROOT = true on the root node (first call) */
    int range_search(
        const vec3 &pos, const real h, const Body::Vector &bodies,
        const int node = 0, const boundary &ibnd = boundary(), int nb = 0) const
    {
      if (ROOT)
      {
        for (int k = 0; k < 8; k++)
          if (node_list[k] != EMPTY)
            nb = range_search<false>(pos, h, bodies, k, boundary(pos, h), nb);
      }
      else
      {
        const int cell = node_list[node];
        if (not_overlapped(ibnd, innerBnd[node]))
          return nb;
        else if (cell > EMPTY)
        {
          for (int k = 0; k < 8; k++)
            if (node_list[cell+k] != EMPTY)
              nb = range_search<false>(pos, h, bodies, cell+k, ibnd, nb);
        }
        else
        {
          const vec3 jpos = bodies[reverse_int(cell)].pos();
          if ((pos - jpos).norm2() < h*h)
            nb++;
        }
      }
      return nb;
    }
  
  /**************/

};


#endif /* __OCTREE_H__ */
