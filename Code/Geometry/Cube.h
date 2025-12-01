#ifndef CGR_COURSEWORK_GEOMETRY_CUBE_H
#define CGR_COURSEWORK_GEOMETRY_CUBE_H

#include "../Math/Vector.h"
#include "Shape.h"
#include <ostream>

struct Cube : Shape {
  Point translation;
  Point rotation;            // Euler angles in radians
  Vec3 scale{1.0, 1.0, 1.0}; // Non-uniform scale
};

inline std::ostream &operator<<(std::ostream &os, const Cube &cube) {
  os << "Cube '" << cube.name << "': translation=" << cube.translation
     << ", rotation=" << cube.rotation << ", scale=" << cube.scale;
  return os;
}

#endif // CGR_COURSEWORK_GEOMETRY_CUBE_H
