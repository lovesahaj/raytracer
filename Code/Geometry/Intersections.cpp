#include "Intersections.h"

#include <cmath>

bool areSame(double a, double b) {
  return std::fabs(a - b) <= std::numeric_limits<double>::epsilon();
}
