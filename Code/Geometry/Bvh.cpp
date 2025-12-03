#include "Bvh.h"

#include <algorithm>

#include "../Render/Raytracer.h"
#include "../Utils/logger.h"
#include "HitRecord.h"
#include "Intersections.h"

using Utils::Logger;

// External counter for intersection tests (defined in raytracer.cpp)
extern thread_local long long g_intersection_tests;

// Compute bounding box that encompasses all specified objects
BoundingBox compute_bounding_box_for_objects(std::vector<int> &object_indices,
                                             const Scene &scene) {
  // Handle empty case
  if (object_indices.empty()) {
    Logger::instance().Debug().Msg("Computing bbox for empty object set");
    return BoundingBox{Point(0, 0, 0), Point(0, 0, 0)};
  }

  Logger::instance()
      .Debug()
      .Int("object_count", object_indices.size())
      .Msg("Computing bounding box for objects");

  // Start with first object's bounding box
  BoundingBox result;
  bool first = true;

  int num_spheres = scene.spheres.size();
  int num_cubes = scene.cubes.size();
  int num_planes = scene.planes.size();
  int num_toruses = scene.toruses.size();
  int num_cylinders = scene.cylinders.size();
  int num_cones = scene.cones.size();

  for (int idx : object_indices) {
    BoundingBox obj_bbox;

    // Get bounding box based on object type
    if (idx < num_spheres) {
      obj_bbox = get_sphere_bounding_box(scene.spheres[idx]);
    } else if (idx < num_spheres + num_cubes) {
      int cube_idx = idx - num_spheres;
      obj_bbox = get_cube_bounding_box(scene.cubes[cube_idx]);
    } else if (idx < num_spheres + num_cubes + num_planes) {
      int plane_idx = idx - num_spheres - num_cubes;
      obj_bbox = get_plane_bounding_box(scene.planes[plane_idx]);
    } else if (idx < num_spheres + num_cubes + num_planes + num_toruses) {
      int torus_idx = idx - num_spheres - num_cubes - num_planes;
      obj_bbox = get_torus_bounding_box(scene.toruses[torus_idx]);
    } else if (idx < num_spheres + num_cubes + num_planes + num_toruses +
                         num_cylinders) {
      int cyl_idx = idx - num_spheres - num_cubes - num_planes - num_toruses;
      obj_bbox = get_cylinder_bounding_box(scene.cylinders[cyl_idx]);
    } else {
      int cone_idx = idx - num_spheres - num_cubes - num_planes - num_toruses -
                     num_cylinders;
      obj_bbox = get_cone_bounding_box(scene.cones[cone_idx]);
    }

    // Merge bounding boxes
    if (first) {
      result = obj_bbox;
      first = false;
    } else {
      // Expand result to include obj_bbox
      result.min.x = std::min(result.min.x, obj_bbox.min.x);
      result.min.y = std::min(result.min.y, obj_bbox.min.y);
      result.min.z = std::min(result.min.z, obj_bbox.min.z);
      result.max.x = std::max(result.max.x, obj_bbox.max.x);
      result.max.y = std::max(result.max.y, obj_bbox.max.y);
      result.max.z = std::max(result.max.z, obj_bbox.max.z);
    }
  }

  return result;
}

// Choose which axis to split on (returns 0=X, 1=Y, 2=Z)
// Chooses the axis with the largest extent
int choose_split_axis(const BoundingBox &bbox) {
  Vec3 extent = bbox.max - bbox.min;

  int axis;
  if (extent.x > extent.y && extent.x > extent.z)
    axis = 0;
  else if (extent.y > extent.z)
    axis = 1;
  else
    axis = 2;

  Logger::instance()
      .Debug()
      .Int("axis", axis)
      .Double("extent_x", extent.x)
      .Double("extent_y", extent.y)
      .Double("extent_z", extent.z)
      .Msg("Split axis chosen");

  return axis;
}

