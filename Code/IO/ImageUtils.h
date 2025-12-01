//
// Created by Lovesahaj on 16/10/25.
//

#ifndef CGR_COURSEWORK_IO_IMAGE_UTILS_H
#define CGR_COURSEWORK_IO_IMAGE_UTILS_H

#include <utility>
#include <vector>

#include "../Core/Camera.h"
#include "../Core/Ray.h"
#include "../Math/Vector.h"
#include "Image.h"

inline std::vector<Ray> image_to_world_coordinates(const Image &img,
                                                   const Camera &camera) {
  std::vector<Ray> ray_img(img.height * img.width);

  std::vector<std::vector<Vec3>> temp_img(img.height,
                                          std::vector<Vec3>(img.width));

  // step 1: converting the pixel in to Normalized Device Coordinates
  for (auto i = 0u; i < img.pixels.size(); i++) {
    for (auto j = 0u; j < img.pixels[0].size(); j++) {
      temp_img[i][j].x = ((2.0 * (j + 0.5)) / img.width) - 1;
      temp_img[i][j].y = 1 - ((2.0 * (i + 0.5)) / img.height);
    }
  }

  // step 2: converting NDC to camera space
  for (auto i = 0u; i < img.pixels.size(); i++) {
    for (auto j = 0u; j < img.pixels[0].size(); j++) {
      temp_img[i][j].x = temp_img[i][j].x * camera.sensor_width / 2.0;
      temp_img[i][j].y = temp_img[i][j].y * camera.sensor_height / 2.0;
      temp_img[i][j].z = -camera.focal_length;
    }
  }

  // step 3: camera to world space transformation
  // orthonormal basis of camera
  // camera-to-world rotation matrix R = [u v w]
  const Vec3 w = -camera.gaze_direction.norm();
  const Vec3 u = camera.up_direction.cross(w).norm();
  const Vec3 v = w.cross(u);

  // direction vector to world space
  for (auto i = 0u; i < img.pixels.size(); i++) {
    for (auto j = 0u; j < img.pixels[0].size(); j++) {
      int index = i * img.width + j;
      ray_img[index].origin = camera.location;
      ray_img[index].direction =
          ((temp_img[i][j].x * u) + (temp_img[i][j].y * v) +
           (temp_img[i][j].z * w))
              .norm();
    }
  }

  return ray_img;
}

#endif // CGR_COURSEWORK_IO_IMAGE_UTILS_H
