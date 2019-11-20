#pragma once
#include "DrawKit.hpp"
#include "Object.hpp"
#include "Observer.hpp"
#include "Viewport.hpp"
#include <fstream>
#include <sstream>

int win_x, win_y;
bool nobackfaces{false};

// simple hash function
[[nodiscard]] constexpr auto operator""_hash(const char* s, const std::size_t count) -> std::size_t {
  return *s ^ count;
}

inline auto process_scale(std::stringstream& ss) {
  double x, y, z;
  ss >> x >> y >> z;
  return scaling_m<4>(x, y, z);
}

inline auto process_rotate(std::stringstream& ss) {
  double x, y, z;
  ss >> x >> y >> z;
  if (x)
    return rotation_m<4>(x, 'x');
  else if (y)
    return rotation_m<4>(y, 'y');
  else
    return rotation_m<4>(z);
}

inline auto process_translate(std::stringstream& ss) {
  double x, y, z;
  ss >> x >> y >> z;
  return translation_m<4>(x, y, z);
}

inline auto process_viewport(std::stringstream& ss) {
  double vxl, vxr, vyb, vyt;
  ss >> vxl >> vxr >> vyb >> vyt;
  //std::cout << Vector<4>{vxl, vxr, vyb, vyt};
  return Viewport{(vxr - vxl) / (vyt - vyb), vxl, vxr, vyb, vyt, win_x, win_y};
}

inline auto process_object(std::stringstream& ss, const Matrix<4>& TM) {
  // open asc file
  std::string asc_path;
  ss >> asc_path;

  std::ifstream asc_file{asc_path};
  if (!asc_file) {
    asc_path = "../Debug/" + asc_path;
    asc_file.open(asc_path);
  }

  // read it
  std::string line;
  std::getline(asc_file, line);
  if (line == "")
    std::getline(asc_file, line);
  std::stringstream asc_ss;
  asc_ss << line;
  size_t v, f;
  asc_ss >> v >> f;

  Object asc_obj{asc_path, v, f};
  asc_obj.set_vertex(asc_ss, asc_file, TM);
  asc_obj.set_face(asc_ss, asc_file);

  return asc_obj;
}

inline auto process_observer(std::stringstream& ss) {
  double Ex, Ey, Ez, COIx, COIy, COIz, Tilt, Hither, Yon, Hav;
  ss >> Ex >> Ey >> Ez >> COIx >> COIy >> COIz >> Tilt >> Hither >> Yon >> Hav;
  return Observer{Ex, Ey, Ez, COIx, COIy, COIz, Tilt, Hither, Yon, Hav};
}

inline auto process_display(std::stringstream& ss, const Viewport& vp, std::vector<Object>& objects, const Matrix<4>& pmXem) {
  auto t0 = high_resolution_clock::now();
  // dump all faces of all objects to Polygons<4>
  Polygons<4> ps;
  for (const auto& obj : objects) {
    auto a = obj.to_polygons();
    ps.insert(ps.end(), a.begin(), a.end());
  }

  // pmXem and perspective divide
  std::transform(std::execution::par_unseq, ps.begin(), ps.end(), ps.begin(), [&](const auto& ps) {
    auto ret{transformed_vs(pmXem, ps)};
    for (auto& a : ret)
      if (a[3] != 1)
        a = Vector<4>{{a[0] / a[3], a[1] / a[3], a[2] / a[3], 1.0}};
    return ret;
  });

  // nobackfaces
  if (nobackfaces)
    ps.erase(std::remove_if(std::execution::par_unseq, ps.begin(), ps.end(), [](const auto& a) {
               return cross(a[1] - a[0], a[2] - a[1])[2] >= 0;
             }),
             ps.end());

  const auto [vxl, vxr, vyb, vyt] = vp.get_borders();
  clear_screen_without_flush();
  draw_polygon(Polygon_u<2>{{{vxl, vyb}, {vxr, vyb}, {vxr, vyt}, {vxl, vyt}}});
  draw_polygons((transformed_ps(Matrix<4>{translation_m<4>(vxl, vyb) *
                                          scaling_m<4>((vxr - vxl) / 2.0, (vyt - vyb) / 2.0) *
                                          translation_m<4>(1.0, 1.0)},
                                clipped_polygons(ps))));
  glFlush();
  auto t1 = high_resolution_clock::now();
  std::cout << "display() takes: " << duration_cast<milliseconds>(t1 - t0).count() << "ms\n";

  system("pause");
}
