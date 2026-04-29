#include "Scene.h"
#include <iostream>

Scene::Scene() : totalTime(0.0f) {
}

Scene::~Scene() {
    objects.clear();
}

void Scene::addModel(const std::string& name, const std::string& filepath,
                     const glm::vec3& position,
                     const glm::vec3& scale,
                     const glm::vec3& rotation) {
    auto model = std::make_shared<Model>();
    if (!model->load(filepath)) {
        std::cerr << "Failed to load model: " << filepath << std::endl;
        return;
    }

    SceneObject obj;
    obj.name = name;
    obj.model = model;
    obj.position = position;
    obj.scale = scale;
    obj.rotation = rotation;

    // Apply initial transform
    obj.model->setPosition(position);
    obj.model->setScale(scale);
    obj.model->setRotation(rotation);

    objects.push_back(obj);
    std::cout << "Scene: Added object '" << name << "' at position ("
              << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
}

void Scene::render() {
    road.render();

    for (auto& obj : assimpObjects)
        obj.model->render();

    for (auto& obj : objects) {
        obj.model->setPosition(obj.position);
        obj.model->setScale(obj.scale);
        obj.model->setRotation(obj.rotation);
        obj.model->render();
    }
}

void Scene::update(float deltaTime) {
    totalTime += deltaTime;

    // Update dynamic objects here if needed
    // For now, just track time for animations
}

void Scene::addAssimpModel(const std::string& name, const std::string& filepath,
                            const glm::vec3& position,
                            const glm::vec3& scale,
                            const glm::vec3& rotation) {
    auto model = std::make_shared<AssimpModel>();
    if (!model->load(filepath)) {
        std::cerr << "Failed to load assimp model: " << filepath << std::endl;
        return;
    }
    model->setPosition(position);
    model->setScale(scale);
    model->setRotation(rotation);
    AssimpObject obj;
    obj.name = name;
    obj.model = model;
    assimpObjects.push_back(obj);
    std::cout << "Scene: Added assimp object '" << name << "'" << std::endl;
}

SceneObject* Scene::getObject(const std::string& name) {
    for (auto& obj : objects) {
        if (obj.name == name) {
            return &obj;
        }
    }
    return nullptr;
}

Model* Scene::getModel(const std::string& name) {
    for (auto& obj : objects) {
        if (obj.name == name) {
            return obj.model.get();
        }
    }
    return nullptr;
}
