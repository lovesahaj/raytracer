#ifndef CGR_COURSEWORK_IO_TEXTURE_H
#define CGR_COURSEWORK_IO_TEXTURE_H

#include <map>
#include <memory>
#include <shared_mutex>
#include <string>

#include "../Math/Vector.h"
#include "Image.h"

// Texture manager class for loading and sampling textures
class TextureManager {
private:
  std::map<std::string, std::shared_ptr<Image>> textures;
  mutable std::shared_mutex mutex;

public:
  // Load a texture from file
  bool load_texture(const std::string &filename);

  // Sample texture at UV coordinates
  // Returns the color at the given UV coordinates, with wrapping
  Color sample(const std::string &filename, double u, double v);

  // Check if texture is loaded
  bool has_texture(const std::string &filename) const;
};

#endif // CGR_COURSEWORK_IO_TEXTURE_H
