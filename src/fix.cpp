#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// 1. Tell tinygltf NOT to try and include "json.hpp" itself
#define TINYGLTF_NO_INCLUDE_JSON

// 2. Include the correct path for Nlohmann JSON
#include <nlohmann/json.hpp>

// 3. Now include tinygltf
#include <tiny_gltf.h>