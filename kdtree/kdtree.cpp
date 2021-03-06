#include "kdtree.h"
#include <cstdio>
#include <cstdlib>
#include "plummer.h"
#include "mytimer.h"

int main(int argc, char * argv[])
{
  int n_bodies = 10240;
  if (argc > 1) n_bodies = atoi(argv[1]);
  assert(n_bodies > 0);
  fprintf(stderr, "n_bodies= %d\n", n_bodies);

  const double t00 = get_wtime();
  Particle::Vector ptcl;
  ptcl.reserve(n_bodies);
#if 1
#define PLUMMER
  const Plummer data(n_bodies);
  for (int i = 0; i < n_bodies; i++)
  {
    ptcl.push_back(Particle(data.pos[i], data.mass[i]));
  }
#elif 1  /* reads IC */
  int dummy;
  std::cin >> dummy >> n_bodies;
  fprintf(stderr, " -- input file: nbodies= %d\n", n_bodies);
  for (int i = 0; i < n_bodies; i++)
  {
    int idum;
    real fdum;
    Particle p;
    std::cin >> p.pos.x >> p.pos.y >> p.pos.z >> fdum >> idum;
    ptcl.push_back(p);
  }
#elif 0
  {
    for (int i = 0; i < n_bodies; i++)
    {
      ptcl.push_back(Particle(
            vec3(drand48(), drand48(), drand48()),
            1.0/n_bodies));
    }
  }
#endif


  const double t10 = get_wtime();
  fprintf(stderr, " -- Buidling kdTree -- \n");
  kdTree tree(ptcl);
  fprintf(stderr, "    Depth= %d\n", tree.getDepth());
  const double t20 = get_wtime();

  fprintf(stderr, " -- Searching nearest ngb -- \n");
  {
    real s = 0.0;
    real s2 = 0.0;
#pragma omp parallel for reduction(+:s,s2)
    for (int i = 0; i < n_bodies; i++)
    {
      const int  j = tree.find_nnb(ptcl[i].pos);
      const real r = (ptcl[i].pos - ptcl[j].pos).abs();
#if 0 /* correctness check */
      real s2min = HUGE;
      int  jmin  = -1;
      for (int jx = 0; jx < n_bodies; jx++)
      {
        const real r2 = (ptcl[i].pos - ptcl[jx].pos).norm2();
        if (r2 < s2min && r2 != 0.0f)
        {
          s2min = r2;
          jmin = jx;
        } 
      }
      assert(jmin == j);
#endif
      s  += r;
      s2 += r*r;
    }
    s  *= 1.0/n_bodies;
    s2 *= 1.0/n_bodies;
    const real ds = std::sqrt(s2 - s*s);
    fprintf(stderr, "<r> = %g  sigma= %g \n", s, ds);
  }
  const double t30 = get_wtime();
  
  fprintf(stderr, " Tree dump \n");

  std::vector<int> sortedBodies;
  sortedBodies.reserve(n_bodies);
  tree.dump(sortedBodies);
  assert((int)sortedBodies.size() == n_bodies);
  
  const double t40 = get_wtime();
  
  fprintf(stderr, " Shuffle \n");

  Particle::Vector sortedPtcl(n_bodies);
  for (int i = 0; i < n_bodies; i++)
    sortedPtcl[i] = ptcl[sortedBodies[i]];

  const double t50 = get_wtime();

  fprintf(stderr, " Building sortedTree \n");
  kdTree sortedTree(sortedPtcl);
  fprintf(stderr, "    Depth= %d\n", tree.getDepth());
  
  const double t60 = get_wtime();
  
  fprintf(stderr, " -- Searching nearest ngb in sortedTree -- \n");
  {
    real s = 0.0;
    real s2 = 0.0;
#pragma omp parallel for reduction(+:s,s2)
    for (int i = 0; i < n_bodies; i++)
    {
      vec3 ipos = sortedPtcl[i].pos;
      const int  j = sortedTree.find_nnb(ipos);
      const real r = (ipos - sortedPtcl[j].pos).abs();
#if 0 /* correctness check */
      real s2min = HUGE;
      int  jmin  = -1;
      for (int jx = 0; jx < n_bodies; jx++)
      {
        const real r2 = (ipos - sortedPtcl[jx].pos).norm2();
        if (r2 < s2min && r2 != 0.0f)
        {
          s2min = r2;
          jmin = jx;
        } 
      }
      assert(jmin == j);
#endif
      s  += r;
      s2 += r*r;
    }
    s  *= 1.0/n_bodies;
    s2 *= 1.0/n_bodies;
    const real ds = std::sqrt(s2 - s*s);
    fprintf(stderr, "<r> = %g  sigma= %g \n", s, ds);
  }
  
  const double t70 = get_wtime();
  
  fprintf(stderr, " -- Searching nearest ngb w/ Leaf in sortedTree -- \n");
  {
#if 1
    const kdTree &t = sortedTree;
#else
    const kdTree &t = tree;
#endif
    const int nLeaf = t.nLeaf();
    real s = 0.0;
    real s2 = 0.0;
    for (int ileaf = 0; ileaf < nLeaf; ileaf++)
    {
      const kdTree::Leaf &leaf = t.getLeaf(ileaf);
#pragma omp parallel for
      for (int i = 0; i < leaf.size(); i++)
      {
        const vec3& ipos = leaf[i].pos();
        const int  j = t.find_nnb(ipos);
        const real r = (ipos - sortedPtcl[j].pos).abs();
#if 0 /* correctness check */
        real s2min = HUGE;
        int  jmin  = -1;
        for (int jx = 0; jx < n_bodies; jx++)
        {
          const real r2 = (ipos - sortedPtcl[jx].pos).norm2();
          if (r2 < s2min && r2 != 0.0f)
          {
            s2min = r2;
            jmin = jx;
          } 
        }
        assert(jmin == j);
#endif
        s  += r;
        s2 += r*r;
      }
    }
    s  *= 1.0/n_bodies;
    s2 *= 1.0/n_bodies;
    const real ds = std::sqrt(s2 - s*s);
    fprintf(stderr, "<r> = %g  sigma= %g \n", s, ds);
  }

  const double t80 = get_wtime();



  fprintf(stderr, " Timing info: \n");
  fprintf(stderr, " -------------\n");
  fprintf(stderr, "   Plummer:  %g sec \n", t10 -t00);
  fprintf(stderr, "   kdTree:   %g sec \n", t20 -t10);
  fprintf(stderr, "   nnb   :   %g sec \n", t30 -t20);
  fprintf(stderr, "   Dump  :   %g sec \n", t40 -t30);
  fprintf(stderr, "   Move  :   %g sec \n", t50 -t40);
  fprintf(stderr, "   sTree :   %g sec \n", t60 -t50);
  fprintf(stderr, "   sNgbe :   %g sec \n", t70 -t60);
  fprintf(stderr, "   sLeaf :   %g sec \n", t80 -t70);



  return 0;
}
