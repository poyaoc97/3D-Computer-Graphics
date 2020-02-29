#include "my_utils.hpp"
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>

constexpr int window_width{800};
constexpr int window_height{600};
constexpr double pi{3.141'592'653'589'793'238'46};
constexpr double pi_divided_by_180{pi / 180};
constexpr Matrix<3> identity_matrix{{{1, 0, 0},
                                     {0, 1, 0},
                                     {0, 0, 1}}};
std::string file_path;

// polygon clipping
template<typename Predicate1, typename Predicate2, typename Predicate3>
auto clip_polygons(const Polygons<3>& polygons, Polygons<3>& clipped_polys, const double boundary, const std::size_t comp,
                   const Predicate1& in2in, const Predicate2& in2out, const Predicate3& out2in) {
  Vertices<3> clipped_vs;
  for (const auto& vs : polygons) {
    clipped_vs.clear();
    for (std::size_t i = 0, size = vs.size(); i < size; ++i) {
      const auto& s = vs[i];
      const auto& p = vs[(i + 1) % size];
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
    clipped_polys.push_back(std::move(clipped_vs));
  }
}

inline auto draw(const double wxl, const double wxr, const double wyb, const double wyt,
                 const double vxl, const double vxr, const double vyb, const double vyt,
                 const Polygons<3>& polygons) {
  // draw borders
  glColor3f(1.0, 1.0, 1.0);
  draw_polygon<2>(Vertices<2>{{{vxl, vyb}, {vxr, vyb}, {vxr, vyt}, {vxl, vyt}}});

  // clipping
  Polygons<3> a;
  Polygons<3> clipped_polys;
  clip_polygons(
      polygons, clipped_polys, wxr, 0,
      [&](const auto s, const auto p) { return s <= wxr && p <= wxr; },
      [&](const auto s, const auto p) { return s <= wxr && p > wxr; },
      [&](const auto s, const auto p) { return s > wxr && p <= wxr; });
  a = std::move(clipped_polys);
  clipped_polys.clear();

  clip_polygons(
      a, clipped_polys, wyt, 1,
      [&](const auto s, const auto p) { return s <= wyt && p <= wyt; },
      [&](const auto s, const auto p) { return s <= wyt && p > wyt; },
      [&](const auto s, const auto p) { return s > wyt && p <= wyt; });
  a = std::move(clipped_polys);
  clipped_polys.clear();

  clip_polygons(
      a, clipped_polys, wxl, 0,
      [&](const auto s, const auto p) { return s >= wxl && p >= wxl; },
      [&](const auto s, const auto p) { return s >= wxl && p < wxl; },
      [&](const auto s, const auto p) { return s < wxl && p >= wxl; });
  a = std::move(clipped_polys);
  clipped_polys.clear();

  clip_polygons(
      a, clipped_polys, wyb, 1,
      [&](const auto s, const auto p) { return s >= wyb && p >= wyb; },
      [&](const auto s, const auto p) { return s >= wyb && p < wyb; },
      [&](const auto s, const auto p) { return s < wyb && p >= wyb; });

  // map to window space and draw
  glColor3f(1.0, 1.0, 0.0);
  draw_polygons(transformed_ps(
      Matrix<3>{t_m<3>(vxl, vyb) *
                s_m<3>((vxr - vxl) / (wxr - wxl), (vyt - vyb) / (wyt - wyb)) *
                t_m<3>(-wxl, -wyb)},
      clipped_polys));
  glFlush();
}

// simple hash function
constexpr auto operator""_hash(const char* s, const std::size_t count) -> std::size_t {
  return *s ^ count;
}

auto displayFunc() {
  std::ifstream infile{file_path};
  std::stringstream ss;
  std::string line, str;
  Matrix<3> t{identity_matrix};
  double sx, sy;
  double angle;
  double dx, dy;
  double wxl, wxr, wyb, wyt, vxl, vxr, vyb, vyt;
  const Vertices<3> square_vs{{{1, 1, 1}, {-1, 1, 1}, {-1, -1, 1}, {1, -1, 1}}};
  const Vertices<3> triangle_vs{{{0, 1, 1}, {-1, -1, 1}, {1, -1, 1}}};
  Polygons<3> polygons; // records all drawing activities

  std::ios_base::sync_with_stdio(false);

  while (std::getline(infile, line)) {
    ss << line;
    ss >> str;
    switch (operator""_hash(str.c_str(), str.size())) {
    case "scale"_hash: // apply scaling
      ss >> sx >> sy;
      t = s_m<3>(sx, sy) * t;
      break;
    case "rotate"_hash: // apply rotation
      ss >> angle;
      t = r_m<3>(angle * pi_divided_by_180) * t;
      break;
    case "translate"_hash: // apply translation
      ss >> dx >> dy;
      t = t_m<3>(dx, dy) * t;
      break;
    case "square"_hash: // draw a square
      polygons.push_back(std::move(transformed_vs(t, square_vs)));
      break;
    case "triangle"_hash: // draw a triangle
      polygons.push_back(std::move(transformed_vs(t, triangle_vs)));
      break;
    case "view"_hash: // create a view (map to the screen)
      ss >> wxl >> wxr >> wyb >> wyt >> vxl >> vxr >> vyb >> vyt;
      {
        auto t0 = std::chrono::high_resolution_clock::now();
        draw(wxl, wxr, wyb, wyt, vxl, vxr, vyb, vyt, polygons);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::cout << "draw() takes: " << std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() << "us\n";
      }
      system("pause");
      break;
    case "clearData"_hash: // clear all recorded data, zero out all stats
      polygons.clear();
      break;
    case "clearScreen"_hash: // clear glut window
      clear_screen();
      break;
    case "end"_hash: // terminate the process
      exit(EXIT_SUCCESS);
    case "reset"_hash: // set transformation matrix to identity
      t = identity_matrix;
      [[fallthrough]];
    case ""_hash:
    case "#"_hash:
      break;
    default:
      exit(EXIT_FAILURE);
    }
    while (ss >> str)
      ;
    str.clear();
    ss.clear();
  }
}

auto main(int argc, char* argv[]) -> int {
  system("pause");
  file_path = ((argc == 2) ? argv[1] : "lab2E.in");

  // init GLUT and create Window
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowPosition(1'000, 100);
  glutInitWindowSize(window_width, window_height);
  glutCreateWindow("Lab 2 Window");
  // displayFunc is called whenever there is a need to redisplay the
  // window, e.g. when the window is exposed from under another window or
  // when the window is de-iconified
  glutDisplayFunc(displayFunc);
  // set bargckground color
  glClearColor(0.0, 0.0, 0.0, 0.0); // set the bargckground
  glClear(GL_COLOR_BUFFER_BIT);     // clear the buffer
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, window_width, 0.0, window_height);
  glFlush();
  // enter GLUT event processing cycle
  glutMainLoop();
}
