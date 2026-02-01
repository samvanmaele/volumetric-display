#pragma once
#include <tiny_gltf.h>
#include <glm/glm.hpp>

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
	#include <emscripten/html5.h>
	#include <GLES3/gl3.h>
#else
	#include <GL/glew.h>
#endif

struct GpuTriangle {
    glm::vec4 v0;
    glm::vec4 v1;
    glm::vec4 v2;
};

class GlModel
{
	public:
		std::vector<GpuTriangle> triangles;
		glm::vec3 globalMin = glm::vec3( std::numeric_limits<float>::max());;
		glm::vec3 globalMax = glm::vec3(-std::numeric_limits<float>::max());;

		GlModel(const char* filename);
	private:
		void processNode(tinygltf::Model& model, const tinygltf::Node& node, glm::mat4 parentTransform);
		void extractPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const glm::mat4& transform);
};