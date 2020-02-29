#include "DrawKit.hpp"
#include "Object.hpp"
#include "Observer.hpp"

std::ifstream in_file;
int win_x, win_y;

auto process_background(std::stringstream& ss) {
  double Br, Bg, Bb;
  ss >> Br >> Bg >> Bb;
  return Background{Br, Bg, Bb};
}

auto process_ambient(std::stringstream& ss) {
  double KaIar, KaIag, KaIab;
  ss >> KaIar >> KaIag >> KaIab;
  return Ambient{KaIar, KaIag, KaIab};
}

auto process_light(std::stringstream& ss, std::vector<Light>& lights) {
  size_t index;
  double Ipr, Ipg, Ipb, Ix, Iy, Iz;
  ss >> index >> Ipr >> Ipg >> Ipb >> Ix >> Iy >> Iz;
  if (index > lights.size())
    lights.push_back(Light{Ipr, Ipg, Ipb, Ix, Iy, Iz});
  else
    lights[index - 1] = Light{Ipr, Ipg, Ipb, Ix, Iy, Iz};
}

auto process_scale(std::stringstream& ss) {
  double x, y, z;
  ss >> x >> y >> z;
  return scaling_m(x, y, z);
}

auto process_rotate(std::stringstream& ss) {
  double x, y, z;
  ss >> x >> y >> z;
  if (x)
    return rotation_m(x, 'x');
  else if (y)
    return rotation_m(y, 'y');
  else
    return rotation_m(z);
}

auto process_translate(std::stringstream& ss) {
  double x, y, z;
  ss >> x >> y >> z;
  return translation_m(x, y, z);
}

auto process_viewport(std::stringstream& ss) {
  double vxl, vxr, vyb, vyt;
  ss >> vxl >> vxr >> vyb >> vyt;
  return Viewport{(vxr - vxl) / (vyt - vyb), vxl, vxr, vyb, vyt, win_x, win_y};
}

auto process_object(std::stringstream& ss, const Matrix<4>& TM) {
  // open asc file
  std::string asc_path;
  double Or, Og, Ob, Kd, Ks;
  int N;
  ss >> asc_path >> Or >> Og >> Ob >> Kd >> Ks >> N;

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

  Object asc_obj{v, f, Or, Og, Ob, Kd, Ks, N};
  asc_obj.set_vertex(asc_ss, asc_file, TM);
  asc_obj.set_face(asc_ss, asc_file);

  return asc_obj;
}

auto process_observer(std::stringstream& ss) {
  double Ex, Ey, Ez, COIx, COIy, COIz, Tilt, Hither, Yon, Hav;
  ss >> Ex >> Ey >> Ez >> COIx >> COIy >> COIz >> Tilt >> Hither >> Yon >> Hav;
  return Observer{Ex, Ey, Ez, COIx, COIy, COIz, Tilt, Hither, Yon, Hav};
}

auto process_display(const Viewport& vp, const std::vector<Object>& objects, const Observer& ob_ov,
                     const Background& bg, const Ambient& ambient, const std::vector<Light>& lights) {

  auto t0 = std::chrono::high_resolution_clock::now();

  clear_screen(0.0f, 0.0f, 0.0f);

  const Polygons_au ps_illuminated = [&]() {
    Polygons_au ret, a;
    for (const auto& obj : objects) {
      a = flat_shading(obj, ob_ov, ambient, lights);
      ret.insert(ret.end(), a.begin(), a.end());
    }
    return ret;
  }();

  std::unique_ptr<Cbuffer> cbuf{new Cbuffer};
  for (auto& arr : (*cbuf))
    arr.fill(Color{bg.Br, bg.Bg, bg.Bb});

  std::unique_ptr<Zbuffer> zbuf{new Zbuffer};
  for (auto& arr : (*zbuf))
    arr.fill(std::numeric_limits<double>::max());

  const auto [vxl, vxr, vyb, vyt] = vp.get_borders();
  z_buffer_algorithm(translation_m(vxl, vyb) *
                         scaling_m((vxr - vxl) / 2.0, (vyt - vyb) / 2.0) *
                         translation_m(1.0, 1.0) *
                         to_screenspace(ps_illuminated, ob_ov.get_pmXem(vp.AR)),
                     *zbuf, *cbuf);

  draw(*cbuf, vp);

  auto t1 = std::chrono::high_resolution_clock::now();
  std::cout << "display takes: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << "ms\n";
  system("pause");
}

// some hash function found on the Internet. I need a constexpr hash function.
constexpr size_t fnv1a_32(char const* s, std::size_t count) {
#pragma warning(disable : 4307)
  return ((count ? fnv1a_32(s, count - 1) : 2166136261U) ^ s[count]) * 16777619U;
}
constexpr size_t operator""_hash(char const* s, const size_t count) { return fnv1a_32(s, count); }

auto displayFunc() {
  std::stringstream ss;
  Matrix<4> TM{identity_matrix};
  Viewport vp;
  Observer ob_ov;
  std::vector<Object> objects;
  Background background;
  Ambient ambient;
  std::vector<Light> lights;

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
      process_display(vp, objects, ob_ov, background, ambient, lights);
      break;
    case "ambient"_hash:
      ambient = process_ambient(ss);
      break;
    case "background"_hash:
      background = process_background(ss);
      break;
    case "light"_hash:
      process_light(ss, lights);
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
  in_file.open(((argc == 2) ? argv[1] : "../Debug/lab4B.in")); // Lab3A.in, simple.in...
  if (!in_file)
    return -1;
  std::string line;
  std::getline(in_file, line);
  std::stringstream ss{line};
  ss >> win_x >> win_y;
  system("pause");
  std::cout << "display takes about 2 seconds in DEBUG MODE, \n       less than 30 milliseconds in RELEASE MODE.\nPatience is a virtue!\n\n";

  // GLUT stuff
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowPosition(100, 100);
  glutInitWindowSize(win_x, win_y);
  glutCreateWindow("Lab 4 Window");
  glutDisplayFunc(displayFunc);
  glClearColor(0.0, 0.0, 0.0, 0.0); // set the bargckground
  glClear(GL_COLOR_BUFFER_BIT);     // clear the buffer
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, win_x, 0.0, win_y);
  glFlush();
  glutMainLoop();
}
