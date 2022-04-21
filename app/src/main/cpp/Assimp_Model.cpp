//
// Created by ivmu6294 on 20.04.2022.
//

#include "Assimp_Model.h"

namespace AssimpModelLoader{


Assimp_Model::Assimp_Model(const void* model_data, size_t model_size) {
    loadModel(model_data, model_size);
}

void Assimp_Model::Draw(GLuint &shader){
    for(unsigned int i = 0; i < meshes.size(); i++){
        meshes[i].Draw(shader);
    }
}

void Assimp_Model::loadModel(const void* model_data, size_t model_size) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(model_data, model_size, aiProcess_Triangulate |aiProcess_FlipUVs, "obj");
    //const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate |aiProcess_FlipUVs);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        return;
    }
    //directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);
}

void Assimp_Model::processNode(aiNode *node, const aiScene *scene) {
    for(unsigned int i = 0; i < node->mNumMeshes; i++){
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    for(unsigned int i = 0; i < node->mNumChildren; i++){
        processNode(node->mChildren[i], scene);
    }
}


Assimp_Mesh Assimp_Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

    for(unsigned int i = 0; i < mesh->mNumVertices; i++){
        Vertex vertex;
        glm::vec3 vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        if(mesh->mNormals){
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        } else {
            vertex.Normal = glm::vec3(0.0f);
        }

        if(mesh->mTextureCoords[0]){
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

 /*   if(mesh->mMaterialIndex >= 0)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material,aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<Texture> specularMaps = loadMaterialTextures(material,aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    } */

    return Assimp_Mesh(vertices, indices, textures);
}





} //namespace

