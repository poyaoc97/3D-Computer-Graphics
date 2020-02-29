#pragma once
#include "Lighting.hpp"
#include "MatrixKit.hpp"
#include "Object.hpp"
#include "Observer.hpp"
#include <GL/glut.h>

struct Viewport {
  double AR, vxl, vxr, vyb, vyt;
  int win_x, win_y;

  [[nodiscard]] auto get_borders() const {
    const auto b = [&](const auto& v, const auto win) { return static_cast<int>(std::round((1 + v) * win / 2)); };
    return std::tuple{b(vxl, win_x), b(vxr, win_x), b(vyb, win_y), b(vyt, win_y)};
  }
};

inline auto clear_screen(GLclampf r, GLclampf g, GLclampf b, bool flush = false) {
  glClearColor(r, g, b, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  if (flush)
    glFlush();
}

inline auto flat_shading(const Object& obj, const Observer& ob_ov, const Ambient& ambient, const std::vector<Light>& lights) {
  const auto [Or, Og, Ob, Kd, Ks, N] = obj.get_lighting_info();
  const auto eye = ob_ov.get_eye_pos();
  auto ps_au = obj.to_polygons();

  for (auto& poly : ps_au) {
    const Vector<3> normal{normalize_3D(get_normal(poly.polygon, false))};
    double Ir{ambient.KaIar * Or}, Ig{ambient.KaIag * Og}, Ib{ambient.KaIab * Ob};

    for (const auto& light : lights) {
      const auto [NL, HNn] = [&]() {
        auto L = light.pos() - poly.polygon[0];
        auto V = eye - poly.polygon[0];
        auto NL = dot_3D(normal, normalize_3D(L));
        auto H = normalize_3D(L + V);
        auto HNn = std::pow(dot_3D(H, normal), N);
        return NL <= 0.0 ? std::pair{0.0, 0.0} : std::pair{NL, HNn};
      }();

      Ir += Kd * light.Ipr * NL * Or + Ks * light.Ipr * HNn;
      Ig += Kd * light.Ipg * NL * Og + Ks * light.Ipg * HNn;
      Ib += Kd * light.Ipb * NL * Ob + Ks * light.Ipb * HNn;
    }
    poly.color = Color{Ir, Ig, Ib};
  }
  return ps_au;
}

constexpr auto clip_one_case = [](const Polygon_u<4>& polygon, const auto& c) {
  Polygon_u<4> relay;
  const auto sz = polygon.size();
  for (size_t i = 0; i < sz; ++i) {
    const auto& s = polygon[i];
    const auto& p = polygon[(i + 1) % sz];
    if (const double c1{c(s)}, c2{c(p)}; (c1 >= 0 && c2 >= 0)) { // in2in
      relay.push_back(p);
    } else if (const auto new_s = (s + c1 / (c1 - c2) * (p - s)); (c1 >= 0 && c2 < 0)) { // in2out
      relay.push_back(new_s);
    } else if (c1 < 0 && c2 >= 0) { // out2in
      relay.push_back(new_s);
      relay.push_back(p);
    }
  }
  return relay;
};

inline auto to_screenspace(const Polygons_au& polygons, const Matrix<4>& pmXem) {
  using Codes = std::array<const std::function<double(const Vector<4>&)>, 6>;
  const Codes codes{{[](const Vector<4>& v) { return v[3] - v[0]; }, [](const Vector<4>& v) { return v[3] + v[0]; }, // w - x, w + x
                     [](const Vector<4>& v) { return v[3] - v[1]; }, [](const Vector<4>& v) { return v[3] + v[1]; }, // w - y, w + y
                     [](const Vector<4>& v) { return v[3] - v[2]; }, [](const Vector<4>& v) { return v[2]; }}};      // w - z, w
  Polygons_au clipped_polys;
  std::mutex lock_clipped_polys;

  std::for_each(std::execution::par_unseq, polygons.begin(), polygons.end(), [&](Polygon_au polygon_au) {
    polygon_au.polygon = pmXem * polygon_au.polygon; // project a polygon to projection space

    for (const auto& code : codes) // clip against all six planes
      polygon_au.polygon = clip_one_case(polygon_au.polygon, code);

    if (polygon_au.polygon.size()) {
      for (auto& a : polygon_au.polygon) // perspective division
        a = Vector<4>{{a[0] / a[3], a[1] / a[3], a[2] / a[3], 1.0}};

      std::scoped_lock l{lock_clipped_polys};
      clipped_polys.push_back(std::move(polygon_au));
    }
  });
  return clipped_polys;
}

using Zbuffer = std::array<std::array<double, 500>, 500>;
using Cbuffer = std::array<std::array<Color, 500>, 500>;

inline void z_buffer_algorithm(const Polygons_au& ps, Zbuffer& zbuf, Cbuffer& cbuf) {
  auto get_min_max = [](Polygon_u<4> poly, size_t index) {
    auto [min, max] = std::minmax_element(begin(poly), end(poly), [=](auto a, auto b) { return a[index] < b[index]; });
    return std::tuple{static_cast<int>(std::trunc((*min)[index])), static_cast<int>(std::ceil((*max)[index]))};
  };

  auto is_in_poly = [](int x, int y, const Polygon_u<4>& poly, auto normal) {
    for (size_t i = 0, sz = poly.size(); i != sz; ++i)
      if (cross_Z((poly[(i + 1) % sz] - poly[i]), (Vector<4>{double(x), double(y), 0.0, 0.0} - poly[i])) * normal[2] < 0.0)
        return false;
    return true;
  };

  for (const auto& p : ps) {
    const Vector<3> normal{normalize_3D(get_normal(p.polygon))};
    const auto [A, B, C] = normal;
    const double D = -(dot_3D(Vector<3>{A, B, C}, Vector<3>{p.polygon[0][0], p.polygon[0][1], p.polygon[0][2]}));

    for (auto [y, y_max] = get_min_max(p.polygon, 1); y != y_max; ++y) {
      auto [x, x_max] = get_min_max(p.polygon, 0);
      for (double z = -(A * x + B * y + D) / C; x != x_max; ++x, z -= A / C) {
        if (z < zbuf[y][x] && is_in_poly(x, y, p.polygon, normal)) {
          zbuf[y][x] = z;
          cbuf[y][x] = p.color;
        }
      }
    }
  }
}

inline void draw(const Cbuffer& cbuf, const Viewport& vp) {
  const auto [vxl, vxr, vyb, vyt] = vp.get_borders();

  glBegin(GL_POINTS);
  for (auto y = vyb; y < vyt; ++y) {
    for (auto x = vxl; x < vxr; ++x) {
      glColor3d(cbuf[y][x].r, cbuf[y][x].g, cbuf[y][x].b);
      glVertex2i(x, y);
    }
  }
  glEnd();
  glFlush();
}
