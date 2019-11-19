#pragma once
#include <array>
#include <chrono>
#include <cmath>
#include <execution>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std::chrono;

template<std::size_t N>
using Matrix = std::array<std::array<double, N>, N>;
template<std::size_t N, typename T = double>
using Vector = std::array<T, N>;
template<std::size_t N>
using Vertices = std::vector<Vector<N>>;
template<std::size_t N>
using Polygons = std::vector<Vertices<N>>;

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
    out << '(' << v[0] << ", " << v[1] << ", " << v[2] << ")\n";
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
[[nodiscard]] constexpr auto scaling_m(const double sx, const double sy, [[maybe_unused]] const double sz = 0) -> Matrix<N> {
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
[[nodiscard]] constexpr auto translation_m(const double dx, const double dy, [[maybe_unused]] const double dz = 0) -> Matrix<N> {
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
[[nodiscard]] inline auto rotation_m(double degree, [[maybe_unused]] const char axis = 'z') -> Matrix<N> {
  degree *= piDiv180;
  const double c{std::cos(degree)};
  const double s{std::sin(degree)};
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
    default:
      std::cerr << "rotation return identity!\n";
      return identity_matrix;
    }
  }
}

// apply a transformation to all vertices of a polygon
template<std::size_t N>
[[nodiscard]] inline auto transformed_vs(const Matrix<N>& t, const Vertices<N>& vs) -> Vertices<N> {
  auto size = vs.size();
  // std::cout << "Number of vertices: " << size << '\n';
  Vertices<N> ret{size};
  std::transform(std::execution::seq,
                 vs.cbegin(),
                 vs.cend(),
                 ret.begin(),
                 [&](auto v) { return t * v; });
  return ret;
}

// apply a transformation to a vector of vertices
template<std::size_t N>
[[nodiscard]] inline auto transformed_ps(const Matrix<N>& t, const Polygons<N>& ps) -> Polygons<N> {
  auto size = ps.size();
  // std::cout << "Number of polygons: " << size << '\n';
  Polygons<N> ret{size};
  if (100 < size) // arbitrary threshold
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

inline auto cross(const Vector<4>& a, const Vector<4>& b) -> Vector<4> {
  return {{a[1] * b[2] - b[1] * a[2],
           a[2] * b[0] - b[2] * a[0],
           a[0] * b[1] - b[0] * a[1],
           0}};
}

template<size_t N>
auto normalize(const Vector<N>& a) -> Vector<N> {
  double norm = std::sqrt(a * a);
  Vector<N> b;
  std::transform(a.begin(), a.end(), b.begin(), [=](auto a) { return a / norm; });
  return b;
}
