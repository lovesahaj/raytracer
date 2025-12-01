#ifndef CGR_COURSEWORK_GEOMETRY_TORUS_H
#define CGR_COURSEWORK_GEOMETRY_TORUS_H

#include <ostream>

#include "../Math/Vector.h"
#include "Shape.h"

struct Torus : Shape {
  Point location;
  Point rotation;
  Vec3 scale{1.0, 1.0, 1.0};  // NEW: Non-uniform scale
  double major_radius{};
  double minor_radius{};
};

inline std::ostream& operator<<(std::ostream& os, const Torus& torus) {
  os << "Torus '" << torus.name << "': location=" << torus.location
     << ", major_radius=" << torus.major_radius << ", minor_radius=" << torus.minor_radius;
  return os;
}

#endif  // CGR_COURSEWORK_GEOMETRY_TORUS_H
