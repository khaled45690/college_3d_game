#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Material {
    glm::vec3 ambientColor  = glm::vec3(0.2f);
    glm::vec3 diffuseColor  = glm::vec3(0.8f);
    glm::vec3 specularColor = glm::vec3(0.2f);
    float     shininess     = 16.0f;
    unsigned int diffuseTexture = 0;  // GL texture id, 0 = none
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::string name;
    std::string materialName;
    Material material;
};

class Model {
public:
    Model();
    ~Model();

    bool load(const std::string& filepath);
    void render();
    void setPosition(const glm::vec3& pos);
    void setScale(const glm::vec3& scale);
    void setRotation(const glm::vec3& rotation);

    glm::vec3 getPosition() const { return position; }
    glm::vec3 getScale() const { return scale; }
    glm::vec3 getRotation() const { return rotation; }

private:
    std::vector<Mesh> meshes;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;

    // Bounding box computed at load time — used for normalisation in render()
    glm::vec3 bbMin, bbMax, bbCenter;
    float maxDim;

    void renderMesh(const Mesh& mesh);
};

#endif // MODEL_H
