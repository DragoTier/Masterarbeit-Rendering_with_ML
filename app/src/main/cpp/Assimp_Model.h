//
// Created by ivmu6294 on 20.04.2022.
//

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

class Assimp_Model {
public:
    Assimp_Model(const void* model_data, size_t model_size);
    void Draw(GLuint &shader);

private:
    std::vector<Assimp_Mesh> meshes;
    std::string directory;

    void loadModel(const void* model_data, size_t model_size);
    void processNode(aiNode *node, const aiScene *scene);
    Assimp_Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    //std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};
} //

#endif //MASTERTHESIS_AIUPSCALE_ASSIMP_MODEL_H

