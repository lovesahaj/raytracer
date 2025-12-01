#include "Cube.h"

#include <algorithm>
#include <cmath>

#include "../Math/Transform.h"
#include "Intersections.h"

// Cube intersection
bool intersect_cube(const Cube& cube, const Ray& ray, HitRecord& hit, double t_min, double t_max) {
  // Handle motion blur by interpolating transforms
  Transform transform;
  if (cube.has_motion) {
    // Interpolate between start and end transforms based on ray time
    Mat4 current_matrix = Mat4::interpolate(cube.start_transform, cube.end_transform, ray.time);
    transform = Transform(current_matrix);
  } else {
    // Use precomputed cached transform
    transform = cube.cached_transform;
  }

  // Transform ray from world space to object space
  Ray object_ray = transform.world_to_object_ray(ray);

  // Step 3: Intersect with unit cube in object space
  // Unit cube has corners at (-0.5, -0.5, -0.5) to (0.5, 0.5, 0.5)
  Point box_min(-1.0, -1.0, -1.0);
  Point box_max(1.0, 1.0, 1.0);

  // Use slab method: intersect ray with three pairs of parallel planes
  double t_near = t_min;
  double t_far = t_max;

  // Test intersection with each pair of parallel planes (x, y, z)
  for (int i = 0; i < 3; i++) {
    if (areSame(object_ray.direction[i], 0.0)) {
      // Ray is parallel to this slab - check if origin is within bounds
      if (object_ray.origin[i] < box_min[i] || object_ray.origin[i] > box_max[i]) {
        return false;  // Ray is outside slab, no intersection
      }
    } else {
      // Compute intersection distances with both planes of this slab
      double t1 = (box_min[i] - object_ray.origin[i]) / object_ray.direction[i];
      double t2 = (box_max[i] - object_ray.origin[i]) / object_ray.direction[i];

      // Ensure t1 is the near intersection, t2 is the far
      if (t1 > t2) std::swap(t1, t2);

      // Update overall near and far distances
      t_near = std::max(t_near, t1);
      t_far = std::min(t_far, t2);

      // If near is beyond far, ray missed the box
      if (t_near > t_far) return false;
    }
  }

  // Check if intersection is in valid range
  if (t_near < t_min || t_near > t_max) {
    return false;
  }

  // Step 4: Compute hit point in object space
  Point object_hit_point = object_ray.origin + object_ray.direction * t_near;

  // Step 5: Determine which face was hit (compute normal in object space)
  Direction object_normal;
  double tolerance = 1e-4;  // Slightly larger than epsilon for face detection

  // Check each axis to see which face was hit
  if (areSame(object_hit_point.x, 1.0) || std::abs(object_hit_point.x - 1.0) < tolerance)
    object_normal = Vec3(1, 0, 0);  // +X face
  else if (areSame(object_hit_point.x, -1.0) || std::abs(object_hit_point.x + 1.0) < tolerance)
    object_normal = Vec3(-1, 0, 0);  // -X face
  else if (areSame(object_hit_point.y, 1.0) || std::abs(object_hit_point.y - 1.0) < tolerance)
    object_normal = Vec3(0, 1, 0);  // +Y face
  else if (areSame(object_hit_point.y, -1.0) || std::abs(object_hit_point.y + 1.0) < tolerance)
    object_normal = Vec3(0, -1, 0);  // -Y face
  else if (areSame(object_hit_point.z, 1.0) || std::abs(object_hit_point.z - 1.0) < tolerance)
    object_normal = Vec3(0, 0, 1);  // +Z face
  else
    object_normal = Vec3(0, 0, -1);  // -Z face

  // Step 6: Transform results back to world space
  hit.intersection_point = transform.object_to_world_point(object_hit_point);

  // CRITICAL FIX: Calculate world-space t from world-space hit point
  // Using object-space t is WRONG when there's non-uniform scaling
  Vec3 world_offset = hit.intersection_point - ray.origin;
  hit.t = world_offset.length() / ray.direction.length();

  Direction world_normal = transform.object_to_world_normal(object_normal);
  hit.set_face_normal(ray, world_normal);

  // Step 7: Set material properties
  hit.material = cube.material;
  hit.object_name = cube.name;

  // Step 8: Calculate texture coordinates (box mapping in world scale)
  // Use OBJECT SPACE coordinates multiplied by SCALE to get World Space
  // dimensions This ensures the texture size is consistent (not stretched)
  // regardless of cube scale.

  // Tangent space vectors in object space (depend on which face we hit)
  Direction object_tangent, object_bitangent;

  // Use the dominant axis to determine UV mapping
  if (std::abs(object_normal.x) > 0.5) {
    // X-face: use Y and Z in object space
    // Scale UVs by the dimension of the face to prevent stretching/squashing
    hit.u = (object_hit_point.z + 1.0) * cube.scale.z;
    hit.v = (object_hit_point.y + 1.0) * cube.scale.y;

    // Tangent along Z, bitangent along Y
    object_tangent = Vec3(0, 0, object_normal.x > 0 ? 1 : -1);
    object_bitangent = Vec3(0, 1, 0);
  } else if (std::abs(object_normal.y) > 0.5) {
    // Y-face: use X and Z in object space
    hit.u = (object_hit_point.x + 1.0) * cube.scale.x;
    hit.v = (object_hit_point.z + 1.0) * cube.scale.z;

    // Tangent along X, bitangent along Z
    object_tangent = Vec3(1, 0, 0);
    object_bitangent = Vec3(0, 0, object_normal.y > 0 ? 1 : -1);
  } else {
    // Z-face: use X and Y in object space
    hit.u = (object_hit_point.x + 1.0) * cube.scale.x;
    hit.v = (object_hit_point.y + 1.0) * cube.scale.y;

    // Tangent along X, bitangent along Y
    object_tangent = Vec3(object_normal.z > 0 ? 1 : -1, 0, 0);
    object_bitangent = Vec3(0, 1, 0);
  }

  // Step 9: Transform tangent and bitangent to world space
  hit.tangent = transform.object_to_world_direction(object_tangent).norm();
  hit.bitangent = transform.object_to_world_direction(object_bitangent).norm();

  // Gram-Schmidt orthogonalization to ensure orthogonal TBN basis
  // 1. Re-orthogonalize Tangent with respect to Normal
  hit.tangent = (hit.tangent - hit.normal * hit.tangent.dot(hit.normal)).norm();

  // 2. Re-calculate Bitangent to be orthogonal to both Normal and Tangent
  hit.bitangent = hit.normal.cross(hit.tangent).norm();

  return true;
}

// Cube bounding box
BoundingBox get_cube_bounding_box(const Cube& cube) {
  // Unit cube has bounding box from (-1.0,-1.0,-1.0) to (1.0,1.0,1.0) in object
  // space
  BoundingBox object_bbox{Point(-1.0, -1.0, -1.0), Point(1.0, 1.0, 1.0)};

  if (cube.has_motion) {
    // Compute union of bounding boxes at start and end of motion
    Transform start_transform(cube.start_transform);
    Transform end_transform(cube.end_transform);

    BoundingBox box_start = start_transform.transform_bbox(object_bbox);
    BoundingBox box_end = end_transform.transform_bbox(object_bbox);

    // Return union of both boxes
    BoundingBox result;
    result.min.x = std::min(box_start.min.x, box_end.min.x);
    result.min.y = std::min(box_start.min.y, box_end.min.y);
    result.min.z = std::min(box_start.min.z, box_end.min.z);
    result.max.x = std::max(box_start.max.x, box_end.max.x);
    result.max.y = std::max(box_start.max.y, box_end.max.y);
    result.max.z = std::max(box_start.max.z, box_end.max.z);
    return result;
  } else {
    // Use precomputed cached transform
    return cube.cached_transform.transform_bbox(object_bbox);
  }
}
