#ifndef CGR_COURSEWORK_IO_IMAGE_H
#define CGR_COURSEWORK_IO_IMAGE_H

#include <iosfwd>
#include <string>
#include <vector>

#include "../Math/Vector.h"

struct Image {
  int height;
  int width;
  int max_value;
  std::string magic_number;
  std::vector<std::vector<Pixel>> pixels; // [height][width][Pixel with doubles]

  // Constructor for creating empty image with specified dimensions
  explicit Image(int height, int width, int max_value,
                 std::string magic_number);

  // Constructor for reading image from file
  explicit Image(const std::string &filepath);

  // Write image to file (format determined by magic_number)
  void write(const std::string &filepath) const;

  // Print image information
  friend std::ostream &operator<<(std::ostream &os, const Image &img);

private:
  // Read next integer from file, skipping comments
  static int read_next_int(std::ifstream &file);

  // Convert byte (0-max_value) to double (0.0-1.0)
  double byte_to_double(int byte_val) const;

  // Convert double (0.0-1.0) to byte (0-max_value) with clamping
  unsigned char double_to_byte(double val) const;

  // Read pixel data in ASCII format (P3)
  void read_ascii_pixels(std::ifstream &file);

  // Read pixel data in binary format (P6)
  void read_binary_pixels(std::ifstream &file);

  // Write image in binary format (P6)
  void write_binary(std::ofstream &file) const;

  // Write image in ASCII format (P3)
  void write_ascii(std::ofstream &file) const;
};

#endif // CGR_COURSEWORK_IO_IMAGE_H
