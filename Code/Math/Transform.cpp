#include "Transform.h"
#include "Quaternion.h"

#include <algorithm>
#include <cmath>

// Compute inverse matrix (using Gaussian elimination)
Mat4 Mat4::inverse() const {
  Mat4 result;
  double aug[4][8];  // Augmented matrix [A|I]

  // Initialize augmented matrix
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      aug[i][j] = m[i][j];
      aug[i][j + 4] = (i == j) ? 1.0 : 0.0;
    }
  }

  // Forward elimination
  for (int i = 0; i < 4; i++) {
    // Find pivot
    int pivot = i;
    for (int j = i + 1; j < 4; j++) {
      if (std::abs(aug[j][i]) > std::abs(aug[pivot][i])) {
        pivot = j;
      }
    }

    // Swap rows
    if (pivot != i) {
      for (int j = 0; j < 8; j++) {
        std::swap(aug[i][j], aug[pivot][j]);
      }
    }

    // Scale pivot row
    double scale = aug[i][i];
    if (std::abs(scale) < 1e-10) continue;  // Singular matrix

    for (int j = 0; j < 8; j++) {
      aug[i][j] /= scale;
    }

    // Eliminate column
    for (int j = 0; j < 4; j++) {
      if (i != j) {
        double factor = aug[j][i];
        for (int k = 0; k < 8; k++) {
          aug[j][k] -= factor * aug[i][k];
        }
      }
    }
  }

  // Extract inverse from augmented matrix
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      result.m[i][j] = aug[i][j + 4];
    }
  }

  return result;
}

// Transform constructors
Transform::Transform() : object_to_world(), world_to_object() {}

Transform::Transform(const Mat4& obj_to_world)
    : object_to_world(obj_to_world), world_to_object(obj_to_world.inverse()) {}

// Create transform from translation, rotation, and uniform scale
Transform Transform::from_trs(const Point& translation, const Point& rotation_radians,
                              double scale) {
  Mat4 t = Mat4::translate(translation);
  Mat4 r = Mat4::rotate_euler(rotation_radians);
  Mat4 s = Mat4::scale_uniform(scale);

  // Apply in order: Scale -> Rotate -> Translate
  Mat4 obj_to_world = t * r * s;
  return Transform(obj_to_world);
}

// Create transform from translation, rotation, and non-uniform scale
Transform Transform::from_trs_nonuniform(const Point& translation, const Point& rotation_radians,
                                         const Vec3& scale) {
  Mat4 t = Mat4::translate(translation);
  Mat4 r = Mat4::rotate_euler(rotation_radians);
  Mat4 s = Mat4::scale(scale.x, scale.y, scale.z);

  Mat4 obj_to_world = t * r * s;
  return Transform(obj_to_world);
}

// Transform ray from world space to object space
Ray Transform::world_to_object_ray(const Ray& world_ray) const {
  return Ray(world_to_object.transform_point(world_ray.origin),
             world_to_object.transform_direction(world_ray.direction), world_ray.time);
}

// Transform point from object space to world space
Point Transform::object_to_world_point(const Point& obj_point) const {
  return object_to_world.transform_point(obj_point);
}

// Transform normal from object space to world space
Direction Transform::object_to_world_normal(const Direction& obj_normal) const {
  return world_to_object.transform_normal(obj_normal).norm();
}

// Transform direction from object space to world space
Direction Transform::object_to_world_direction(const Direction& obj_dir) const {
  return object_to_world.transform_direction(obj_dir).norm();
}

// Transform bounding box from object space to world space
BoundingBox Transform::transform_bbox(const BoundingBox& obj_bbox) const {
  // Transform all 8 corners of the box
  Vec3 corners[8] = {Vec3(obj_bbox.min.x, obj_bbox.min.y, obj_bbox.min.z),
                     Vec3(obj_bbox.max.x, obj_bbox.min.y, obj_bbox.min.z),
                     Vec3(obj_bbox.min.x, obj_bbox.max.y, obj_bbox.min.z),
                     Vec3(obj_bbox.max.x, obj_bbox.max.y, obj_bbox.min.z),
                     Vec3(obj_bbox.min.x, obj_bbox.min.y, obj_bbox.max.z),
                     Vec3(obj_bbox.max.x, obj_bbox.min.y, obj_bbox.max.z),
                     Vec3(obj_bbox.min.x, obj_bbox.max.y, obj_bbox.max.z),
                     Vec3(obj_bbox.max.x, obj_bbox.max.y, obj_bbox.max.z)};

  Point world_min = object_to_world.transform_point(corners[0]);
  Point world_max = world_min;

  for (int i = 1; i < 8; i++) {
    Point world_corner = object_to_world.transform_point(corners[i]);
    world_min.x = std::min(world_min.x, world_corner.x);
    world_min.y = std::min(world_min.y, world_corner.y);
    world_min.z = std::min(world_min.z, world_corner.z);
    world_max.x = std::max(world_max.x, world_corner.x);
    world_max.y = std::max(world_max.y, world_corner.y);
    world_max.z = std::max(world_max.z, world_corner.z);
  }

  return BoundingBox{world_min, world_max};
}

