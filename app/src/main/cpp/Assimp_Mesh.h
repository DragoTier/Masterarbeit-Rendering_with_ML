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

struct VertexData {
    glm::vec3 Position;
    glm::vec2 TexCoord;

};

/**
 * Class for handling each Mesh on the GPU with OpenGL
 */
class Assimp_Mesh {
public:
    Assimp_Mesh(std::vector<VertexData> &vertexData, std::vector<GLuint> &indices);
    void Draw_with_shader(GLuint &shader);

private:
	std::vector<VertexData> vertexData;
	std::vector<GLuint> indices;

    GLuint VAO;
	GLuint VBO;
	GLuint EBO;
};
} //namespace

#endif //MASTERTHESIS_AIUPSCALE_ASSIMP_MESH_H
