#include "helper.hpp"

std::ifstream in_file;

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
