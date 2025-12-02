#ifndef CGR_COURSEWORK_GEOMETRY_BVH_H
#define CGR_COURSEWORK_GEOMETRY_BVH_H

#include <memory>
#include <vector>

#include "../Core/Scene.h"
#include "BoundingBox.h"
#include "HitRecord.h"

#define MAX_LEAF_SIZE 2
#define MAX_DEPTH 30

struct BVHNode {
  BoundingBox bbox;
  std::unique_ptr<BVHNode> left;
  std::unique_ptr<BVHNode> right;
  std::vector<int> object_indices;

  bool is_leaf() const { return left == nullptr && right == nullptr; }
};

BoundingBox compute_bounding_box_for_objects(std::vector<int> &object_indices,
                                             const Scene &scene);

int choose_split_axis(const BoundingBox &bbox);

Point get_obj_center(const Scene &scene, int object_index);

std::pair<std::vector<int>, std::vector<int>>
partition_objs(const std::vector<int> &object_indices, const Scene &scene,
               int axis);

std::unique_ptr<BVHNode> build_bvh(std::vector<int> &object_indices,
                                   const Scene &scene, int depth = 0);

bool intersect_obj(const Ray &ray, const Scene &scene, int idx,
                   HitRecord &hit_record, double t_min, double t_max);

bool intersect_bvh(const Ray &ray, BVHNode *node, const Scene &scene,
                   HitRecord &closest_hit, double t_min, double &closest_t);

struct BVHStats {
  int node_count = 0;
  int leaf_count = 0;
  int max_depth = 0;
};

BVHStats get_bvh_stats(const BVHNode *root);

#endif // CGR_COURSEWORK_GEOMETRY_BVH_H
