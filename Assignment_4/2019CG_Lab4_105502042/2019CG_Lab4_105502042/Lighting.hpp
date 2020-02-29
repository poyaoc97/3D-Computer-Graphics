#pragma once
#include "MatrixKit.hpp"

struct Color {
  double r, g, b;
};

struct Background {
  double Br{0.0}, Bg{0.0}, Bb{0.0};
};

struct Ambient {
  double KaIar, KaIag, KaIab;
};

struct Light {
  double Ipr, Ipg, Ipb, Ix, Iy, Iz;
  auto pos() const { return Vector<4>{Ix, Iy, Iz, 0.0}; }
};

struct Polygon_au {
  Polygon_u<4> polygon;
  Color color;
};

using Polygons_au = std::vector<Polygon_au>;

// apply a transformation to a vector of vertices
inline auto operator*(const Matrix<4>& t, const Polygons_au& ps) {
  Polygons_au ret{ps.size()};
  std::transform(std::execution::par_unseq, ps.begin(), ps.end(), ret.begin(), [&](const Polygon_au& vs) { return Polygon_au{t * vs.polygon, vs.color}; });
  return ret;
}
