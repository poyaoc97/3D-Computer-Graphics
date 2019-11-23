#pragma once
#include "MatrixKit.hpp"
#include <GL/glut.h>
#include <mutex>

constexpr int point_size{1}; // the number of pixels a point maps to

template<size_t N>
auto draw_line(const Vector<N>& endpoint1, const Vector<N>& endpoint2) {
  // unpack data
  int x1{static_cast<int>(std::round(endpoint1[0]))};
  int y1{static_cast<int>(std::round(endpoint1[1]))};
  int x2{static_cast<int>(std::round(endpoint2[0]))};
  int y2{static_cast<int>(std::round(endpoint2[1]))};
  // drawing logic begins
  if (x2 < x1) { // let (x1, y1) be the point on the left
    swap(x1, x2);
    swap(y1, y2);
  }
  glPointSize(point_size);
  glBegin(GL_POINTS);
  glVertex2i(x1, y1); // draw the first point no matter wut

  // preprocessing
  const bool neg_slope{y1 > y2};
  if (neg_slope)         // see if the slope is negative
    y2 += 2 * (y1 - y2); // mirror the line with respect to y = y1 for now, and voila, a positive slope line!
                         // we draw this line as if the slope was positive in our head and draw it "upside down" on the screen in the while loop
  const bool slope_grt_one{(y2 - y1) > (x2 - x1)};
  if (slope_grt_one) { // slope greater than 1, swap x and y,  mirror the line with respect to y = x for now, mirror it again when drawing
    swap(x1, y1);
    swap(x2, y2);
  }
  int x{x1};
  int y{y1};
  const int a{y2 - y1};
  const int b{x1 - x2};
  int d{2 * a + b};
  while (x < x2) { // draw from left to right (recall that we make x2 be always on the right)
    if (d <= 0) {  // choose E
      d += 2 * a;
      if (slope_grt_one)                                    // sort of "mirror" the negative slope line back to where it's supposed to be
        glVertex2i(y, !neg_slope ? (++x) : (2 * x1 - ++x)); // slope > 1 y is actually x and vice versa
      else
        glVertex2i(++x, !neg_slope ? (y) : (2 * y1 - y));
    } else { // choose NE
      d += 2 * (a + b);
      if (slope_grt_one)
        glVertex2i(++y, !neg_slope ? (++x) : (2 * x1 - ++x));
      else
        glVertex2i(++x, !neg_slope ? (++y) : (2 * y1 - ++y));
    }
  }
  glEnd();
}

template<size_t N>
inline auto draw_polygon(const Polygon_u<N>& vs) {
  if (!vs.size())
    return;

  draw_line(vs.front(), vs[1]);
  draw_line(vs.front(), vs.back());
  for (auto it = vs.cbegin() + 1; it != vs.cend(); ++it)
    draw_line(*(it - 1), *it);

  glFlush();
}

template<size_t N>
inline auto draw_polygons(const Polygons<N>& ps) {
  for (const auto& vs : ps)
    draw_polygon(vs);
}

inline auto project_clip_pd(const Polygons<4>& polygons, const Matrix<4>& pmXem) {
  using Codes = std::vector<std::function<double(Vector<4>)>>;
  const Codes codes{{[](const Vector<4>& v) { return v[3] - v[0]; }, [](const Vector<4>& v) { return v[3] + v[0]; }, // w - x, w + x
                     [](const Vector<4>& v) { return v[3] - v[1]; }, [](const Vector<4>& v) { return v[3] + v[1]; }, // w - y, w + y
                     [](const Vector<4>& v) { return v[3] - v[2]; }, [](const Vector<4>& v) { return v[2]; }}};      // w - z, w
  Polygons<4> clipped_polys;
  std::mutex lock_clipped_polys;

  std::for_each(std::execution::par_unseq, polygons.begin(), polygons.end(), [&](Polygon_u<4> polygon) {
    Polygon_u<4> relay;
    polygon = transformed_vs(pmXem, polygon); // project a polygon to projection space

    for (const auto& c : codes) { // clip against all six planes
      const auto sz = polygon.size();
      for (size_t i = 0; i < sz; ++i) {
        const auto& s = polygon[i];
        const auto& p = polygon[(i + 1) % sz];
        if (const double c1{c(s)}, c2{c(p)}; c1 >= 0 && c2 >= 0) { // in2in
          relay.push_back(p);
        } else if (const auto new_s = s + c1 / (c1 - c2) * (p - s); c1 >= 0 && c2 < 0) { // in2out
          relay.push_back(new_s);
        } else if (c1 < 0 && c2 >= 0) { // out2in
          relay.push_back(new_s);
          relay.push_back(p);
        }
      }
      polygon = relay;
      relay.clear();
    }

    if (polygon.size()) {
      for (auto& a : polygon) // perspective division
        if (a[3] != 1)
          a = Vector<4>{{a[0] / a[3], a[1] / a[3], a[2] / a[3], 1.0}};
      std::lock_guard l{lock_clipped_polys};
      clipped_polys.push_back(std::move(polygon));
    }
  });
  return clipped_polys;
}
