#include "mytimer.h"
double dt_00, dt_10, dt_20, dt_30, dt_40, dt_44, dt_50, dt_60;
double dtA;
#include "vorocell.h"
#include <iostream>

typedef Voronoi::real real;
typedef Voronoi::vec3 vec3;

struct cmp_dist
{
  bool operator()(const std::pair<float, int> &lhs, const std::pair<float, int> &rhs) const
  {
    return lhs.first < rhs.first;
  }
};

int main(int argc, char * argv[])
{
  dt_00=dt_10=dt_20=dt_30=dt_40=dt_44=dt_50=dt_60=0.0;
  dtA = 0.0;
  double eps;
  int ns, nrg;
  double lx, ly, lz;
  int np;

  int idum;
  double fdum;
  std::string sdum;

  std::cin >> eps >> sdum;
  std::cin >> ns >> nrg >> sdum >> sdum;
  std::cin >> fdum >> sdum;
  std::cin >> idum >> sdum >> idum;
  std::cin >> idum >> sdum;
  std::cin >> idum >> sdum;
  std::cin >> idum >> sdum;
  std::cin >> idum >> sdum;
  std::cin >> idum >> np >> sdum >> sdum;
  std::cin >> lx >> ly >> lz >> sdum >> sdum >> sdum;

  fprintf(stderr, " eps= %g \n", eps);
  fprintf(stderr, " ns= %d  nrg= %d\n", ns, nrg);
  fprintf(stderr, " np= %d\n", np);
  fprintf(stderr, " l= %g %g %g \n", lx, ly, lz);

  Voronoi::Site::Vector sites(np);
  vec3 min(+HUGE), max(-HUGE);
  for (int i = 0; i < np; i++)
  {
    std::cin >> idum >> 
      sites[i].pos.x >> 
      sites[i].pos.y >> 
      sites[i].pos.z;
    sites[i].idx = i;
    min = mineach(min, sites[i].pos);
    max = maxeach(max, sites[i].pos);
  }
  for (int i = 0; i < np; i++)
    sites[i].pos -= min;
  min -= min;
  max -= min;
  fprintf(stderr, " min= %g %g %g \n", min.x, min.y, min.z);
  fprintf(stderr, " max= %g %g %g \n", max.x, max.y, max.z);
  for (int i = 0; i < np; i++)
  {
    assert(sites[i].pos.x < lx);
    assert(sites[i].pos.y < ly);
    assert(sites[i].pos.z < lz);
  }

  Voronoi::Site::Vector sitesP;
  sitesP.reserve(4*np);
#if 1 /* periodic */
  const real  f = 0.5;
  const real dx = f * lx;
  const real dy = f * ly;
  const real dz = f * lz;
  for (int i = 0; i < np; i++)
  {
    Voronoi::Site &s = sites[i];
    sitesP.push_back(s);

    if      (s.pos.x      < dx) sitesP.push_back(Voronoi::Site(vec3(s.pos.x+lx, s.pos.y, s.pos.z), -1-s.idx));
    else if (lx - s.pos.x < dx) sitesP.push_back(Voronoi::Site(vec3(s.pos.x-lx, s.pos.y, s.pos.z), -1-s.idx));

    if      (s.pos.y      < dy) sitesP.push_back(Voronoi::Site(vec3(s.pos.x, s.pos.y+ly, s.pos.z), -1-s.idx));
    else if (ly - s.pos.y < dy) sitesP.push_back(Voronoi::Site(vec3(s.pos.x, s.pos.y-ly, s.pos.z), -1-s.idx));

    if      (s.pos.z      < dz) sitesP.push_back(Voronoi::Site(vec3(s.pos.x, s.pos.y, s.pos.z+lz), -1-s.idx));
    else if (lz - s.pos.z < dz) sitesP.push_back(Voronoi::Site(vec3(s.pos.x, s.pos.y, s.pos.z-lz), -1-s.idx));
  }
#else /* reflecting */
#endif
  fprintf(stderr, " np= %d  Pnp= %d\n", (int)sites.size(), (int)sitesP.size());
  assert(!sitesP.empty());

  double dt_search = 0.0;
  double dt_voro   = 0.0;
  double nface     = 0.0;
  const double tbeg = get_wtime();

  for (int cnt = 0; cnt < 10; cnt++)
  {

#pragma omp parallel reduction(+:dt_search, dt_voro, nface)
    {
      Voronoi::Cell<128> cell;
      std::vector< std::pair<float, int> > dist(sitesP.size());

      Voronoi::Site::Vector list;
      list.reserve(ns);

#pragma omp for
      for (int i = 0; i < np; i++)
      {
        const Voronoi::Site &s = sites[i];

        double t0 = get_wtime();
        for (int j = 0; j < (const int)sitesP.size(); j++)
          dist[j] = std::make_pair((sitesP[j].pos - s.pos).norm2(), j);
        std::nth_element(dist.begin(), dist.begin() + ns, dist.end(), cmp_dist());
        list.clear();
        for (int j = 0; j < ns+1; j++)
          if (dist[j].first > 0.0f)
            list.push_back(Voronoi::Site(sitesP[dist[j].second].pos - s.pos, sitesP[dist[j].second].idx));
        assert((int)list.size() == ns);
        double t1 = get_wtime();
        dt_search += t1 - t0;

        t0 = t1;
        cell.build(list);
        nface += cell.nb();
        t1 = get_wtime();
        dt_voro += t1 - t0;

      }
    }
  }
  const double tend = get_wtime();

  fprintf(stderr , " dt_search=  %g sec \n", dt_search);
  fprintf(stderr , " dt_voro  =  %g sec \n", dt_voro);
  fprintf(stderr,  "   dt_00=  %g \n" ,dt_00);
  fprintf(stderr,  "   dt_10=  %g \n" ,dt_10);
  fprintf(stderr,  "   dt_20=  %g \n" ,dt_20);
  fprintf(stderr,  "   dt_30=  %g \n" ,dt_30);
  fprintf(stderr,  "   dt_40=  %g \n" ,dt_40);
  fprintf(stderr,  "   dt_44=  %g \n" ,dt_44);
  fprintf(stderr,  "   dt_50=  %g \n" ,dt_50);
  fprintf(stderr,  "   dt_60=  %g \n" ,dt_60);
  fprintf(stderr,  "   dtA=  %g \n" ,dtA);
  //  fprintf(stderr,  "    sum= %g\n", dt_00+dt_10+dt_20+dt_30+dt_40+dt_50+dt_60);
  fprintf(stderr , " dt_total =  %g sec [ sum= %g ]\n", tend - tbeg,
      dt_search + dt_voro);
  fprintf(stderr, "   nface= %g \n", nface/np);



  return 0;
};