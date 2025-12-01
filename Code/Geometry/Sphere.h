#ifndef CGR_COURSEWORK_GEOMETRY_SPHERE_H
#define CGR_COURSEWORK_GEOMETRY_SPHERE_H

#include "../Math/Vector.h"
#include "Shape.h"
#include <ostream>

struct Sphere : Shape {
  Point location;                // Translation/position
  Vec3 scale{1.0, 1.0, 1.0};     // Non-uniform scale (creates ellipsoids)
  Point rotation{0.0, 0.0, 0.0}; // Euler angles in radians
};

inline std::ostream &operator<<(std::ostream &os, const Sphere &sphere) {
  os << "Sphere '" << sphere.name << "': location=" << sphere.location
     << ", scale=" << sphere.scale << ", rotation=" << sphere.rotation;
  return os;
}

#endif // CGR_COURSEWORK_GEOMETRY_SPHERE_H
