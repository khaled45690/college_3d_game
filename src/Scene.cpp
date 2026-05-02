#include "Scene.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>

constexpr float Scene::LANE_X[3];

Scene::Scene()
    : totalTime(0.0f),
      gameResult(GAME_PLAYING),
      charX(0.0f), charY(ROAD_Y), charZ(CHAR_Z),
      charTargetX(0.0f),
      charVelY(0.0f), isJumping(false), charLane(1),
      obstacleSpeed(0.0f), totalObstacles(0)
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

Scene::~Scene() {
    obstacles.clear();
    assimpObjects.clear();
    objects.clear();
    blockerModel.reset();
    carModel.reset();
}

// ── model loading ─────────────────────────────────────────────────────────────

void Scene::addModel(const std::string& name, const std::string& filepath,
                     const glm::vec3& position, const glm::vec3& scale,
                     const glm::vec3& rotation)
{
    auto model = std::make_shared<Model>();
    if (!model->load(filepath)) {
        std::cerr << "Failed to load model: " << filepath << std::endl;
        return;
    }
    SceneObject obj;
    obj.name     = name;
    obj.model    = model;
    obj.position = position;
    obj.scale    = scale;
    obj.rotation = rotation;
    obj.model->setPosition(position);
    obj.model->setScale(scale);
    obj.model->setRotation(rotation);
    objects.push_back(obj);
    std::cout << "Scene: Added '" << name << "' at ("
              << position.x << ", " << position.y << ", " << position.z << ")\n";
}

void Scene::addAssimpModel(const std::string& name, const std::string& filepath,
                            const glm::vec3& position, const glm::vec3& scale,
                            const glm::vec3& rotation)
{
    auto model = std::make_shared<AssimpModel>();
    if (!model->load(filepath)) {
        std::cerr << "Failed to load assimp model: " << filepath << std::endl;
        return;
    }
    model->setPosition(position);
    model->setScale(scale);
    model->setRotation(rotation);
    AssimpObject obj;
    obj.name  = name;
    obj.model = model;
    assimpObjects.push_back(obj);
    std::cout << "Scene: Added assimp '" << name << "'\n";
}

// ── mode init ─────────────────────────────────────────────────────────────────

void Scene::initMode(float speed, int numObstacles)
{
    charLane    = 1;
    charX       = LANE_X[1];
    charTargetX = LANE_X[1];
    charY       = ROAD_Y;
    charZ       = CHAR_Z;
    charVelY    = 0.0f;
    isJumping   = false;
    gameResult     = GAME_PLAYING;
    totalTime      = 0.0f;
    obstacleSpeed  = speed;
    totalObstacles = numObstacles;

    if (!blockerModel) {
        blockerModel = std::make_shared<Model>();
        if (!blockerModel->load("assets/models/RoadBlockade_02.obj"))
            std::cerr << "Scene: Failed to load blocker model\n";
    }
    if (!carModel) {
        carModel = std::make_shared<Model>();
        if (!carModel->load("assets/models/Golf.obj"))
            std::cerr << "Scene: Failed to load car model\n";
    }

    obstacles.clear();
    const float SPACING = 45.0f;
    float nextZ = -45.0f;
    for (int i = 0; i < totalObstacles; ++i) {
        Obstacle obs;
        obs.type   = (std::rand() % 3 == 0) ? Obstacle::CAR : Obstacle::BLOCKER;
        obs.x      = LANE_X[std::rand() % 3];
        obs.z      = nextZ;
        obs.passed = false;
        obstacles.push_back(obs);
        nextZ -= SPACING;
    }

    syncCharacterModel();
    if (AssimpModel* ch = getAssimpModel("character"))
        ch->setAnimation("run");

    std::cout << "Mode started: " << totalObstacles << " obstacles, speed="
              << obstacleSpeed << " m/s\n";
}

void Scene::initEasyMode() {
    initMode(15.0f, 8);
}

void Scene::initHardMode() {
    initMode(30.0f, 16);
}

// ── player controls ───────────────────────────────────────────────────────────

void Scene::moveLeft() {
    if (gameResult != GAME_PLAYING) return;
    if (charLane > 0) {
        --charLane;
        charTargetX = LANE_X[charLane];
    }
}

void Scene::moveRight() {
    if (gameResult != GAME_PLAYING) return;
    if (charLane < 2) {
        ++charLane;
        charTargetX = LANE_X[charLane];
    }
}

void Scene::jump() {
    if (gameResult != GAME_PLAYING) return;
    if (!isJumping) {
        isJumping = true;
        charVelY  = JUMP_VEL;
    }
}

// ── update ────────────────────────────────────────────────────────────────────

