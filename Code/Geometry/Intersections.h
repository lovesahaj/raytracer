#ifndef CGR_COURSEWORK_GEOMETRY_INTERSECTIONS_H
#define CGR_COURSEWORK_GEOMETRY_INTERSECTIONS_H

#include "../Core/Ray.h"
#include "BoundingBox.h"
#include "Cone.h"
#include "Cube.h"
#include "Cylinder.h"
#include "HitRecord.h"
#include "Plane.h"
#include "Sphere.h"
#include "Torus.h"
#include <limits>

// Helper for floating-point comparison
bool areSame(double a, double b);

// Intersection functions
bool intersect_sphere(const Sphere &sphere, const Ray &ray, HitRecord &hit,
                      double t_min = 0.001,
                      double t_max = std::numeric_limits<double>::max());

BoundingBox get_sphere_bounding_box(const Sphere &sphere);

bool intersect_cube(const Cube &cube, const Ray &ray, HitRecord &hit,
                    double t_min = 0.001,
                    double t_max = std::numeric_limits<double>::max());

BoundingBox get_cube_bounding_box(const Cube &cube);

bool intersect_plane(const Plane &plane, const Ray &ray, HitRecord &hit,
                     double t_min = 0.001,
                     double t_max = std::numeric_limits<double>::max());

BoundingBox get_plane_bounding_box(const Plane &plane);

bool intersect_torus(const Torus &torus, const Ray &ray, HitRecord &hit,
                     double t_min = 0.001,
                     double t_max = std::numeric_limits<double>::max());

BoundingBox get_torus_bounding_box(const Torus &torus);

bool intersect_cylinder(const Cylinder &cylinder, const Ray &ray,
                        HitRecord &hit, double t_min = 0.001,
                        double t_max = std::numeric_limits<double>::max());

BoundingBox get_cylinder_bounding_box(const Cylinder &cylinder);

bool intersect_cone(const Cone &cone, const Ray &ray, HitRecord &hit,
                    double t_min = 0.001,
                    double t_max = std::numeric_limits<double>::max());

BoundingBox get_cone_bounding_box(const Cone &cone);

#endif // CGR_COURSEWORK_GEOMETRY_INTERSECTIONS_H
