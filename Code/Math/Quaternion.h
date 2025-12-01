#ifndef CGR_COURSEWORK_MATH_QUATERNION_H
#define CGR_COURSEWORK_MATH_QUATERNION_H

#include "Vector.h"
#include <cmath>

// Quaternion for rotation interpolation
// Represents rotation as q = w + xi + yj + zk
struct Quaternion {
  double w, x, y, z;

  // Default constructor (identity rotation)
  Quaternion() : w(1.0), x(0.0), y(0.0), z(0.0) {}

  // Construct from components
  Quaternion(double w, double x, double y, double z) : w(w), x(x), y(y), z(z) {}

  // Construct from axis and angle (radians)
  static Quaternion from_axis_angle(const Vec3 &axis, double angle) {
    double half_angle = angle * 0.5;
    double s = std::sin(half_angle);
    Vec3 normalized_axis = axis.norm();
    return Quaternion(std::cos(half_angle), normalized_axis.x * s,
                      normalized_axis.y * s, normalized_axis.z * s);
  }

  // Construct from Euler angles (radians, ZYX order)
  static Quaternion from_euler(const Vec3 &euler) {
    // Apply rotations in order: Z * Y * X
    Quaternion qx = from_axis_angle(Vec3(1, 0, 0), euler.x);
    Quaternion qy = from_axis_angle(Vec3(0, 1, 0), euler.y);
    Quaternion qz = from_axis_angle(Vec3(0, 0, 1), euler.z);
    return qz * qy * qx;
  }

  // Quaternion multiplication
  Quaternion operator*(const Quaternion &q) const {
    return Quaternion(w * q.w - x * q.x - y * q.y - z * q.z,
                      w * q.x + x * q.w + y * q.z - z * q.y,
                      w * q.y - x * q.z + y * q.w + z * q.x,
                      w * q.z + x * q.y - y * q.x + z * q.w);
  }

  // Dot product
  double dot(const Quaternion &q) const {
    return w * q.w + x * q.x + y * q.y + z * q.z;
  }

  // Magnitude
  double length() const { return std::sqrt(w * w + x * x + y * y + z * z); }

  // Normalize
  Quaternion norm() const {
    double len = length();
    if (len < 1e-10)
      return Quaternion();
    return Quaternion(w / len, x / len, y / len, z / len);
  }

  // Conjugate
  Quaternion conjugate() const { return Quaternion(w, -x, -y, -z); }

  // Inverse (for unit quaternions, same as conjugate)
  Quaternion inverse() const {
    double len_sq = w * w + x * x + y * y + z * z;
    if (len_sq < 1e-10)
      return Quaternion();
    return Quaternion(w / len_sq, -x / len_sq, -y / len_sq, -z / len_sq);
  }

  // Spherical Linear Interpolation (SLERP)
  // Smoothly interpolates between two quaternions
  static Quaternion slerp(const Quaternion &q1, const Quaternion &q2,
                          double t) {
    Quaternion start = q1.norm();
    Quaternion end = q2.norm();

    // Compute dot product
    double dot = start.dot(end);

    // If dot is negative, negate one quaternion to take shorter path
    if (dot < 0.0) {
      end = Quaternion(-end.w, -end.x, -end.y, -end.z);
      dot = -dot;
    }

    // If quaternions are very close, use linear interpolation
    const double DOT_THRESHOLD = 0.9995;
    if (dot > DOT_THRESHOLD) {
      // Linear interpolation
      return Quaternion(start.w + (end.w - start.w) * t,
                        start.x + (end.x - start.x) * t,
                        start.y + (end.y - start.y) * t,
                        start.z + (end.z - start.z) * t)
          .norm();
    }

    // Clamp dot product to prevent numerical errors with acos
    if (dot > 1.0)
      dot = 1.0;
    if (dot < -1.0)
      dot = -1.0;

    // Calculate angle between quaternions
    double theta_0 = std::acos(dot);
    double theta = theta_0 * t;

    double sin_theta = std::sin(theta);
    double sin_theta_0 = std::sin(theta_0);

    double s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
    double s1 = sin_theta / sin_theta_0;

    return Quaternion(s0 * start.w + s1 * end.w, s0 * start.x + s1 * end.x,
                      s0 * start.y + s1 * end.y, s0 * start.z + s1 * end.z);
  }

  // Convert to 3x3 rotation matrix
  void to_matrix(double mat[3][3]) const {
    double xx = x * x, yy = y * y, zz = z * z;
    double xy = x * y, xz = x * z, yz = y * z;
    double wx = w * x, wy = w * y, wz = w * z;

    mat[0][0] = 1.0 - 2.0 * (yy + zz);
    mat[0][1] = 2.0 * (xy - wz);
    mat[0][2] = 2.0 * (xz + wy);

    mat[1][0] = 2.0 * (xy + wz);
    mat[1][1] = 1.0 - 2.0 * (xx + zz);
    mat[1][2] = 2.0 * (yz - wx);

    mat[2][0] = 2.0 * (xz - wy);
    mat[2][1] = 2.0 * (yz + wx);
    mat[2][2] = 1.0 - 2.0 * (xx + yy);
  }
};

#endif // CGR_COURSEWORK_MATH_QUATERNION_H
