#ifndef MASTERTHESIS_AIUPSCALE_ASSIMP_MODEL_H
#define MASTERTHESIS_AIUPSCALE_ASSIMP_MODEL_H
#include <map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#include <cstdio>
#include <cstdlib>
#include <glm/glm.hpp>
#include <vector>

#include "Assimp_Mesh.h"

namespace AssimpModelLoader {

/**
 * Simple Model class for handling meshes with assimp
 */
class Assimp_Model {
public:
    Assimp_Model(const void* model_data, size_t model_size);
	void load_from_buffer(const void* model_data, size_t model_size);
	void DrawElements(GLuint &shader);

private:
    std::vector<Assimp_Mesh> meshes;

};
} //namespace

#endif //MASTERTHESIS_AIUPSCALE_ASSIMP_MODEL_H

