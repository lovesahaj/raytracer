#ifndef CGR_COURSEWORK_GEOMETRY_BVH_H
#define CGR_COURSEWORK_GEOMETRY_BVH_H

#include <memory>
#include <vector>

#include "../Core/Scene.h"
#include "BoundingBox.h"
#include "HitRecord.h"

// BVH parameters
#define MAX_LEAF_SIZE 2
#define MAX_DEPTH 30

// BVH Node structure
// Represents either an internal node (with left/right children) or a leaf node
// (with objects)
struct BVHNode {
  BoundingBox bbox; // Bounding box encompassing all objects in this subtree

  std::unique_ptr<BVHNode> left;  // Left child (nullptr for leaf)
  std::unique_ptr<BVHNode> right; // Right child (nullptr for leaf)

  std::vector<int> object_indices; // Object indices (only used for leaf nodes)

  bool is_leaf() const { return left == nullptr && right == nullptr; }
};

// Compute bounding box that encompasses all specified objects
BoundingBox compute_bounding_box_for_objects(std::vector<int> &object_indices,
                                             const Scene &scene);

// Choose which axis to split on (returns 0=X, 1=Y, 2=Z)
// Chooses the axis with the largest extent
int choose_split_axis(const BoundingBox &bbox);

// Get the center point of an object by its index
// Index scheme: [0..num_spheres-1] = spheres,
//               [num_spheres..num_spheres+num_cubes-1] = cubes,
//               [num_spheres+num_cubes..end] = planes
Point get_obj_center(const Scene &scene, int object_index);

// Partition objects into left and right groups based on median split along
// chosen axis Returns pair of (left_objects, right_objects)
std::pair<std::vector<int>, std::vector<int>>
partition_objs(const std::vector<int> &object_indices, const Scene &scene,
               int axis);

// Recursively build BVH tree
// Returns pointer to root node of BVH subtree
std::unique_ptr<BVHNode> build_bvh(std::vector<int> &object_indices,
                                   const Scene &scene, int depth = 0);

bool intersect_obj(const Ray &ray, const Scene &scene, int idx,
                   HitRecord &hit_record, double t_min, double t_max);

// Recursively Test against the BVHTree
bool intersect_bvh(const Ray &ray, BVHNode *node, const Scene &scene,
                   HitRecord &closest_hit, double t_min, double &closest_t);

// Helper to compute stats
struct BVHStats {
  int node_count = 0;
  int leaf_count = 0;
  int max_depth = 0;
};

BVHStats get_bvh_stats(const BVHNode *root);

#endif // CGR_COURSEWORK_GEOMETRY_BVH_H