// Get the center point of an object by its index
Point get_obj_center(const Scene &scene, int object_index) {
  int num_spheres = scene.spheres.size();
  int num_cubes = scene.cubes.size();
  int num_planes = scene.planes.size();
  int num_toruses = scene.toruses.size();
  int num_cylinders = scene.cylinders.size();
  int num_cones = scene.cones.size();

  if (object_index < num_spheres) {
    return scene.spheres[object_index].location;
  } else if (object_index < num_spheres + num_cubes) {
    int cube_idx = object_index - num_spheres;
    return scene.cubes[cube_idx].translation;
  } else if (object_index < num_spheres + num_cubes + num_planes) {
    int plane_idx = object_index - num_spheres - num_cubes;
    const auto &plane = scene.planes[plane_idx];
    if (plane.points.empty())
      return Point(0, 0, 0); // Fix division by zero
    Point center(0, 0, 0);
    for (const auto &p : plane.points) {
      center = center + p;
    }
    return center * (1.0 / plane.points.size());
  } else if (object_index <
             num_spheres + num_cubes + num_planes + num_toruses) {
    int torus_idx = object_index - num_spheres - num_cubes - num_planes;
    return scene.toruses[torus_idx].location;
  } else if (object_index < num_spheres + num_cubes + num_planes + num_toruses +
                                num_cylinders) {
    int cyl_idx =
        object_index - num_spheres - num_cubes - num_planes - num_toruses;
    return scene.cylinders[cyl_idx].location;
  } else {
    int cone_idx = object_index - num_spheres - num_cubes - num_planes -
                   num_toruses - num_cylinders;
    return scene.cones[cone_idx].location;
  }
}

// Partition objects into left and right groups based on median split along
// chosen axis
std::pair<std::vector<int>, std::vector<int>>
partition_objs(const std::vector<int> &object_indices, const Scene &scene,
               int axis) {
  Logger::instance()
      .Debug()
      .Int("total_objects", object_indices.size())
      .Int("split_axis", axis)
      .Msg("Partitioning objects");

  // Handle edge case: too few objects to partition
  if (object_indices.size() <= 1) {
    Logger::instance().Debug().Msg("Too few objects to partition");
    return {object_indices, {}};
  }

  std::vector<int> sorted_indices = object_indices;

  // Use nth_element for O(N) median finding instead of O(N log N) sort
  int mid = sorted_indices.size() / 2;
  std::nth_element(sorted_indices.begin(), sorted_indices.begin() + mid,
                   sorted_indices.end(), [&scene, axis](int a, int b) {
                     Point center_a = get_obj_center(scene, a);
                     Point center_b = get_obj_center(scene, b);
                     return center_a[axis] < center_b[axis];
                   });

  // Ensure at least one object in left (prevents empty left partition)
  if (mid == 0)
    mid = 1;

  std::vector<int> left_objects(sorted_indices.begin(),
                                sorted_indices.begin() + mid);
  std::vector<int> right_objects(sorted_indices.begin() + mid,
                                 sorted_indices.end());

  Logger::instance()
      .Debug()
      .Int("left_count", left_objects.size())
      .Int("right_count", right_objects.size())
      .Msg("Partition complete");

  return {left_objects, right_objects};
}

// Recursively build BVH tree
std::unique_ptr<BVHNode> build_bvh(std::vector<int> &object_indices,
                                   const Scene &scene, int depth) {
  Logger::instance()
      .Debug()
      .Int("depth", depth)
      .Int("objects", object_indices.size())
      .Msg("Building BVH node");

  auto node = std::make_unique<BVHNode>();

  node->bbox = compute_bounding_box_for_objects(object_indices, scene);

  // Fix depth check: >= MAX_DEPTH
  if (object_indices.size() <= MAX_LEAF_SIZE || depth >= MAX_DEPTH) {
    node->object_indices = object_indices;
    Logger::instance()
        .Debug()
        .Int("depth", depth)
        .Int("leaf_objects", object_indices.size())
        .Msg("Created leaf node");
    return node;
  }

  int axis = choose_split_axis(node->bbox);

  auto [left_objs, right_objs] = partition_objs(object_indices, scene, axis);

  // Fix unbalanced partition edge case: if one side is empty, make this a leaf
  if (left_objs.empty() || right_objs.empty()) {
    node->object_indices = object_indices;
    Logger::instance()
        .Warn()
        .Int("depth", depth)
        .Bool("left_empty", left_objs.empty())
        .Bool("right_empty", right_objs.empty())
        .Msg("Unbalanced partition - creating leaf");
    return node;
  }

  Logger::instance()
      .Debug()
      .Int("depth", depth)
      .Msg("Creating internal BVH node");

  node->left = build_bvh(left_objs, scene, depth + 1);
  node->right = build_bvh(right_objs, scene, depth + 1);

  return node;
}

