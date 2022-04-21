//
// Created by ivmu6294 on 14.04.2022.
//

#ifndef MASTERTHESIS_AIUPSCALE_ASSIMP_MESH_H
#define MASTERTHESIS_AIUPSCALE_ASSIMP_MESH_H

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


namespace AssimpModelLoader{

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;

};

struct Texture {
    GLuint id;
    char* type;
};

class Assimp_Mesh {
public:
    std::vector<Vertex>     vertices;
    std::vector<GLuint>     indices;
    std::vector<Texture>    textures;

    Assimp_Mesh(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, std::vector<Texture> &textures);
    void Draw(GLuint &shader);
private:
    GLuint VAO, VBO, EBO;

    void setupMesh();
};
} //namespace

#endif //MASTERTHESIS_AIUPSCALE_ASSIMP_MESH_H
