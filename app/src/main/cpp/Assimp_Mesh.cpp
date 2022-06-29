//
// Created by ivmu6294 on 14.04.2022.
//

#include "Assimp_Mesh.h"
#include <string>

namespace AssimpModelLoader{

Assimp_Mesh::Assimp_Mesh(std::vector<VertexData> &vertexData, std::vector<GLuint> &indices) {
    this->vertexData = vertexData;
    this->indices = indices;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    //set data
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(VertexData), &vertexData[0], GL_STATIC_DRAW);

    //set indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),&indices[0], GL_STATIC_DRAW);

    // vertices
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);

    // texture coords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, TexCoord));

    glBindVertexArray(0);
}

void Assimp_Mesh::Draw_with_shader(GLuint &shader) {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLuint>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
} //namespace