// Extract translation from transformation matrix
Vec3 Mat4::extract_translation() const {
  return Vec3(m[0][3], m[1][3], m[2][3]);
}

// Extract scale from transformation matrix
Vec3 Mat4::extract_scale() const {
  // Scale is the length of each basis vector (column)
  Vec3 scale_x(m[0][0], m[1][0], m[2][0]);
  Vec3 scale_y(m[0][1], m[1][1], m[2][1]);
  Vec3 scale_z(m[0][2], m[1][2], m[2][2]);

  return Vec3(scale_x.length(), scale_y.length(), scale_z.length());
}

// Extract rotation quaternion from transformation matrix
Quaternion Mat4::extract_rotation() const {
  // First remove scale from the matrix
  Vec3 scale = extract_scale();

  // Handle zero scale (degenerate case)
  if (scale.x < 1e-10 || scale.y < 1e-10 || scale.z < 1e-10) {
    return Quaternion(); // Return identity
  }

  // Create normalized rotation matrix by dividing out scale
  double rot[3][3];
  for (int i = 0; i < 3; i++) {
    rot[i][0] = m[i][0] / scale.x;
    rot[i][1] = m[i][1] / scale.y;
    rot[i][2] = m[i][2] / scale.z;
  }

  // Convert rotation matrix to quaternion
  // Using Shepperd's method (numerically stable)
  double trace = rot[0][0] + rot[1][1] + rot[2][2];

  if (trace > 0.0) {
    // w is largest component
    double s = std::sqrt(trace + 1.0) * 2.0;
    return Quaternion(0.25 * s, (rot[2][1] - rot[1][2]) / s,
                      (rot[0][2] - rot[2][0]) / s,
                      (rot[1][0] - rot[0][1]) / s);
  } else if (rot[0][0] > rot[1][1] && rot[0][0] > rot[2][2]) {
    // x is largest component
    double s = std::sqrt(1.0 + rot[0][0] - rot[1][1] - rot[2][2]) * 2.0;
    return Quaternion((rot[2][1] - rot[1][2]) / s, 0.25 * s,
                      (rot[0][1] + rot[1][0]) / s,
                      (rot[0][2] + rot[2][0]) / s);
  } else if (rot[1][1] > rot[2][2]) {
    // y is largest component
    double s = std::sqrt(1.0 + rot[1][1] - rot[0][0] - rot[2][2]) * 2.0;
    return Quaternion((rot[0][2] - rot[2][0]) / s,
                      (rot[0][1] + rot[1][0]) / s, 0.25 * s,
                      (rot[1][2] + rot[2][1]) / s);
  } else {
    // z is largest component
    double s = std::sqrt(1.0 + rot[2][2] - rot[0][0] - rot[1][1]) * 2.0;
    return Quaternion((rot[1][0] - rot[0][1]) / s,
                      (rot[0][2] + rot[2][0]) / s,
                      (rot[1][2] + rot[2][1]) / s, 0.25 * s);
  }
}

// Compose matrix from TRS components
Mat4 Mat4::compose(const Vec3 &translation, const Quaternion &rotation,
                   const Vec3 &scale) {
  Mat4 result;

  // Convert quaternion to rotation matrix
  double rot[3][3];
  rotation.to_matrix(rot);

  // Combine rotation and scale into upper-left 3x3
  for (int i = 0; i < 3; i++) {
    result.m[i][0] = rot[i][0] * scale.x;
    result.m[i][1] = rot[i][1] * scale.y;
    result.m[i][2] = rot[i][2] * scale.z;
  }

  // Set translation
  result.m[0][3] = translation.x;
  result.m[1][3] = translation.y;
  result.m[2][3] = translation.z;

  // Bottom row stays [0, 0, 0, 1]
  result.m[3][0] = 0.0;
  result.m[3][1] = 0.0;
  result.m[3][2] = 0.0;
  result.m[3][3] = 1.0;

  return result;
}

// Interpolate between two matrices using TRS decomposition
Mat4 Mat4::interpolate(const Mat4 &start, const Mat4 &end, double t) {
  // Decompose both matrices
  Vec3 start_t = start.extract_translation();
  Vec3 start_s = start.extract_scale();
  Quaternion start_r = start.extract_rotation();

  Vec3 end_t = end.extract_translation();
  Vec3 end_s = end.extract_scale();
  Quaternion end_r = end.extract_rotation();

  // Interpolate each component
  Vec3 curr_t = start_t + (end_t - start_t) * t;
  Vec3 curr_s = start_s + (end_s - start_s) * t;
  Quaternion curr_r = Quaternion::slerp(start_r, end_r, t);

  // Recompose into matrix
  return compose(curr_t, curr_r, curr_s);
}
