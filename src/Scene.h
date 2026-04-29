#ifndef SCENE_H
#define SCENE_H

#include "Model.h"
#include "Road.h"
#include "AssimpModel.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

struct SceneObject {
    std::shared_ptr<Model> model;
    std::string name;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;
};

class Scene {
public:
    Scene();
    ~Scene();

    void addModel(const std::string& name, const std::string& filepath,
                  const glm::vec3& position = glm::vec3(0, 0, 0),
                  const glm::vec3& scale = glm::vec3(1, 1, 1),
                  const glm::vec3& rotation = glm::vec3(0, 0, 0));

    void addAssimpModel(const std::string& name, const std::string& filepath,
                        const glm::vec3& position = glm::vec3(0,0,0),
                        const glm::vec3& scale    = glm::vec3(1,1,1),
                        const glm::vec3& rotation = glm::vec3(0,0,0));

    void render();
    void update(float deltaTime);

    SceneObject* getObject(const std::string& name);
    Model* getModel(const std::string& name);

private:
    struct AssimpObject {
        std::shared_ptr<AssimpModel> model;
        std::string name;
    };

    std::vector<SceneObject>   objects;
    std::vector<AssimpObject>  assimpObjects;
    Road road;
    float totalTime;
};

#endif // SCENE_H
