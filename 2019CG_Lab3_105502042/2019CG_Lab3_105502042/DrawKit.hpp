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
  bool neg_slope{y1 > y2};
  if (neg_slope)         // see if the slope is negative
    y2 += 2 * (y1 - y2); // mirror the line with respect to y = y1 for now, and voila, a positive slope line!
                         // we draw this line as if the slope was positive in our head and draw it "upside down" on the screen in the while loop
  bool slope_grt_one{(y2 - y1) > (x2 - x1)};
  if (slope_grt_one) { // slope greater than 1, swap x and y,  mirror the line with respect to y = x for now, mirror it again when drawing
    swap(x1, y1);
    swap(x2, y2);
  }
  int x{x1};
  int y{y1};
  int a{y2 - y1};
  int b{x1 - x2};
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
inline auto draw_polygon(const Vertices<N>& vs) {
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

inline auto clear_screen_without_flush() {
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  // glFlush();
}

template<typename Predicate1, typename Predicate2, typename Predicate3>
auto clip_one_edge(const Polygons<4>& polygons, Polygons<4>& clipped_polys, const double boundary, const size_t comp,
                   const Predicate1& in2in, const Predicate2& in2out, const Predicate3& out2in) {
  std::mutex lock;

  std::for_each(std::execution::par_unseq, polygons.begin(), polygons.end(), [&](const auto& vs) {
    const auto sz = vs.size();
    Vertices<4> clipped_vs;
    for (size_t i = 0; i < sz; ++i) {
      const auto& s = vs[i];
      const auto& p = vs[(i + 1) % sz];
      const auto sc = s[comp];
      const auto pc = p[comp];
      if (auto dir = p - s; in2in(sc, pc))
        clipped_vs.push_back(p);
      else if (in2out(sc, pc))
        clipped_vs.push_back(s + std::abs((boundary - sc) / dir[comp]) * dir);
      else if (out2in(sc, pc)) {
        clipped_vs.push_back(s + std::abs((boundary - sc) / dir[comp]) * dir);
        clipped_vs.push_back(p);
      }
    }
    std::lock_guard<std::mutex> l{lock};
    clipped_polys.push_back(std::move(clipped_vs));
  });
}

inline auto clipped_polygons(const Polygons<4>& polygons) {
  // auto t0 = high_resolution_clock::now();
  constexpr auto x = 0;
  constexpr auto y = 1;

  auto Pred_t_1 = [](const auto s, const auto p) { return s <= 1 && p <= 1; };
  auto Pred_t_2 = [](const auto s, const auto p) { return s <= 1 && p > 1; };
  auto Pred_t_3 = [](const auto s, const auto p) { return s > 1 && p <= 1; };

  auto Pred_b_1 = [](const auto s, const auto p) { return s >= -1 && p >= -1; };
  auto Pred_b_2 = [](const auto s, const auto p) { return s >= -1 && p < -1; };
  auto Pred_b_3 = [](const auto s, const auto p) { return s < -1 && p >= -1; };

  Polygons<4> a;
  Polygons<4> clipped_polys;
  clip_one_edge(polygons, clipped_polys, 1, x, Pred_t_1, Pred_t_2, Pred_t_3);
  a = std::move(clipped_polys);
  clipped_polys.clear();

  clip_one_edge(a, clipped_polys, 1, y, Pred_t_1, Pred_t_2, Pred_t_3);
  a = std::move(clipped_polys);
  clipped_polys.clear();

  clip_one_edge(a, clipped_polys, -1, x, Pred_b_1, Pred_b_2, Pred_b_3);
  a = std::move(clipped_polys);
  clipped_polys.clear();

  clip_one_edge(a, clipped_polys, -1, y, Pred_b_1, Pred_b_2, Pred_b_3);
  // auto t1 = high_resolution_clock::now();
  // when switch to release mode this go from 309ms to 4ms!
  // std::cout << "clipped_polygons() takes: " << duration_cast<milliseconds>(t1 - t0).count() << "ms\n";
  return clipped_polys;
}
