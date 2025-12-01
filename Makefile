CXX = g++
# OpenMP flags for macOS (using Homebrew's libomp)
LIBOMP_PREFIX := $(shell brew --prefix libomp 2>/dev/null)
ifeq ($(LIBOMP_PREFIX),)
    ifneq ($(wildcard /opt/homebrew/opt/libomp),)
        LIBOMP_PREFIX := /opt/homebrew/opt/libomp
    endif
endif

ifneq ($(LIBOMP_PREFIX),)
  OPENMP_CFLAGS = -Xpreprocessor -fopenmp -I$(LIBOMP_PREFIX)/include
  OPENMP_LDFLAGS = -L$(LIBOMP_PREFIX)/lib -lomp
else
  OPENMP_CFLAGS = -fopenmp
  OPENMP_LDFLAGS = -fopenmp
endif

LDFLAGS = $(OPENMP_LDFLAGS)
TARGET = raytracer

# Build directory
BUILD_DIR = build

# Directories
SCENE_DIR = ASCII
OUTPUT_DIR = Output
BLEND_DIR = Blend

# Default scene
SCENE_FILE ?= Test1.txt

# Blender Configuration
BLENDER_PATH = /Applications/Blender.app/Contents/MacOS/Blender
BLEND_FILE ?= Test1.blend
PYTHON_SCRIPT = Export.py

# VPATH to find source files
VPATH = $(CORE_DIR):$(MATH_DIR):$(GEOMETRY_DIR):$(IO_DIR):$(RENDER_DIR)
# Source files location
SRC_DIR = Code

# Subdirectories
CORE_DIR = $(SRC_DIR)/Core
MATH_DIR = $(SRC_DIR)/Math
GEOMETRY_DIR = $(SRC_DIR)/Geometry
IO_DIR = $(SRC_DIR)/IO
RENDER_DIR = $(SRC_DIR)/Render

# Include paths
INCLUDES = -I$(SRC_DIR) -I$(CORE_DIR) -I$(MATH_DIR) -I$(GEOMETRY_DIR) -I$(IO_DIR) -I$(RENDER_DIR)

# Update CXXFLAGS to include new paths
# Performance build: O3 + aggressive optimizations
# FIX: Removed -ffast-math which was causing NaN handling issues with implicit surfaces (torus)
# -ffast-math assumes NaN never happens, breaking quartic solver checks
CXXFLAGS = -std=c++20 -Wall -Wextra -O3 -march=native -funroll-loops $(OPENMP_CFLAGS) $(INCLUDES)
CXXFLAGS_DEBUG = -std=c++20 -Wall -Wextra -g -O0 $(OPENMP_CFLAGS) $(INCLUDES)

# Source files
SOURCES = \
    $(CORE_DIR)/Main.cpp \
    $(CORE_DIR)/Config.cpp \
    $(CORE_DIR)/Camera.cpp \
    $(MATH_DIR)/Transform.cpp \
    $(GEOMETRY_DIR)/Sphere.cpp \
    $(GEOMETRY_DIR)/Cube.cpp \
    $(GEOMETRY_DIR)/Plane.cpp \
    $(GEOMETRY_DIR)/Torus.cpp \
    $(GEOMETRY_DIR)/Cylinder.cpp \
    $(GEOMETRY_DIR)/Cone.cpp \
    $(GEOMETRY_DIR)/Intersections.cpp \
    $(GEOMETRY_DIR)/Bvh.cpp \
    $(IO_DIR)/Image.cpp \
    $(IO_DIR)/Texture.cpp \
    $(IO_DIR)/SceneLoader.cpp \
    $(IO_DIR)/UsdExporter.cpp \
    $(RENDER_DIR)/Raytracer.cpp

# Header files (for dependency tracking)
HEADERS = \
    $(CORE_DIR)/Ray.h \
    $(CORE_DIR)/Camera.h \
    $(CORE_DIR)/Light.h \
    $(CORE_DIR)/Material.h \
    $(CORE_DIR)/Scene.h \
    $(CORE_DIR)/Config.h \
    $(MATH_DIR)/Vector.h \
    $(MATH_DIR)/Transform.h \
    $(GEOMETRY_DIR)/Shape.h \
    $(GEOMETRY_DIR)/Sphere.h \
    $(GEOMETRY_DIR)/Cube.h \
    $(GEOMETRY_DIR)/Plane.h \
    $(GEOMETRY_DIR)/Torus.h \
    $(GEOMETRY_DIR)/Cylinder.h \
    $(GEOMETRY_DIR)/Cone.h \
    $(GEOMETRY_DIR)/BoundingBox.h \
    $(GEOMETRY_DIR)/HitRecord.h \
    $(GEOMETRY_DIR)/Intersections.h \
    $(GEOMETRY_DIR)/Bvh.h \
    $(IO_DIR)/Image.h \
    $(IO_DIR)/ImageUtils.h \
    $(IO_DIR)/Texture.h \
    $(IO_DIR)/SceneUtils.h \
    $(IO_DIR)/UsdExporter.h \
    $(RENDER_DIR)/Raytracer.h

