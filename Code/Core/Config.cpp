#include "Config.h"

// Global configuration instance - initialized with default values
// Note: tone_mapping_mode is set to "none" by default in Config.h
// to match Blender's output which already has color management applied
RenderConfig g_config;
