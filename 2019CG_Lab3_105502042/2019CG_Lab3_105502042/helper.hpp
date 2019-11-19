#pragma once
#include "DrawKit.hpp"
#include "Object.hpp"
#include <tuple>

int win_x, win_y;
bool nobackfaces{false};

// simple hash function
[[nodiscard]] constexpr auto operator""_hash(const char* s, const std::size_t count) -> std::size_t {
  return *s ^ count;
}

inline auto process_scale(std::stringstream& ss, Matrix<4>& t) {
  double x, y, z;
  ss >> x >> y >> z;
  t = scaling_m<4>(x, y, z) * t;
}

inline auto process_rotate(std::stringstream& ss, Matrix<4>& t) {
  double x, y, z;
  ss >> x >> y >> z;
  if (x)
    t = rotation_m<4>(x, 'x') * t;
  else if (y)
    t = rotation_m<4>(y, 'y') * t;
  else
    t = rotation_m<4>(z) * t;
}

inline auto process_translate(std::stringstream& ss, Matrix<4>& t) {
  double x, y, z;
  ss >> x >> y >> z;
  t = translation_m<4>(x, y, z) * t;
}

struct Viewport {
  double vxl, vxr, vyb, vyt;

  [[nodiscard]] auto get_borders() const -> std::tuple<double, double, double, double> {
    auto bx = [](auto v) { return (1 + v) * win_x / 2; };
    auto by = [](auto v) { return (1 + v) * win_y / 2; };
    return {bx(vxl), bx(vxr), by(vyb), by(vyt)};
  }
};

inline auto process_viewport(std::stringstream& ss, Viewport& vp) {
  ss >> vp.vxl >> vp.vxr >> vp.vyb >> vp.vyt;
}

inline auto process_object(std::stringstream& ss, std::vector<Object>& objects, const Matrix<4>& TM) {
  // open asc file
  std::string asc_path;
  ss >> asc_path;
  if (objects.size() && objects.back().get_name() == asc_path) {
    objects.push_back(objects.back());
    return;
  }

  std::ifstream asc_file{asc_path};
  if (!asc_file) {
    asc_path = "../Debug/" + asc_path;
    asc_file.open(asc_path);
  }

  // read it
  std::string line;
  std::getline(asc_file, line);
  std::stringstream asc_ss;
  asc_ss << line;

  size_t v, f;
  asc_ss >> v >> f;

  Object asc_obj{asc_path, v, f};
  asc_obj.set_vertex(asc_ss, asc_file, TM);
  asc_obj.set_face(asc_ss, asc_file);

  objects.push_back(asc_obj);
}

inline auto process_observer(std::stringstream& ss, const Viewport& vp, Matrix<4>& observer_M) {
  observer_M = identity_matrix;
  double Ex, Ey, Ez, COIx, COIy, COIz, Tilt, Hither, Yon, Hav;
  ss >> Ex >> Ey >> Ez >> COIx >> COIy >> COIz >> Tilt >> Hither >> Yon >> Hav;
  const Vector<4> top_vector{0, 1, 0, 0};
  Vector<4> vz{{COIx - Ex, COIy - Ey, COIz - Ez}};
  Vector<4> v1{cross(top_vector, vz)};
  Matrix<4> GRM{{normalize(v1),
                 normalize(cross(vz, v1)),
                 normalize(vz),
                 {0, 0, 0, 1}}};

  // construct mirror
  Matrix<4> mirror{identity_matrix};
  mirror[0][0] = -1;

  // construct PM, projection matrix
  Matrix<4> PM{identity_matrix};
  PM[1][1] = (vp.vxr - vp.vxl) / (vp.vyt - vp.vyb);
  PM[2][2] = Yon / (Yon - Hither) * std::tan(Hav * piDiv180);
  PM[2][3] = Hither * Yon / (Hither - Yon) * std::tan(Hav * piDiv180);
  PM[3][2] = std::tan(Hav * piDiv180);
  PM[3][3] = 0;

  observer_M = PM * rotation_m<4>(-Tilt) * mirror * GRM * translation_m<4>(-Ex, -Ey, -Ez);
}

inline auto process_display(std::stringstream& ss, const Viewport& vp, std::vector<Object>& objects, const Matrix<4>& pmXemXtm) {
  clear_screen();
  auto [vxl, vxr, vyb, vyt] = vp.get_borders();
  draw_polygon(Vertices<2>{{{vxl, vyb}, {vxr, vyb}, {vxr, vyt}, {vxl, vyt}}});

  Matrix<4> to_screen{translation_m<4>(vxl, vyb) *
                      scaling_m<4>((vxr - vxl) / 2.0, (vyt - vyb) / 2.0) *
                      translation_m<4>(1.0, 1.0)};

  for (auto& obj : objects) {
    auto a = transformed_ps(pmXemXtm, obj.get_polygons());
    std::transform(a.begin(), a.end(), a.begin(), [](auto a) {   //
      std::transform(a.begin(), a.end(), a.begin(), [](auto a) { //
        if (a[3])
          return Vector<4>{{a[0] / a[3], a[1] / a[3], a[2] / a[3], 1.0}};
        else
          return a;
      });
      return a;
    }); // perspective divide
    draw_polygons(transformed_ps(to_screen, clipped_polygons(a)));
  }

  glFlush();
  system("pause");
}