void Scene::update(float dt) {
    if (gameResult != GAME_PLAYING) return;
    totalTime += dt;
    updateCharacter(dt);
    updateObstacles(dt);
    checkCollisions();
    updateCharacterAnimation();
    // advance animation on all assimp models
    for (auto& obj : assimpObjects)
        obj.model->update(dt);
}

void Scene::updateCharacter(float dt) {
    // Smooth horizontal slide toward target lane
    float dx = charTargetX - charX;
    float step = LANE_SPEED * dt;
    if (std::fabs(dx) <= step)
        charX = charTargetX;
    else
        charX += (dx > 0.0f ? step : -step);

    if (isJumping) {
        charVelY += GRAVITY * dt;
        charY    += charVelY * dt;
        if (charY <= ROAD_Y) {
            charY     = ROAD_Y;
            charVelY  = 0.0f;
            isJumping = false;
        }
    }
    charZ -= obstacleSpeed * dt;
    syncCharacterModel();
}

void Scene::updateObstacles(float dt) {
    int passedCount = 0;
    for (auto& obs : obstacles) {
        // Obstacles are fixed; mark passed when character runs 5m past them
        if (!obs.passed && charZ < obs.z - 15.0f)
            obs.passed = true;
        if (obs.passed) ++passedCount;
    }
    if (passedCount == totalObstacles)
        gameResult = GAME_WON;
}

void Scene::checkCollisions() {
    for (const auto& obs : obstacles) {
        if (obs.passed) continue;

        // Z overlap
        float dz = obs.z - charZ;
        if (dz < -10.5f || dz > 10.5f) continue;

        // X overlap (generous hitbox for fair gameplay)
        if (std::fabs(obs.x - charX) > 2.4f) continue;

        // BLOCKER: safe if the character has jumped high enough
        if (obs.type == Obstacle::BLOCKER && charY >= BLOCKER_CLEAR_Y) continue;

        // Collision!
        gameResult = GAME_LOST;
        return;
    }
}

void Scene::syncCharacterModel() {
    for (auto& obj : assimpObjects) {
        if (obj.name == "character") {
            obj.model->setPosition(glm::vec3(charX, charY, charZ));
            return;
        }
    }
}

// ── render ────────────────────────────────────────────────────────────────────

void Scene::render() {
    road.render();
    renderObstacles();

    for (auto& obj : assimpObjects)
        obj.model->render();

    for (auto& obj : objects) {
        obj.model->setPosition(obj.position);
        obj.model->setScale(obj.scale);
        obj.model->setRotation(obj.rotation);
        obj.model->render();
    }
}

void Scene::renderObstacles() {
    if (!blockerModel && !carModel) return;

    const glm::vec3 blockerScale(9.0f, 9.0f, 9.0f);
    const glm::vec3 carScale(12.0f, 12.0f, 12.0f);
    const glm::vec3 blockerRot(0.0f, 0.0f, 0.0f);
    const glm::vec3 carRot(0.0f, 180.0f, 0.0f);

    for (const auto& obs : obstacles) {
        if (obs.passed) continue;

        if (obs.type == Obstacle::BLOCKER && blockerModel) {
            blockerModel->setPosition(glm::vec3(obs.x, 0.0f, obs.z));
            blockerModel->setScale(blockerScale);
            blockerModel->setRotation(blockerRot);
            blockerModel->render();
        } else if (obs.type == Obstacle::CAR && carModel) {
            carModel->setPosition(glm::vec3(obs.x, 0.0f, obs.z));
            carModel->setScale(carScale);
            carModel->setRotation(carRot);
            carModel->render();
        }
    }
}

// ── animation helpers ─────────────────────────────────────────────────────────

void Scene::loadAssimpAnimation(const std::string& modelName,
                                 const std::string& fbxPath,
                                 const std::string& clipName) {
    for (auto& obj : assimpObjects) {
        if (obj.name == modelName) {
            obj.model->loadAnimation(fbxPath, clipName);
            return;
        }
    }
    std::cerr << "Scene::loadAssimpAnimation: model '" << modelName
              << "' not found\n";
}

void Scene::updateCharacterAnimation() {
    AssimpModel* ch = getAssimpModel("character");
    if (!ch) return;
    if (isJumping)
        ch->setAnimation("jump");
    else
        ch->setAnimation("run");
}

// ── legacy helpers ────────────────────────────────────────────────────────────

SceneObject* Scene::getObject(const std::string& name) {
    for (auto& obj : objects)
        if (obj.name == name) return &obj;
    return nullptr;
}

Model* Scene::getModel(const std::string& name) {
    for (auto& obj : objects)
        if (obj.name == name) return obj.model.get();
    return nullptr;
}

AssimpModel* Scene::getAssimpModel(const std::string& name) {
    for (auto& obj : assimpObjects)
        if (obj.name == name) return obj.model.get();
    return nullptr;
}
