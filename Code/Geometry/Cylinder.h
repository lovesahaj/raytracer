#ifndef CGR_COURSEWORK_GEOMETRY_CYLINDER_H
#define CGR_COURSEWORK_GEOMETRY_CYLINDER_H

#include "../Math/Vector.h"
#include "Shape.h"
#include <ostream>

struct Cylinder : Shape {
  Point location;
  Point rotation;
  Vec3 scale{1.0, 1.0, 1.0}; // NEW: Non-uniform scale
  double radius{};
  double depth{};
};

inline std::ostream &operator<<(std::ostream &os, const Cylinder &cylinder) {
  os << "Cylinder '" << cylinder.name << "': location=" << cylinder.location
     << ", radius=" << cylinder.radius << ", depth=" << cylinder.depth;
  return os;
}

#endif // CGR_COURSEWORK_GEOMETRY_CYLINDER_H
