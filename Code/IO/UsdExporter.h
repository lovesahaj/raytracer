#ifndef CGR_COURSEWORK_IO_USD_EXPORTER_H
#define CGR_COURSEWORK_IO_USD_EXPORTER_H

#include "../Core/Scene.h"
#include <string>

// Export the scene to a USDA (text-based USD) file
void export_scene_to_usd(const Scene &scene, const std::string &filename);

#endif // CGR_COURSEWORK_IO_USD_EXPORTER_H
