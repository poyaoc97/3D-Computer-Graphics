#include "DrawKit.hpp"
#include "Object.hpp"
#include "Observer.hpp"
#include "Viewport.hpp"
#include <chrono>

std::ifstream in_file;
int win_x, win_y;
bool nobackfaces{false};

// simple hash function
[[nodiscard]] constexpr auto operator""_hash(const char* s, const size_t count) {
  return size_t{*s ^ count};
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

inline auto process_display(const Viewport& vp, const std::vector<Object>& objects, const Matrix<4>& pmXem) {
  auto t0 = high_resolution_clock::now();
  // dump all faces of all objects to Polygons<4>
  Polygons<4> ps;
  for (const auto& obj : objects) {
    const auto&& a = obj.to_polygons();
    ps.insert(ps.end(), a.begin(), a.end());
  }

  ps = project_clip_pd(ps, pmXem); // performs projection, clipping, and perspective division in parallel

  // nobackfaces
  if (nobackfaces)
    ps.erase(std::remove_if(std::execution::par_unseq, ps.begin(), ps.end(), [](const auto& a) {
               return cross(a[1] - a[0], a[2] - a[1])[2] >= 0;
             }),
             ps.end());

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  const auto [vxl, vxr, vyb, vyt] = vp.get_borders();
  draw_polygon(Polygon_u<2>{{{vxl, vyb}, {vxr, vyb}, {vxr, vyt}, {vxl, vyt}}}); // this function will do the flushing
  draw_polygons(transformed_ps(Matrix<4>{translation_m<4>(vxl, vyb) *
                                         scaling_m<4>((vxr - vxl) / 2.0, (vyt - vyb) / 2.0) *
                                         translation_m<4>(1.0, 1.0)},
                               ps));

  auto t1 = high_resolution_clock::now();
  std::cout << "display takes: " << duration_cast<milliseconds>(t1 - t0).count() << "ms\n";
  system("pause");
}

auto displayFunc() {
  std::stringstream ss;
  Matrix<4> TM{identity_matrix};
  Viewport vp;
  Observer ob_ov;
  std::vector<Object> objects;

  for (std::string line, str; std::getline(in_file, line);) {
    while (ss >> str)
      ;
    str.clear();
    ss.clear();
    ss << line;
    ss >> str;
    switch (operator""_hash(str.c_str(), str.size())) {
    case "scale"_hash:
      TM = process_scale(ss) * TM;
      break;
    case "rotate"_hash:
      TM = process_rotate(ss) * TM;
      break;
    case "translate"_hash:
      TM = process_translate(ss) * TM;
      break;
    case "viewport"_hash:
      vp = process_viewport(ss);
      break;
    case "object"_hash:
      objects.push_back(process_object(ss, TM));
      break;
    case "observer"_hash:
      ob_ov = process_observer(ss);
      break;
    case "display"_hash:
      process_display(vp, objects, ob_ov.get_pmXem(vp));
      break;
    case "nobackfaces"_hash:
      nobackfaces = true;
      break;
    case "end"_hash:
      exit(EXIT_SUCCESS);
      [[fallthrough]];
    case "reset"_hash:
      TM = identity_matrix;
    case ""_hash: // newlines fall into this case
    case "#"_hash:
      break;
    } // catches all cases no default!
  }
}

auto main(int argc, char* argv[]) -> int {
  std::ios_base::sync_with_stdio(false);
  in_file.open(((argc == 2) ? argv[1] : "../Debug/lab3c.in")); // Lab3A.in, simple.in...
  if (!in_file)
    return -1;
  std::string line;
  std::getline(in_file, line);
  std::stringstream ss{line};
  ss >> win_x >> win_y;
  system("pause");

  // GLUT stuff
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowPosition(1000, 100);
  glutInitWindowSize(win_x, win_y);
  glutCreateWindow("Lab 3 Window");
  glutDisplayFunc(displayFunc);
  glClearColor(0.0, 0.0, 0.0, 0.0); // set the bargckground
  glClear(GL_COLOR_BUFFER_BIT);     // clear the buffer
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, win_x, 0.0, win_y);
  glFlush();
  glutMainLoop();
}
