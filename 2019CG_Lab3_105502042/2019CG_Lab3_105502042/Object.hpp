#pragma once
#include "MatrixKit.hpp"

using Face = std::vector<int>;

class Object {
  std::string file_name;
  size_t v_count;
  size_t f_count;
  Vertices<4> vs;
  std::vector<Face> faces;

public:
  Object(std::string_view s, size_t v, size_t f) : file_name{s}, v_count{v}, f_count{f}, vs{v}, faces{f} {}
  auto set_vertex(std::stringstream& ss, std::ifstream& asc_file, const Matrix<4>& TM);
  auto set_face(std::stringstream& ss, std::ifstream& asc_file);
  [[nodiscard]] auto get_name() const { return file_name; }

  // turn faces into polygons
  [[nodiscard]] auto get_polygons() const -> Polygons<4>;

private:
  friend auto operator<<(std::ostream& out, const Object& obj) const -> std::ostream&;
  [[nodiscard]] auto get_v(int i) const { return vs[i - 1]; }
};

inline auto Object::set_vertex(std::stringstream& ss, std::ifstream& asc_file, const Matrix<4>& TM) {
  std::string line;
  double a, b, c;
  auto it = vs.begin();
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

inline auto Object::get_polygons() -> Polygons<4> {
  Polygons<4> polygons{f_count};
  auto it = polygons.begin();
  for (const auto& face : faces) {
    for (const auto& i : face)
      (*it).push_back(get_v(i));
    ++it;
  }
  return polygons;
}

inline auto operator<<(std::ostream& out, const Object& obj) -> std::ostream& {
  out << "Object name: " << obj.file_name << '\n'
      << "Vertex count: " << obj.v_count << '\n'
      << "Face count: " << obj.f_count << "\n"
      << obj.vs;

  for (const auto& f : obj.faces) {
    out << f.size() << '\t';
    for (auto it = f.begin(); it != f.end(); ++it)
      out << *it << ' ';
    out << '\n';
  }
  out << "end of object print\n";
  return out;
}
