#pragma once
#include "MatrixKit.hpp"
#include <fstream>
#include <sstream>

using Face = std::vector<int>;

class Object {
  std::string file_name;
  size_t v_count;
  size_t f_count;
  std::vector<Vector<4>> vertices;
  std::vector<Face> faces;

public:
  explicit Object(std::string_view s, size_t v, size_t f) : file_name{s}, v_count{v}, f_count{f}, vertices{v}, faces{f} {}

  auto set_vertex(std::stringstream& ss, std::ifstream& asc_file, const Matrix<4>& TM);
  auto set_face(std::stringstream& ss, std::ifstream& asc_file);

  // turn faces into polygons
  auto to_polygons() const;

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
  Polygons<4> polygons{f_count};
  auto it = polygons.begin();
  for (const auto& face : faces) {
    for (const auto& i : face)
      (*it).emplace_back(get_v(i));
    ++it;
  }
  return polygons;
}
