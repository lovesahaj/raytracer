#ifndef CGR_COURSEWORK_GEOMETRY_CONE_H
#define CGR_COURSEWORK_GEOMETRY_CONE_H

#include "../Math/Vector.h"
#include "Shape.h"
#include <ostream>

struct Cone : Shape {
  Point location;
  Point rotation;
  Vec3 scale{1.0, 1.0, 1.0}; // NEW: Non-uniform scale
  double radius{};
  double depth{};
};

inline std::ostream &operator<<(std::ostream &os, const Cone &cone) {
  os << "Cone '" << cone.name << "': location=" << cone.location
     << ", radius=" << cone.radius << ", depth=" << cone.depth;
  return os;
}

#endif // CGR_COURSEWORK_GEOMETRY_CONE_H
