#ifndef CGR_COURSEWORK_GEOMETRY_PLANE_H
#define CGR_COURSEWORK_GEOMETRY_PLANE_H

#include "../Math/Vector.h"
#include "Shape.h"
#include <ostream>
#include <vector>

struct Plane : Shape {
  std::vector<Point> points;
};

inline std::ostream &operator<<(std::ostream &os, const Plane &plane) {
  os << "Plane '" << plane.name << "': points=" << plane.points.size();
  return os;
}

#endif // CGR_COURSEWORK_GEOMETRY_PLANE_H
