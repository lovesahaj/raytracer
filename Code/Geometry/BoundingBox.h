#ifndef CGR_COURSEWORK_GEOMETRY_BOUNDING_BOX_H
#define CGR_COURSEWORK_GEOMETRY_BOUNDING_BOX_H

#include <algorithm>

#include "../Core/Ray.h"
#include "../Math/Vector.h"

// Simple axis-aligned bounding box for spatial queries
struct BoundingBox {
  Point min;
  Point max;

  bool intersect(Ray ray, double t_min, double t_max) {
    // same method as the cube just without the transformations
    for (int axis = 0; axis < 3; axis++) {
      double inv_d = 1.0 / ray.direction[axis];

      double t0 = (min[axis] - ray.origin[axis]) * inv_d;
      double t1 = (max[axis] - ray.origin[axis]) * inv_d;

      if (inv_d < 0.0) std::swap(t0, t1);

      t_min = std::max(t0, t_min);
      t_max = std::min(t1, t_max);

      if (t_max <= t_min) return false;
    }

    return true;
  }
};

#endif  // CGR_COURSEWORK_GEOMETRY_BOUNDING_BOX_H
