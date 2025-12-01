#ifndef CGR_COURSEWORK_IO_SCENE_UTILS_H
#define CGR_COURSEWORK_IO_SCENE_UTILS_H

#include "../Core/Ray.h"
#include "../Core/Scene.h"
#include <string>

Scene load_scene(const std::string &filepath);
void write_rays_to_file(const std::vector<Ray> &rays,
                        const std::string &filepath);

#endif // CGR_COURSEWORK_IO_SCENE_UTILS_H