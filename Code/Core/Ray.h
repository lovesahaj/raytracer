#ifndef CGR_COURSEWORK_CORE_RAY_H
#define CGR_COURSEWORK_CORE_RAY_H

#include "../Math/Vector.h"

struct Ray {
  Point origin;
  Direction direction;
  double time;  // Time parameter for motion blur (0.0 to 1.0)

  // Default constructor
  Ray() : origin(), direction(), time(0.5) {}

  // Constructor with time (default to middle of shutter time)
  Ray(const Point& o, const Direction& d, double t = 0.5)
    : origin(o), direction(d), time(t) {}
};

#endif // CGR_COURSEWORK_CORE_RAY_H