bool intersect_obj(const Ray &ray, const Scene &scene, int idx,
                   HitRecord &hit_record, double t_min, double t_max) {
  g_intersection_tests++; // Count each object intersection test

  int num_spheres = scene.spheres.size();
  int num_cubes = scene.cubes.size();
  int num_planes = scene.planes.size();
  int num_toruses = scene.toruses.size();
  int num_cylinders = scene.cylinders.size();
  int num_cones = scene.cones.size();

  // Check visibility flag
  bool visible = true;
  if (idx < num_spheres)
    visible = scene.spheres[idx].visible;
  else if (idx < num_spheres + num_cubes)
    visible = scene.cubes[idx - num_spheres].visible;
  else if (idx < num_spheres + num_cubes + num_planes)
    visible = scene.planes[idx - num_spheres - num_cubes].visible;
  else if (idx < num_spheres + num_cubes + num_planes + num_toruses)
    visible = scene.toruses[idx - num_spheres - num_cubes - num_planes].visible;
  else if (idx <
           num_spheres + num_cubes + num_planes + num_toruses + num_cylinders)
    visible =
        scene
            .cylinders[idx - num_spheres - num_cubes - num_planes - num_toruses]
            .visible;
  else
    visible = scene
                  .cones[idx - num_spheres - num_cubes - num_planes -
                         num_toruses - num_cylinders]
                  .visible;

  if (!visible)
    return false;

  // Get intersection based on object type
  if (idx < num_spheres) {
    return intersect_sphere(scene.spheres[idx], ray, hit_record, t_min, t_max);
  } else if (idx < num_spheres + num_cubes) {
    int cube_idx = idx - num_spheres;
    return intersect_cube(scene.cubes[cube_idx], ray, hit_record, t_min, t_max);
  } else if (idx < num_spheres + num_cubes + num_planes) {
    int plane_idx = idx - num_spheres - num_cubes;
    return intersect_plane(scene.planes[plane_idx], ray, hit_record, t_min,
                           t_max);
  } else if (idx < num_spheres + num_cubes + num_planes + num_toruses) {
    int torus_idx = idx - num_spheres - num_cubes - num_planes;
    return intersect_torus(scene.toruses[torus_idx], ray, hit_record, t_min,
                           t_max);
  } else if (idx < num_spheres + num_cubes + num_planes + num_toruses +
                       num_cylinders) {
    int cyl_idx = idx - num_spheres - num_cubes - num_planes - num_toruses;
    return intersect_cylinder(scene.cylinders[cyl_idx], ray, hit_record, t_min,
                              t_max);
  } else {
    int cone_idx = idx - num_spheres - num_cubes - num_planes - num_toruses -
                   num_cylinders;
    return intersect_cone(scene.cones[cone_idx], ray, hit_record, t_min, t_max);
  }
}

bool intersect_bvh(const Ray &ray, BVHNode *node, const Scene &scene,
                   HitRecord &closest_hit, double t_min, double &closest_t) {
  if (!node->bbox.intersect(ray, t_min, closest_t)) {
    return false;
  }

  if (node->is_leaf()) {
    bool hit_any = false;
    for (int idx : node->object_indices) {
      HitRecord temp_hit;
      if (intersect_obj(ray, scene, idx, temp_hit, t_min, closest_t)) {
        hit_any = true;
        closest_t = temp_hit.t;
        closest_hit = temp_hit;
      }
    }
    return hit_any;
  }

  // Must check both children (can't short-circuit with ||)
  // because we need to find the CLOSEST hit, not just ANY hit
  bool hit_left = intersect_bvh(ray, node->left.get(), scene, closest_hit,
                                t_min, closest_t);
  bool hit_right = intersect_bvh(ray, node->right.get(), scene, closest_hit,
                                 t_min, closest_t);

  return hit_left || hit_right;
}

void traverse_stats(const BVHNode *node, int depth, BVHStats &stats) {
  if (!node)
    return;
  stats.node_count++;
  stats.max_depth = std::max(stats.max_depth, depth);
  if (node->is_leaf()) {
    stats.leaf_count++;
  } else {
    traverse_stats(node->left.get(), depth + 1, stats);
    traverse_stats(node->right.get(), depth + 1, stats);
  }
}

BVHStats get_bvh_stats(const BVHNode *root) {
  BVHStats stats;
  traverse_stats(root, 1, stats);
  return stats;
}