# Object files
# Map source files to object files in build directory, preserving structure or flattening
# Flattening is easier for now: build/Main.o, build/Transform.o, etc.
# But we have multiple directories. Let's keep it simple and flatten if names are unique.
# They seem unique.
OBJECTS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(SOURCES)))

# Default target
.DEFAULT_GOAL := all

all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
	@echo "Build complete!"

# Compile source files
$(BUILD_DIR)/%.o: %.cpp $(HEADERS) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Debug build
debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: clean $(TARGET)
	@echo "Debug build complete!"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Clean complete!"

# Run the raytracer with default scene
run: $(TARGET)
	@echo "Running raytracer with $(SCENE_FILE)..."
	./$(TARGET)

# Render a specific scene
render: $(TARGET)
	@if [ -z "$(SCENE)" ]; then \
		echo "Usage: make render SCENE=scene_name"; \
		echo "Example: make render SCENE=Test1"; \
		exit 1; \
	fi
	@echo "Rendering scene: $(SCENE).txt"
	./$(TARGET) --scene $(SCENE_DIR)/$(SCENE).txt --output $(OUTPUT_DIR)/$(SCENE).ppm
	@echo "Render complete! Saved to $(OUTPUT_DIR)/$(SCENE).ppm"
	
# Benchmark mode - render with timing
benchmark: $(TARGET)
	@echo "Running benchmark..."
	time ./$(TARGET) --scene $(SCENE_DIR)/$(SCENE_FILE) --output $(OUTPUT_DIR)/benchmark.ppm

# Check code with static analysis
check:
	@echo "Running static analysis..."
	cppcheck --enable=all --suppress=missingIncludeSystem $(SOURCES)

# Format code (requires clang-format)
format:
	@echo "Formatting code..."
	@for src in $(SOURCES) $(HEADERS); do \
		if [ -f $$src ]; then \
			clang-format -i $$src; \
			echo "Formatted $$src"; \
		fi; \
	done

# Create output directory
init-output:
	mkdir -p $(OUTPUT_DIR)

# Module 4: Generate all test renders with high quality settings
test-all: $(TARGET) init-output
	@echo "=== Generating Module 4 Test Renders ==="
	@echo "Using high quality settings: 800x600, 32 samples"
	@echo ""
	@echo "[1/8] Rendering point light (Test1)..."
	./$(TARGET) -s $(SCENE_DIR)/Test1.txt -o $(OUTPUT_DIR)/test_point_light.ppm -w 800 -H 600 --samples 32
	@echo "[2/8] Rendering area light (Test6)..."
	./$(TARGET) -s $(SCENE_DIR)/Test6.txt -o $(OUTPUT_DIR)/test_area_light.ppm -w 800 -H 600 --samples 32
	@echo "[3/8] Rendering motion blur (Test5)..."
	./$(TARGET) -s $(SCENE_DIR)/Test5.txt -o $(OUTPUT_DIR)/test_motion_blur.ppm -w 800 -H 600 --samples 32
	@echo ""
	@echo "=== All Test Renders Complete ==="
	@echo "Output saved to: $(OUTPUT_DIR)/"
	@echo ""
	@echo "Generated files:"
	@ls -lh $(OUTPUT_DIR)/test_*.ppm | awk '{printf "  %-40s %8s\n", $$9, $$5}'

# Full workflow: export from Blender and render
workflow:
	@echo "=== Full Rendering Workflow ==="
	@echo "Step 1: Exporting scene from Blender..."
	$(MAKE) export
	@echo ""
	@echo "Step 2: Building raytracer..."
	$(MAKE) all
	@echo ""
	@echo "Step 3: Rendering scene..."
	$(MAKE) run
	@echo ""
	@echo "=== Workflow Complete ==="
	@echo "Output images saved to: $(OUTPUT_DIR)/"

