#pragma once

#include <GL/glut.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <execution>
#include <iostream>
#include <utility>
#include <vector>

constexpr int point_size{1}; // the number of pixels a point maps to

template<std::size_t N>
using Matrix = std::array<std::array<double, N>, N>;
template<std::size_t N>
using Vector = std::array<double, N>;
template<std::size_t N>
using Vertices = std::vector<Vector<N>>;
template<std::size_t N>
using Polygons = std::vector<Vertices<N>>;

// just want a constexpr swap function
template<typename T>
constexpr auto swap(T& a, T& b) {
  const T temp{std::move(a)};
  a = std::move(b);
  b = std::move(temp);
}

// transpose a matrix
template<typename T, std::size_t N>
constexpr auto transpose(const std::array<std::array<T, N>, N>& a) {
  auto m = a;
  for (std::size_t i = 0; i < N; ++i)
    for (std::size_t j = 0; j < i; ++j)
      swap(m[i][j], m[j][i]);
  return m;
}

// standard inner product
template<typename T, std::size_t N>
constexpr auto operator*(const std::array<T, N>& lhs, const std::array<T, N>& rhs) {
  return std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0.0);
}

// matrix multiplication
template<typename T, std::size_t N>
constexpr auto operator*(const std::array<std::array<T, N>, N>& lhs, const std::array<std::array<T, N>, N>& rhs) {
  std::array<std::array<T, N>, N> product{};
  auto rhst{transpose(rhs)};
  for (std::size_t i = 0; i < N; ++i)
    for (std::size_t j = 0; j < N; ++j)
      product[i][j] = lhs[i] * rhst[j];
  return product;
}

// matrix X vector ⟼ vector
template<typename T, std::size_t N>
constexpr auto operator*(const std::array<std::array<T, N>, N>& lhs, const std::array<T, N>& rhs) {
  std::array<T, N> ret{};
  std::transform(std::execution::seq,
                 lhs.begin(),
                 lhs.end(),
                 ret.begin(),
                 [&](auto l) { return l * rhs; });
  return ret;
}

// scalar X vector ⟼ vector
template<typename Scalar, typename T, std::size_t N>
constexpr auto operator*(const Scalar lhs, const std::array<T, N>& rhs) {
  std::array<T, N> ret{};
  std::transform(std::execution::seq,
                 rhs.begin(),
                 rhs.end(),
                 ret.begin(),
                 [&](auto r) { return lhs * r; });
  return ret;
}

// print a vector, has a trailing newline
template<typename T, std::size_t N>
constexpr auto operator<<(std::ostream& out, const std::array<T, N>& v) -> std::ostream& {
  for (const auto& a : v)
    out << "[ " << a << " ]" << '\n';
  return out;
}

// print a matrix, has a trailing newline
template<typename T, std::size_t N>
constexpr auto operator<<(std::ostream& out, const std::array<std::array<T, N>, N>& m) -> std::ostream& {
  for (const auto& row : m) {
    out << "[ ";
    for (const auto& scalar : row)
      out << scalar << " ";
    out << "]\n";
  }
  return out;
}

// print all vertices of a polygon
template<std::size_t N>
inline auto operator<<(std::ostream& out, const Vertices<N>& vs) -> std::ostream& {
  for (const auto& v : vs)
    out << '(' << v[0] << ", " << v[1] << ")\n";
  return out;
}

// vector - vector ⟼ vector
template<typename T, std::size_t N>
constexpr auto operator-(const std::array<T, N>& lhs, const std::array<T, N>& rhs) {
  std::array<T, N> ret{};
  std::transform(std::execution::seq,
                 lhs.begin(),
                 lhs.end(),
                 rhs.begin(),
                 ret.begin(),
                 [](auto l, auto r) { return l - r; });
  return ret;
}

