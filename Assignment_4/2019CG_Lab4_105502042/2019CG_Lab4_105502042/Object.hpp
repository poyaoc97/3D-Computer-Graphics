#pragma once
#include "Lighting.hpp"
#include "MatrixKit.hpp"
#include <fstream>
#include <sstream>

using Face = std::vector<int>;

class Object {
  size_t v_count;
  size_t f_count;
  std::vector<Vector<4>> vertices;
  std::vector<Face> faces;
  double Or, Og, Ob, Kd, Ks;
  int N;

public:
  explicit Object(size_t v, size_t f, double Or, double Og, double Ob, double Kd, double Ks, int N)
      : v_count{v}, f_count{f}, vertices{v}, faces{f}, Or{Or}, Og{Og}, Ob{Ob}, Kd{Kd}, Ks{Ks}, N{N} {}

  auto set_vertex(std::stringstream& ss, std::ifstream& asc_file, const Matrix<4>& TM);
  auto set_face(std::stringstream& ss, std::ifstream& asc_file);

  // turn faces into polygons_au
  auto to_polygons() const;

  auto get_lighting_info() const { return std::tuple{Or, Og, Ob, Kd, Ks, N}; }

private:
  auto get_v(const int i) const { return vertices[i - 1]; }
};

inline auto Object::set_vertex(std::stringstream& ss, std::ifstream& asc_file, const Matrix<4>& TM) {
  std::string line;
  double a, b, c;
  auto it = vertices.begin();
  for (size_t i = 0; i != v_count; ++i, ++it) {
    while (ss >> line)
      ;
    line.clear();
    ss.clear();

    std::getline(asc_file, line);
    ss << line;
    ss >> a >> b >> c;
    (*it) = TM * Vector<4>{{a, b, c, 1}};
  }
}

inline auto Object::set_face(std::stringstream& ss, std::ifstream& asc_file) {
  std::string line;
  int n, a, b, c, d;
  auto it = faces.begin();
  for (size_t i = 0; i != f_count; ++i, ++it) {
    while (ss >> line)
      ;
    line.clear();
    ss.clear();

    std::getline(asc_file, line);
    ss << line;
    ss >> n;
    if (n == 3) {
      ss >> a >> b >> c;
      (*it) = Face{{a, b, c}};
    } else {
      ss >> a >> b >> c >> d;
      (*it) = Face{{a, b, c, d}};
    }
  }
}

inline auto Object::to_polygons() const {
  Polygons_au polygons{f_count};
  auto it = polygons.begin();
  for (const auto& face : faces) {
    for (const auto& i : face)
      (*it).polygon.emplace_back(get_v(i));
    ++it;
  }
  return polygons;
}
