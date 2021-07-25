#include "static_mesh.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>



Vertex::Vertex(){};

Vertex::Vertex(float x, float y, float z) {Position = glm::vec3(x, y, z);}

bool  Vertex::operator==(const Vertex v) { return this == &v; }




StaticMesh::StaticMesh() {

}

StaticMesh::~StaticMesh() {
    //std::cout << "deleting mesh..." << std::endl;
}

std::vector<float> StaticMesh::getAABB() {
    if (limits.size()) {
        return limits;
    }
    std::cout << "StaticMesh::getAABB called" << std::endl;
    float minX = numeric_limits<float>::max();
    float maxX = numeric_limits<float>::min();
    float minY = numeric_limits<float>::max();
    float maxY = numeric_limits<float>::min();
    float minZ = numeric_limits<float>::max();
    float maxZ = numeric_limits<float>::min();
    for(unsigned int i = 0; i < vertices.size(); i++)
    {
        if (vertices[i].Position.x < minX) {
            minX = vertices[i].Position.x;
        } else if (vertices[i].Position.x > maxX) {
            maxX = vertices[i].Position.x;
        }
        if (vertices[i].Position.y < minY) {
            minY = vertices[i].Position.y;
        } else if (vertices[i].Position.y > maxY) {
            maxY = vertices[i].Position.y;
        }
        if (vertices[i].Position.z < minZ) {
            minZ = vertices[i].Position.z;
        } else if (vertices[i].Position.z > maxZ) {
            maxZ = vertices[i].Position.z;
        }
    }
    limits = {minX, minY, minZ, maxX, maxY, maxZ};
    return limits;
}

std::vector<std::vector<glm::vec3>> StaticMesh::getTriangles(glm::mat4 model) {


    std::vector<std::vector<glm::vec3>> triangles = std::vector<std::vector<glm::vec3>>();
    for (unsigned int i = 0; i < vertices.size();) {
        glm::vec3 v1 = glm::vec3(model * glm::vec4(vertices.at(i++).Position, 1.0));
        glm::vec3 v2 = glm::vec3(model * glm::vec4(vertices.at(i++).Position, 1.0));
        glm::vec3 v3 = glm::vec3(model * glm::vec4(vertices.at(i++).Position, 1.0));
        std::vector<glm::vec3> triangle = std::vector<glm::vec3>();
        triangle = {v1, v2, v3};
        triangles.push_back(triangle);
    }
    return triangles;
}
/// @todo a system to cache loaded models and use copy semantics to create duplicate objects
StaticMesh::StaticMesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures, const Material &mat)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    this->mats = mat;
    this->specularity = mat.Ks.x;
    // now that we have all the required data, set the vertex buffers and its attribute pointers.
    setupMesh();

}

StaticMesh::StaticMesh(const StaticMesh& other) {
    this->vertices = other.vertices;
    this->indices = other.indices;
    this->textures = other.textures;
    this->VAO = other.VAO;
    this->VBO = other.VBO;
    this->EBO = other.EBO;
    this->instanceBuffer = other.instanceBuffer;
    this->frustumCulled = other.frustumCulled;
    this->mats = other.mats;
    this->specularity = other.mats.Ks.x;
    this->filePath = other.filePath;
    this->modelID = other.modelID;
    this->toDraw = other.toDraw;
    this->boundingCube = other.boundingCube;

    //setupMesh();
}

StaticMesh& StaticMesh::operator=(StaticMesh other) {
    this->vertices = other.vertices;
    this->indices = other.indices;
    this->textures = other.textures;
    this->VAO = other.VAO;
    this->VBO = other.VBO;
    this->EBO = other.EBO;
    this->instanceBuffer = other.instanceBuffer;
    this->frustumCulled = other.frustumCulled;
    this->mats = other.mats;
    this->specularity = other.mats.Ks.x;
    this->filePath = other.filePath;
    this->modelID = other.modelID;
    this->toDraw = other.toDraw;
    this->boundingCube = other.boundingCube;
    return *this;
}

// render the mesh
int StaticMesh::Draw(StaticShader shader)
{
    // bind appropriate textures
    unsigned int diffuseNr  = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr   = 1;
    unsigned int heightNr   = 1;
    for(unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
        // retrieve texture number (the N in diffuse_textureN)
        string number;
        string name = textures[i].type;
        if(name == "texture_diffuse")
    number = std::to_string(diffuseNr++);
  else if(name == "texture_specular")
    number = std::to_string(specularNr++); // transfer unsigned int to stream
        else if(name == "texture_normal")
    number = std::to_string(normalNr++); // transfer unsigned int to stream
         else if(name == "texture_height")
      number = std::to_string(heightNr++); // transfer unsigned int to stream

                       // now set the sampler to the correct texture unit
        glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
        // and finally bind the texture
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    // draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);

    return 0;
}


void StaticMesh::DrawInstanced(StaticShader shader, std::vector<mat4> modelMatrices)
{
    // bind appropriate textures
    unsigned int diffuseNr  = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr   = 1;
    unsigned int heightNr   = 1;
    for(unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
        // retrieve texture number (the N in diffuse_textureN)
        string number;
        string name = textures[i].type;
        if(name == "texture_diffuse")
    number = std::to_string(diffuseNr++);
  else if(name == "texture_specular")
    number = std::to_string(specularNr++); // transfer unsigned int to stream
        else if(name == "texture_normal")
    number = std::to_string(normalNr++); // transfer unsigned int to stream
         else if(name == "texture_height")
      number = std::to_string(heightNr++); // transfer unsigned int to stream

                       // now set the sampler to the correct texture unit
        glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
        // and finally bind the texture
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }
    shader.setFloat("spec", specularity);
    // draw mesh
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, modelMatrices.size() * sizeof(glm::mat4), &modelMatrices.front(), GL_STATIC_DRAW);
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, modelMatrices.size());
    glBindVertexArray(0);

    // always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
}

void StaticMesh::setupMesh()
{   
    // create buffers/arrays
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &instanceBuffer);
  


    glBindVertexArray(VAO);
    // load data into vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // A great thing about structs is that their memory layout is sequential for all its items.
    // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a vec3/2 array which
    // again translates to 3/2 floats which translates to a byte array.
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    //glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
    //glBufferData(GL_ARRAY_BUFFER, modelMatrices.size() * sizeof(glm::mat4), modelMatrices.data(), GL_STATIC_DRAW);

    // set the vertex attribute pointers
    // vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    // vertex tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    // vertex bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    std::size_t vec4Size = sizeof(glm::vec4);
    glEnableVertexAttribArray(5); 
    glEnableVertexAttribArray(6); 
    glEnableVertexAttribArray(7);
    glEnableVertexAttribArray(8); 
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
    glVertexAttribDivisor(8, 1);

    glBindVertexArray(0);
}