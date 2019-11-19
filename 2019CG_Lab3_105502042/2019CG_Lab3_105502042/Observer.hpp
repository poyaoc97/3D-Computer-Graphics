#pragma once
#include "MatrixKit.hpp"
#include "Viewport.hpp"

struct Observer {
  double Ex, Ey, Ez, COIx, COIy, COIz, Tilt, Hither, Yon, Hav;

  auto get_pmXem(const Viewport& vp) const -> Matrix<4> {
    const Vector<4> top_vector{{0, 1, 0, 0}};
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
    auto tan_theta = std::tan(Hav * piDiv180);
    auto a33 = Yon / (Yon - Hither) * tan_theta;
    Matrix<4> PM = {{{1, 0, 0, 0},
                     {0, vp.AR, 0, 0},
                     {0, 0, a33, -Hither * a33},
                     {0, 0, tan_theta, 0}}};

    return PM *
           rotation_m<4>(-Tilt) *
           mirror *
           GRM *
           translation_m<4>(-Ex, -Ey, -Ez);
  }
};
