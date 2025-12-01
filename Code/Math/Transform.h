#ifndef CGR_COURSEWORK_MATH_TRANSFORM_H
#define CGR_COURSEWORK_MATH_TRANSFORM_H

#include <cmath>

#include "../Core/Ray.h"
#include "../Geometry/BoundingBox.h"
#include "Vector.h"

// 4x4 transformation matrix for homogeneous coordinates
// Methods are kept inline for performance (small, frequently called functions)
struct Mat4 {
  double m[4][4];

  Mat4() {
    // Identity matrix by default
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        m[i][j] = (i == j) ? 1.0 : 0.0;
      }
    }
  }

  // Matrix multiplication
  Mat4 operator*(const Mat4 &other) const {
    Mat4 result;
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        result.m[i][j] = 0;
        for (int k = 0; k < 4; k++) {
          result.m[i][j] += m[i][k] * other.m[k][j];
        }
      }
    }
    return result;
  }

  // Transform a point (w=1)
  Point transform_point(const Point &p) const {
    double x = m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z + m[0][3];
    double y = m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z + m[1][3];
    double z = m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z + m[2][3];
    double w = m[3][0] * p.x + m[3][1] * p.y + m[3][2] * p.z + m[3][3];

    // Divide by w for perspective (homogeneous coordinates)
    if (w != 1.0 && w != 0.0) {
      return Point(x / w, y / w, z / w);
    }
    return Point(x, y, z);
  }

  // Transform a direction/vector (w=0)
  Direction transform_direction(const Direction &d) const {
    double x = m[0][0] * d.x + m[0][1] * d.y + m[0][2] * d.z;
    double y = m[1][0] * d.x + m[1][1] * d.y + m[1][2] * d.z;
    double z = m[2][0] * d.x + m[2][1] * d.y + m[2][2] * d.z;
    return Direction(x, y, z);
  }

  // Transform a normal (needs transpose of inverse)
  Direction transform_normal(const Direction &n) const {
    // For normals, we need (M^-1)^T, which is equivalent to
    // treating the normal as a row vector and multiplying from the left
    double x = m[0][0] * n.x + m[1][0] * n.y + m[2][0] * n.z;
    double y = m[0][1] * n.x + m[1][1] * n.y + m[2][1] * n.z;
    double z = m[0][2] * n.x + m[1][2] * n.y + m[2][2] * n.z;
    return Direction(x, y, z).norm();
  }

  // Compute inverse matrix (using Gaussian elimination)
  Mat4 inverse() const;

  // Create translation matrix
  static Mat4 translate(const Vec3 &t) {
    Mat4 result;
    result.m[0][3] = t.x;
    result.m[1][3] = t.y;
    result.m[2][3] = t.z;
    return result;
  }

  // Create scale matrix
  static Mat4 scale(double sx, double sy, double sz) {
    Mat4 result;
    result.m[0][0] = sx;
    result.m[1][1] = sy;
    result.m[2][2] = sz;
    return result;
  }

  static Mat4 scale_uniform(double s) { return scale(s, s, s); }

  // Create rotation matrix around X axis
  static Mat4 rotate_x(double angle_radians) {
    Mat4 result;
    double c = std::cos(angle_radians);
    double s = std::sin(angle_radians);
    result.m[1][1] = c;
    result.m[1][2] = -s;
    result.m[2][1] = s;
    result.m[2][2] = c;
    return result;
  }

  // Create rotation matrix around Y axis
  static Mat4 rotate_y(double angle_radians) {
    Mat4 result;
    double c = std::cos(angle_radians);
    double s = std::sin(angle_radians);
    result.m[0][0] = c;
    result.m[0][2] = s;
    result.m[2][0] = -s;
    result.m[2][2] = c;
    return result;
  }

  // Create rotation matrix around Z axis
  static Mat4 rotate_z(double angle_radians) {
    Mat4 result;
    double c = std::cos(angle_radians);
    double s = std::sin(angle_radians);
    result.m[0][0] = c;
    result.m[0][1] = -s;
    result.m[1][0] = s;
    result.m[1][1] = c;
    return result;
  }

  // Create rotation matrix from Euler angles (in radians)
  // Rotation order: Z * Y * X (typical Blender order)
  static Mat4 rotate_euler(const Vec3 &euler_radians) {
    return rotate_z(euler_radians.z) * rotate_y(euler_radians.y) *
           rotate_x(euler_radians.x);
  }

  // Decompose matrix into Translation, Rotation, Scale (TRS)
  Vec3 extract_translation() const;
  Vec3 extract_scale() const;
  struct Quaternion extract_rotation() const; // Forward declaration

  // Compose matrix from TRS components
  static Mat4 compose(const Vec3 &translation, const struct Quaternion &rotation,
                      const Vec3 &scale);

  // Interpolate between two matrices using TRS decomposition
  static Mat4 interpolate(const Mat4 &start, const Mat4 &end, double t);
};

// Transformation class that manages object-to-world transformations
struct Transform {
  Mat4 object_to_world;
  Mat4 world_to_object;

  Transform();
  Transform(const Mat4 &obj_to_world);

  // Create transform from translation, rotation, and uniform scale
  static Transform from_trs(const Point &translation,
                            const Point &rotation_radians, double scale);

  // Create transform from translation, rotation, and non-uniform scale
  static Transform from_trs_nonuniform(const Point &translation,
                                       const Point &rotation_radians,
                                       const Vec3 &scale);

  // Transform ray from world space to object space
  Ray world_to_object_ray(const Ray &world_ray) const;

  // Transform point from object space to world space
  Point object_to_world_point(const Point &obj_point) const;

  // Transform normal from object space to world space
  Direction object_to_world_normal(const Direction &obj_normal) const;

  // Transform direction from object space to world space
  Direction object_to_world_direction(const Direction &obj_dir) const;

  // Transform bounding box from object space to world space
  // Returns axis-aligned bounding box that contains the transformed box
  BoundingBox transform_bbox(const BoundingBox &obj_bbox) const;
};

#endif // CGR_COURSEWORK_MATH_TRANSFORM_H
