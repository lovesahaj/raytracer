# CGR Raytracer

A C++ raytracer implementation with Blender integration for computer graphics rendering.

## Features

- Whitted-style recursive raytracing
- BVH acceleration structure for optimized rendering
- Multiple geometric primitives (sphere, cube, plane, torus, cylinder, cone)
- Composite BRDF shading (Lambertian diffuse + Blinn-Phong specular)
- Hard and soft shadows
- Reflection and refraction with Snell's law
- Texture mapping with UV coordinates
- Antialiasing with multi-sampling
- Glossy reflections
- Motion blur
- Depth of field
- Blender scene exporter

## Requirements

- C++20 compatible compiler (g++)
- OpenMP (libomp on macOS)
- Blender (optional, for scene creation)

## Building

Build the raytracer:
```bash
make
```

Clean build artifacts:
```bash
make clean
```

Debug build:
```bash
make debug
```

## Usage

Basic usage:
```bash
./raytracer --scene ASCII/Test1.txt --output Output/result.ppm
```

With custom settings:
```bash
./raytracer --scene ASCII/Test1.txt \
  --output Output/result.ppm \
  --resolution 1920 1080 \
  --samples 16 \
  --max-depth 5
```

Available options:
- `--scene <file>` - Input ASCII scene file
- `--output <file>` - Output PPM file
- `--resolution <w> <h>` - Image resolution
- `--samples <n>` - Antialiasing samples per pixel (1 = no AA, 4+ = AA enabled)
- `--max-depth <n>` - Maximum ray recursion depth
- `--enable-textures` / `--disable-textures` - Enable/disable texture mapping (default: enabled)
- `--soft-shadows <n>` - Enable soft shadows with n samples
- `--glossy-reflection <n>` - Enable glossy reflections with n samples
- `--motion-blur <n>` - Enable motion blur with n temporal samples (0 to disable)
- `--disable-motion-blur` - Disable motion blur completely
- `--depth-of-field <a> <d>` - Enable DOF with aperture f-stop and focal distance
- `--disable-dof` - Disable depth of field
- `--light-intensity <f>` - Global light intensity multiplier (default: 0.2)
- `--ambient-light <f>` - Ambient light factor (default: 1.0)
- `--threads <n>` - Number of rendering threads (0 = auto-detect)
- `--log-level <level>` - Set log level (debug, info, warn, error)

**Note**: The following features are always enabled and cannot be disabled:
- BVH acceleration structure
- Shadows (with transparency support)
- OpenMP parallelization
- Fresnel reflections for glass materials
- Normal and bump mapping (when textures provide them)

## Makefile Targets

### Build Targets
```bash
make            # Build the raytracer (default)
make clean      # Remove build artifacts
make debug      # Build with debug symbols
```

### Test Targets
```bash
make test-1     # Render Test1 scene with optimal settings
make test-2     # Render Test2 scene with optimal settings
# ... test-3 through test-7 also available
make test-all   # Render all test scenes (test-1 through test-7)
```

### Workflow Targets
```bash
make run                         # Build and run raytracer
make benchmark                   # Run with timing information
make workflow                    # Full workflow: export → build → render
make export BLEND_FILE=Test1.blend  # Export Blender scene to ASCII
make export-all                  # Export all .blend files
```

### Utility Targets
```bash
make convert       # Convert PPM files to PNG
make view-output   # Open output directory (macOS)
make info          # Show build information
make help          # Show all available targets
```

## Examples

### Rendering with different features

Disable textures:
```bash
./raytracer --scene ASCII/Test1.txt --output Output/result.ppm --disable-textures
```

High quality render with all features:
```bash
./raytracer --scene ASCII/Test5.txt \
  --output Output/result.ppm \
  --resolution 1920 1080 \
  --samples 16 \
  --soft-shadows 16 \
  --glossy-reflection 8 \
  --motion-blur 12 \
  --max-depth 8
```

Quick preview (low quality):
```bash
./raytracer --scene ASCII/Test1.txt \
  --output Output/preview.ppm \
  --resolution 640 480 \
  --samples 4 \
  --max-depth 3
```

## Project Structure

```
Code/
├── Core/       - Main application, camera, scene, config
├── Math/       - Vector, transform, quaternion utilities
├── Geometry/   - Shape primitives and BVH acceleration
├── IO/         - Image I/O, texture loading, scene parsing
└── Render/     - Raytracing engine
ASCII/          - Scene description files
Blend/          - Blender files and export scripts
Output/         - Rendered images
```

## License

Academic project for computer graphics course.
