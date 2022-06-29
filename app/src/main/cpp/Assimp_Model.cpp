#include "Assimp_Model.h"

namespace AssimpModelLoader{


Assimp_Model::Assimp_Model(const void* model_data, size_t model_size) {
    load_from_buffer(model_data, model_size);
}

void Assimp_Model::DrawElements(GLuint &shader){
    for(unsigned int i = 0; i < meshes.size(); i++){
        meshes[i].Draw_with_shader(shader);
    }
}

void Assimp_Model::load_from_buffer(const void* model_data, size_t model_size) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(model_data, model_size, aiProcess_Triangulate |aiProcess_FlipUVs, "obj");

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        return;
    }
    //skip root node. Nested children won't be processed.
    aiNode* node = scene->mRootNode->mChildren[0];

    for(int i = 0; i < node->mNumMeshes; i++){
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        std::vector<VertexData> vertex_data;
        std::vector<GLuint> indices;


        //add all vertices on the mesh and a texCoord for each
        for(int j = 0; j < mesh->mNumVertices; j++){
            VertexData vertex;
            glm::vec3 position;
            position.x = mesh->mVertices[j].x;
            position.y = mesh->mVertices[j].y;
            position.z = mesh->mVertices[j].z;
            vertex.Position = position;

            if(mesh->mTextureCoords[0]){
                glm::vec2 texCoord;
                texCoord.x = mesh->mTextureCoords[0][j].x;
                texCoord.y = mesh->mTextureCoords[0][j].y;
                vertex.TexCoord = texCoord;
            } else {
                vertex.TexCoord = glm::vec2(0.0f, 0.0f);
            }
            vertex_data.push_back(vertex);
        }

        //add indices for EBO
        for(unsigned int j = 0; j < mesh->mNumFaces; j++)
        {
            aiFace face = mesh->mFaces[j];
            for(unsigned int k = 0; k < face.mNumIndices; k++){
                indices.push_back(face.mIndices[k]);
            }
        }
        meshes.push_back(Assimp_Mesh(vertex_data, indices));
    }
}

} //namespace