# Export a specific blend file (use: make export BLEND_FILE=scene.blend)
export:
	@echo "Exporting $(BLEND_FILE) to ASCII format..."
	$(BLENDER_PATH) $(BLEND_DIR)/$(BLEND_FILE) --background --python $(BLEND_DIR)/$(PYTHON_SCRIPT)
	@echo "Export complete!"

# Export all blend files in the directory
export-all:
	@echo "Exporting all .blend files..."
	@for blend in $(BLEND_DIR)/*.blend; do \
		echo "Exporting $$blend..."; \
		$(BLENDER_PATH) "$$blend" --background --python $(BLEND_DIR)/Export.py; \
	done
	@echo "All exports complete!"

# Open a blend file in Blender GUI
view:
	@echo "Opening $(BLEND_FILE) in Blender..."
	$(BLENDER_PATH) $(BLEND_DIR)/$(BLEND_FILE)

# Quick test with lower samples (for faster iteration)
test-quick: $(TARGET) init-output
	@echo "=== Quick Test Renders (400x300, 8 samples) ==="
	@echo ""
	@echo "[1/2] Soft shadows comparison (Test6)..."
	./$(TARGET) -s $(SCENE_DIR)/Test6.txt -o $(OUTPUT_DIR)/quick_area_light.ppm -w 400 -H 300 --samples 8
	@echo "[2/2] Motion blur (Test5)..."
	./$(TARGET) -s $(SCENE_DIR)/Test5.txt -o $(OUTPUT_DIR)/quick_motion_blur.ppm -w 400 -H 300 --samples 8
	@echo ""
	@echo "=== Quick Test Complete ==="


test-1: init-output
	./raytracer --scene ASCII/Test1.txt \
		--output Output/rendered_bvh_Test1.ppm \
		--resolution 1920 1080 \
		--samples 16 \
		--soft-shadows 4 \
		--glossy-reflection 4 \
		--max-depth 5


test-2: init-output
	./raytracer --scene ASCII/Test2.txt \
		--output Output/rendered_bvh_Test2.ppm \
		--resolution 1920 1080 \
		--samples 16 \
		--soft-shadows 4 \
		--glossy-reflection 4 \
		--max-depth 5


test-3: init-output
	./raytracer --scene ASCII/Test3.txt \
		--output Output/rendered_bvh_Test3.ppm \
		--resolution 1920 1080 \
		--samples 4 \
		--soft-shadows 2 \
		--max-depth 2

		# --glossy-reflection 4 \

test-4: init-output
	./raytracer --scene ASCII/Test4.txt \
		--output Output/rendered_bvh_Test4.ppm \
		--resolution 1280 720 \
		--samples 16 \
		--soft-shadows 4 \
		--glossy-reflection 4 \
		--max-depth 5


test-5: init-output
	./raytracer --scene ASCII/Test5.txt \
		--output Output/rendered_bvh_Test5.ppm \
		--resolution 1920 1080 \
		--samples 4 \
		--soft-shadows 8 \
		--glossy-reflection 8 \
		--motion-blur 12 \
		--max-depth 64


test-6: init-output
	./raytracer --scene ASCII/Test6.txt \
		--output Output/rendered_bvh_Test6.ppm \
		--resolution 1920 1080 \
		--samples 16 \
		--soft-shadows 4 \
		--glossy-reflection 4 \
		--max-depth 5

test-7: init-output
	./raytracer --scene ASCII/Test7.txt \
		--output Output/rendered_bvh_Test7.ppm \
		--resolution 800 400 \
		--samples 128 \
		--soft-shadows 4 \
		--glossy-reflection 4 \
		--max-depth 12

test-all: test-1 test-2 test-3 test-4 test-5 test-6


# Ultra high quality for final report (1920x1080, 64 samples)
test-hq: $(TARGET) init-output
	@echo "=== Generating Ultra High Quality Test Renders ==="
	@echo "Resolution: 1920x1080, Samples: 64"
	@echo "WARNING: This will take a long time!"
	@echo ""
	@echo "[1/8] Rendering point light (hard shadows)..."
	./$(TARGET) -s $(SCENE_DIR)/test_point_light.txt -o $(OUTPUT_DIR)/hq_point_light.ppm -w 1920 -H 1080 --samples 64
	@echo "[2/8] Rendering area light (soft shadows)..."
	./$(TARGET) -s $(SCENE_DIR)/test_area_light.txt -o $(OUTPUT_DIR)/hq_area_light.ppm -w 1920 -H 1080 --samples 64
	@echo "[3/8] Rendering mirror reflection..."
	./$(TARGET) -s $(SCENE_DIR)/test_glossy_mirror.txt -o $(OUTPUT_DIR)/hq_glossy_mirror.ppm -w 1920 -H 1080 --samples 64
	@echo "[4/8] Rendering rough reflection..."
	./$(TARGET) -s $(SCENE_DIR)/test_glossy_rough.txt -o $(OUTPUT_DIR)/hq_glossy_rough.ppm -w 1920 -H 1080 --samples 64
	@echo "[5/8] Rendering glossy progression..."
	./$(TARGET) -s $(SCENE_DIR)/test_glossy.txt -o $(OUTPUT_DIR)/hq_glossy_progression.ppm -w 1920 -H 1080 --samples 64
	@echo "[6/8] Rendering pinhole camera..."
	./$(TARGET) -s $(SCENE_DIR)/test_dof_pinhole.txt -o $(OUTPUT_DIR)/hq_dof_pinhole.ppm -w 1920 -H 1080 --samples 64
	@echo "[7/8] Rendering with depth of field..."
	./$(TARGET) -s $(SCENE_DIR)/test_dof_mid_focus.txt -o $(OUTPUT_DIR)/hq_dof_blur.ppm -w 1920 -H 1080 --samples 64
	@echo "[8/8] Rendering motion blur..."
	./$(TARGET) -s $(SCENE_DIR)/test_motion_moving.txt -o $(OUTPUT_DIR)/hq_motion_blur.ppm -w 1920 -H 1080 --samples 64
	@echo ""
	@echo "=== Ultra High Quality Renders Complete ==="
	@echo "Output saved to: $(OUTPUT_DIR)/"

# View output images (macOS)
view-output:
	@echo "Opening output directory..."
	open $(OUTPUT_DIR)

# Show build information
info:
	@echo "=== Build Information ==="
	@echo "Compiler:        $(CXX)"
	@echo "Flags:           $(CXXFLAGS)"
	@echo "Target:          $(TARGET)"
	@echo "Build Dir:       $(BUILD_DIR)"
	@echo "Scene Dir:       $(SCENE_DIR)"
	@echo "Output Dir:      $(OUTPUT_DIR)"
	@echo "Source Files:    $(words $(SOURCES)) files"
	@echo "Header Files:    $(words $(HEADERS)) files"

# Convert PPM to PNG using sips
convert:
	@echo "Converting PPM to PNG..."
	@for ppm in $(OUTPUT_DIR)/*.ppm; do \
		sips -s format png "$$ppm" --out "$${ppm%.ppm}.png"; \
	done
	@echo "Conversion complete!"

# Help target
help:
	@echo "=== CGR Raytracer Makefile ==="
	@echo ""
	@echo "Build Targets:"
	@echo "  all              - Build the raytracer (default)"
	@echo "  debug            - Build with debug symbols"
	@echo "  clean            - Remove build artifacts"
	@echo ""
	@echo "Run Targets:"
	@echo "  run              - Build and run raytracer"
	@echo "  benchmark        - Run with timing information"
	@echo ""
	@echo "Module 4 Test Targets:"
	@echo "  test-quick       - Quick test renders (400x300, 8 spp) - ~2 min"
	@echo "  test-all         - High quality test renders (800x600, 32 spp) - ~10 min"
	@echo "  test-hq          - Ultra HQ for report (1920x1080, 64 spp) - ~1 hour"
	@echo ""
	@echo "Workflow Targets:"
	@echo "  workflow         - Full workflow: export → build → render"
	@echo "  export           - Export BLEND_FILE to ASCII (default: Test1.blend)"
	@echo "  export-all       - Export all .blend files in Blend/"
	@echo "  view             - Open BLEND_FILE in Blender GUI"
	@echo ""
	@echo "Utility Targets:"
	@echo "  check            - Run static code analysis"
	@echo "  format           - Format code with clang-format"
	@echo "  view-output      - Open output directory"
	@echo "  info             - Show build information"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build raytracer"
	@echo "  make test-quick         # Generate all test renders (fast)"
	@echo "  make test-all           # Generate high quality test renders"
	@echo "  make workflow           # Export, build, and render"
	@echo "  make export             # Export Test1.blend (default)"
	@echo "  make export BLEND_FILE=my.blend # Export specific file"
	@echo "  make clean all          # Clean rebuild"
	@echo "  make debug              # Debug build"

.PHONY: all debug clean run render workflow benchmark check format init-output view-output info help test-all test-quick test-hq export export-all view
