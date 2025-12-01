#ifndef CGR_COURSEWORK_MATH_RANDOM_H
#define CGR_COURSEWORK_MATH_RANDOM_H

#include <random>

inline double random_double() {
  static thread_local std::mt19937 generator(std::random_device{}());
  std::uniform_real_distribution<double> distribution(0.0, 1.0);
  return distribution(generator);
}

inline double random_double(double min, double max) {
  return min + (max - min) * random_double();
}

#endif // CGR_COURSEWORK_MATH_RANDOM_H
