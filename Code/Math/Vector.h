#ifndef CGR_COURSEWORK_MATH_VECTOR_H
#define CGR_COURSEWORK_MATH_VECTOR_H

#include <cmath>
#include <cstddef>
#include <ostream>

// Generic vector template for any dimension
template <size_t N> struct Vec {
  double data[N];

  Vec() {
    for (size_t i = 0; i < N; ++i) {
      data[i] = 0;
    }
  }

  // Access operators
  double operator[](const size_t i) const { return data[i]; }
  double &operator[](const size_t i) { return data[i]; }

  // Generic operations
  Vec<N> &operator+=(const Vec<N> &other) {
    for (size_t i = 0; i < N; ++i) {
      data[i] += other.data[i];
    }
    return *this;
  }

  Vec<N> &operator-=(const Vec<N> &other) {
    for (size_t i = 0; i < N; ++i) {
      data[i] -= other.data[i];
    }
    return *this;
  }

  Vec<N> &operator*=(const double c) {
    for (size_t i = 0; i < N; ++i) {
      data[i] *= c;
    }
    return *this;
  }

  Vec<N> &operator/=(const double c) {
    for (size_t i = 0; i < N; ++i) {
      data[i] /= c;
    }
    return *this;
  }

  Vec<N> operator+(const Vec<N> &other) const {
    Vec<N> result = *this;
    result += other;
    return result;
  }

  Vec<N> operator-(const Vec<N> &other) const {
    Vec<N> result = *this;
    result -= other;
    return result;
  }

  Vec<N> operator*(const double c) const {
    Vec<N> result = *this;
    result *= c;
    return result;
  }

  Vec<N> operator/(const double c) const {
    Vec<N> result = *this;
    result /= c;
    return result;
  }

  // Component-wise multiplication
  Vec<N> operator*(const Vec<N> &other) const {
    Vec<N> result;
    for (size_t i = 0; i < N; ++i) {
      result.data[i] = data[i] * other.data[i];
    }
    return result;
  }

  Vec<N> operator-() const {
    Vec<N> result;
    for (size_t i = 0; i < N; ++i) {
      result.data[i] = -data[i];
    }
    return result;
  }

  [[nodiscard]] double dot(const Vec<N> &other) const {
    double sum = 0;
    for (size_t i = 0; i < N; ++i) {
      sum += data[i] * other.data[i];
    }
    return sum;
  }

  [[nodiscard]] double length_squared() const { return dot(*this); }

  [[nodiscard]] double length() const { return std::sqrt(length_squared()); }

  [[nodiscard]] Vec<N> normalized() const { return *this / length(); }

  // Left-side scalar multiplication
  friend Vec<N> operator*(const double c, const Vec<N> &v) { return v * c; }
};

// Specialization for 3D vectors with named accessors
template <> struct Vec<3> {
  union {
    double data[3];
    struct {
      double x, y, z;
    };
  };

  Vec() : x(0), y(0), z(0) {}
  Vec(double x, double y, double z) : x(x), y(y), z(z) {}

  // Named accessors for color
  [[nodiscard]] double r() const { return x; }
  [[nodiscard]] double g() const { return y; }
  [[nodiscard]] double b() const { return z; }

  double &r() { return x; }
  double &g() { return y; }
  double &b() { return z; }

  // Array access (for generic compatibility)
  double operator[](const size_t i) const { return data[i]; }
  double &operator[](const size_t i) { return data[i]; }

  // Operators
  Vec<3> &operator+=(const Vec<3> &other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  Vec<3> &operator-=(const Vec<3> &other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  Vec<3> &operator*=(const double c) {
    x *= c;
    y *= c;
    z *= c;
    return *this;
  }

  Vec<3> &operator/=(const double c) {
    x /= c;
    y /= c;
    z /= c;
    return *this;
  }

  Vec<3> operator+(const Vec<3> &other) const {
    return Vec<3>{x + other.x, y + other.y, z + other.z};
  }

  Vec<3> operator-(const Vec<3> &other) const {
    return Vec<3>{x - other.x, y - other.y, z - other.z};
  }

  Vec<3> operator*(const double c) const { return Vec<3>{x * c, y * c, z * c}; }

  Vec<3> operator/(const double c) const { return Vec<3>{x / c, y / c, z / c}; }

  // Component-wise multiplication
  Vec<3> operator*(const Vec<3> &other) const {
    return Vec<3>{x * other.x, y * other.y, z * other.z};
  }

  Vec<3> operator-() const { return Vec<3>{-x, -y, -z}; }

  [[nodiscard]] double dot(const Vec<3> &other) const {
    return x * other.x + y * other.y + z * other.z;
  }

  [[nodiscard]] Vec<3> cross(const Vec<3> &other) const {
    return Vec<3>{y * other.z - z * other.y, z * other.x - x * other.z,
                  x * other.y - y * other.x};
  }

  [[nodiscard]] double length_squared() const { return x * x + y * y + z * z; }

  [[nodiscard]] double length() const { return std::sqrt(length_squared()); }

  [[nodiscard]] Vec<3> norm() const { return *this / length(); }

  friend Vec<3> operator*(const double c, const Vec<3> &v) { return v * c; }

  friend std::ostream &operator<<(std::ostream &os, const Vec<3> &v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
  }
};

// Convenient typedefs
using Vec3 = Vec<3>;

using Point = Vec3;
using Direction = Vec3;
using Color = Vec3;
using Pixel = Vec3;

#endif // CGR_COURSEWORK_MATH_VECTOR_H