// vector + vector ⟼ vector
template<typename T, std::size_t N>
constexpr auto operator+(const std::array<T, N>& lhs, const std::array<T, N>& rhs) {
  std::array<T, N> ret{};
  std::transform(std::execution::seq,
                 lhs.begin(),
                 lhs.end(),
                 rhs.begin(),
                 ret.begin(),
                 [](auto l, auto r) { return l + r; });
  return ret;
}

// return a scaling matrix
template<std::size_t N>
constexpr auto s_m(const double sx, const double sy, [[maybe_unused]] const double sz = 0) -> Matrix<N> {
  if constexpr (N == 3)
    return {{{sx, 0, 0},
             {0, sy, 0},
             {0, 0, 1}}};
  else if constexpr (N == 4)
    return {{{sx, 0, 0, 0},
             {0, sy, 0, 0},
             {0, 0, sz, 0},
             {0, 0, 0, 1}}};
}

// return a translation matrix
template<std::size_t N>
constexpr auto t_m(const double dx, const double dy, [[maybe_unused]] const double dz = 0) -> Matrix<N> {
  if constexpr (N == 3)
    return {{{1, 0, dx},
             {0, 1, dy},
             {0, 0, 1}}};
  else if constexpr (N == 4)
    return {{{1, 0, 0, dx},
             {0, 1, 0, dy},
             {0, 0, 1, dz},
             {0, 0, 0, 1}}};
}

// return a rotation matrix
template<std::size_t N>
inline auto r_m(const double radian, [[maybe_unused]] const char axis = 'z') -> Matrix<N> {
  const double c{std::cos(radian)};
  const double s{std::sin(radian)};
  if constexpr (N == 3)
    return {{{c, -s, 0},
             {s, c, 0},
             {0, 0, 1}}};
  else if constexpr (N == 4) {
    switch (axis) {
    case 'z':
      return {{{c, -s, 0, 0},
               {s, c, 0, 0},
               {0, 0, 1, 0},
               {0, 0, 0, 1}}};
    case 'x':
      return {{{1, 0, 0, 0},
               {0, c, -s, 0},
               {0, s, c, 0},
               {0, 0, 0, 1}}};
    case 'y':
      return {{{c, 0, s, 0},
               {0, 1, 0, 0},
               {-s, 0, c, 0},
               {0, 0, 0, 1}}};
    }
  }
}

template<std::size_t N>
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

template<std::size_t N>
inline auto draw_polygon(const Vertices<N>& vs) {
  if (!vs.size())
    return;
  draw_line(vs.front(), vs[1]);
  draw_line(vs.front(), vs.back());
  for (auto it = vs.cbegin() + 1; it != vs.cend(); ++it)
    draw_line(*(it - 1), *it);
}

template<std::size_t N>
inline auto draw_polygons(const Polygons<N>& ps) {
  for (const auto& vs : ps)
    draw_polygon(vs);
}

// apply a transformation to all vertices of a polygon
template<std::size_t N>
inline auto transformed_vs(const Matrix<N>& t, const Vertices<N>& vs) -> Vertices<N> {
  Vertices<N> ret{vs.size()};
  std::transform(std::execution::seq,
                 vs.cbegin(),
                 vs.cend(),
                 ret.begin(),
                 [&](auto v) { return t * v; });
  return ret;
}

// apply a transformation to polygons
template<std::size_t N>
inline auto transformed_ps(const Matrix<N>& t, const Polygons<N>& ps) -> Polygons<N> {
  auto size = ps.size();
  Polygons<N> ret{size};
  if (50 < size)
    std::transform(std::execution::par_unseq,
                   ps.begin(),
                   ps.end(),
                   ret.begin(),
                   [&](auto& vs) { return transformed_vs(t, vs); });
  else
    std::transform(std::execution::seq,
                   ps.begin(),
                   ps.end(),
                   ret.begin(),
                   [&](auto& vs) { return transformed_vs(t, vs); });
  return ret;
}

#define clear_screen()              \
  glClearColor(0.0, 0.0, 0.0, 0.0); \
  glClear(GL_COLOR_BUFFER_BIT);     \
  glFlush()
