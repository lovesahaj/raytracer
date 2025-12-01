#include "Intersections.h"

#include <cmath>

// Helper for floating-point comparison - avoids precision issues
bool areSame(double a, double b) {
  return std::fabs(a - b) <= std::numeric_limits<double>::epsilon();
}
