#pragma once
#include <array>
#include <cmath>
#include <execution>
#include <iostream>

using namespace std::chrono;

template<size_t N, typename T = double>
using Matrix = std::array<std::array<T, N>, N>;

template<size_t N, typename T = double>
using Vector = std::array<T, N>;

template<size_t N>
using Polygon_u = std::vector<Vector<N>>;

template<size_t N>
using Polygons = std::vector<Polygon_u<N>>;

constexpr double pi{3.141'592'653'589'793'238'46};
constexpr double piDiv180{pi / 180};
constexpr Matrix<4> identity_matrix{{{1, 0, 0, 0},
                                     {0, 1, 0, 0},
                                     {0, 0, 1, 0},
                                     {0, 0, 0, 1}}};

// just want a constexpr swap function
template<typename T>
constexpr auto swap(T& a, T& b) {
  const T temp{std::move(a)};
  a = std::move(b);
  b = std::move(temp);
}

// transpose a matrix
template<size_t N, typename T = double>
constexpr auto transpose(const Matrix<N, T>& a) {
  auto m = a;
  for (size_t i = 0; i < N; ++i)
    for (size_t j = 0; j < i; ++j)
      swap(m[i][j], m[j][i]);
  return m;
}

// standard inner product
template<size_t N, typename T = double>
constexpr auto operator*(const Vector<N, T>& lhs, const Vector<N, T>& rhs) {
  return std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0.0);
}

// matrix multiplication
template<size_t N, typename T = double>
constexpr auto operator*(const Matrix<N, T>& lhs, const Matrix<N, T>& rhs) {
  std::array<std::array<T, N>, N> product{};
  auto rhst{transpose(rhs)};
  for (size_t i = 0; i < N; ++i)
    for (size_t j = 0; j < N; ++j)
      product[i][j] = lhs[i] * rhst[j];
  return product;
}

// matrix X vector ⟼ vector
template<size_t N, typename T = double>
constexpr auto operator*(const Matrix<N, T>& lhs, const Vector<N, T>& rhs) {
  std::array<T, N> ret{};
  std::transform(lhs.begin(), lhs.end(), ret.begin(), [&](const auto& l) { return l * rhs; });
  return ret;
}

// scalar X vector ⟼ vector
template<typename Scalar, size_t N, typename T = double>
constexpr auto operator*(const Scalar lhs, const Vector<N, T>& rhs) {
  std::array<T, N> ret{};
  std::transform(rhs.begin(), rhs.end(), ret.begin(), [&](const auto& r) { return lhs * r; });
  return ret;
}

// print a vector, has a trailing newline
template<size_t N, typename T = double>
constexpr auto operator<<(std::ostream& out, const Vector<N, T>& v) -> std::ostream& {
  for (const auto& a : v)
    out << "[ " << a << " ]" << '\n';
  return out;
}

// print a matrix, has a trailing newline
template<size_t N, typename T = double>
constexpr auto operator<<(std::ostream& out, const Matrix<N, T>& m) -> std::ostream& {
  for (const auto& row : m) {
    out << "[ ";
    for (const auto& scalar : row)
      out << scalar << " ";
    out << "]\n";
  }
  return out;
}

// print all vertices of a polygon
template<size_t N>
inline auto operator<<(std::ostream& out, const Polygon_u<N>& vs) -> std::ostream& {
  for (const auto& v : vs)
    out << '(' << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")\n";
  return out;
}

// vector - vector ⟼ vector
template<size_t N, typename T = double>
constexpr auto operator-(const Vector<N, T>& lhs, const Vector<N, T>& rhs) {
  std::array<T, N> ret{};
  std::transform(lhs.begin(), lhs.end(), rhs.begin(), ret.begin(), [](const auto l, const auto r) { return l - r; });
  return ret;
}

// vector + vector ⟼ vector
template<size_t N, typename T = double>
constexpr auto operator+(const Vector<N, T>& lhs, const Vector<N, T>& rhs) {
  std::array<T, N> ret{};
  std::transform(lhs.begin(), lhs.end(), rhs.begin(), ret.begin(), [](const auto l, const auto r) { return l + r; });
  return ret;
}

// return a scaling matrix
template<size_t N>
[[nodiscard]] constexpr auto scaling_m(const double sx, const double sy, [[maybe_unused]] const double sz = 0) {
  if constexpr (N == 3)
    return Matrix<N>{{{sx, 0, 0},
                      {0, sy, 0},
                      {0, 0, 1}}};
  else if constexpr (N == 4)
    return Matrix<N>{{{sx, 0, 0, 0},
                      {0, sy, 0, 0},
                      {0, 0, sz, 0},
                      {0, 0, 0, 1}}};
}

// return a translation matrix
template<size_t N>
[[nodiscard]] constexpr auto translation_m(const double dx, const double dy, [[maybe_unused]] const double dz = 0) {
  if constexpr (N == 3)
    return Matrix<N>{{{1, 0, dx},
                      {0, 1, dy},
                      {0, 0, 1}}};
  else if constexpr (N == 4)
    return Matrix<N>{{{1, 0, 0, dx},
                      {0, 1, 0, dy},
                      {0, 0, 1, dz},
                      {0, 0, 0, 1}}};
}

// return a rotation matrix
template<size_t N>
[[nodiscard]] inline auto rotation_m(double degree, [[maybe_unused]] const char axis = 'z') {
  degree *= piDiv180;
  const double c{std::cos(degree)};
  const double s{std::sin(degree)};
  if constexpr (N == 3)
    return Matrix<N>{{{c, -s, 0},
                      {s, c, 0},
                      {0, 0, 1}}};
  else if constexpr (N == 4) {
    switch (axis) {
    case 'z':
      return Matrix<N>{{{c, -s, 0, 0},
                        {s, c, 0, 0},
                        {0, 0, 1, 0},
                        {0, 0, 0, 1}}};
    case 'x':
      return Matrix<N>{{{1, 0, 0, 0},
                        {0, c, -s, 0},
                        {0, s, c, 0},
                        {0, 0, 0, 1}}};
    case 'y':
      return Matrix<N>{{{c, 0, s, 0},
                        {0, 1, 0, 0},
                        {-s, 0, c, 0},
                        {0, 0, 0, 1}}};
    default:
      std::cerr << "rotation return identity!\n";
      return identity_matrix;
    }
  }
}

// apply a transformation to all vertices of a polygon
template<size_t N>
[[nodiscard]] inline auto transformed_vs(const Matrix<N>& t, const Polygon_u<N>& vs) {
  Polygon_u<N> ret{vs.size()};
  std::transform(vs.begin(), vs.end(), ret.begin(), [&](const auto& v) { return t * v; });
  return ret;
}

// apply a transformation to a vector of vertices
template<size_t N>
[[nodiscard]] inline auto transformed_ps(const Matrix<N>& t, const Polygons<N>& ps) {
  Polygons<N> ret{ps.size()};
  std::transform(std::execution::par_unseq, ps.begin(), ps.end(), ret.begin(), [&](const auto& vs) { return transformed_vs(t, vs); });
  return ret;
}

// cross product, ignore the 4th component and set it to zero
inline auto cross(const Vector<4>& a, const Vector<4>& b) {
  return Vector<4>{{a[1] * b[2] - b[1] * a[2], a[2] * b[0] - b[2] * a[0], a[0] * b[1] - b[0] * a[1], 0}};
}

template<size_t N>
auto normalize(const Vector<N>& a) {
  double norm = std::sqrt(a * a); // square root of <a, a>
  Vector<N> b;
  std::transform(a.begin(), a.end(), b.begin(), [&](auto a) { return a / norm; });
  return b;
}
