#pragma once
#include <tuple>

struct Viewport {
  double AR, vxl, vxr, vyb, vyt;
  int win_x, win_y;

  [[nodiscard]] auto get_borders() const {
    auto bx = [&](const auto& v) { return (1 + v) * win_x / 2; };
    auto by = [&](const auto& v) { return (1 + v) * win_y / 2; };
    return std::tuple{bx(vxl), bx(vxr), by(vyb), by(vyt)};
  }
};
