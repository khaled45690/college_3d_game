#ifndef SCENE_H
#define SCENE_H

#include "Model.h"
#include "Road.h"
#include "AssimpModel.h"
#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>

struct SceneObject {
    std::shared_ptr<Model> model;
    std::string name;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;
};

enum GameResult { GAME_PLAYING, GAME_WON, GAME_LOST };

class Scene {
public:
    Scene();
    ~Scene();

    void addModel(const std::string& name, const std::string& filepath,
                  const glm::vec3& position = glm::vec3(0,0,0),
                  const glm::vec3& scale    = glm::vec3(1,1,1),
                  const glm::vec3& rotation = glm::vec3(0,0,0));

    void addAssimpModel(const std::string& name, const std::string& filepath,
                        const glm::vec3& position = glm::vec3(0,0,0),
                        const glm::vec3& scale    = glm::vec3(1,1,1),
                        const glm::vec3& rotation = glm::vec3(0,0,0));

    void render();
    void update(float dt);

    // Game setup
    void initEasyMode();
    void initHardMode();

    // Player controls
    void moveLeft();
    void moveRight();
    void jump();

    // State queries
    GameResult getResult() const { return gameResult; }
    float      getCharX()  const { return charX; }
    float      getCharZ()  const { return charZ; }

    // Load an extra animation clip onto an already-added AssimpModel.
    // Call after addAssimpModel(). clipName is used with setAnimation().
    void loadAssimpAnimation(const std::string& modelName,
                              const std::string& fbxPath,
                              const std::string& clipName);

    SceneObject*  getObject(const std::string& name);
    Model*        getModel(const std::string& name);
    AssimpModel*  getAssimpModel(const std::string& name);

private:
    struct Obstacle {
        enum Type { BLOCKER, CAR } type;
        float x, z;
        bool  passed;
    };

    struct AssimpObject {
        std::shared_ptr<AssimpModel> model;
        std::string name;
    };

    std::vector<SceneObject>  objects;
    std::vector<AssimpObject> assimpObjects;
    Road  road;
    float totalTime;

    // ── game state ────────────────────────────────────────────────────────
    GameResult gameResult;
    float charX, charY, charZ;
    float charTargetX;
    float charVelY;
    bool  isJumping;
    int   charLane;

    std::vector<Obstacle>  obstacles;
    std::shared_ptr<Model> blockerModel;
    std::shared_ptr<Model> carModel;
    float obstacleSpeed;
    int   totalObstacles;

    static constexpr float LANE_X[3]       = { -9.0f, 0.0f, 9.0f };
    static constexpr float ROAD_Y          = 0.0f;
    static constexpr float CHAR_Z          = 18.0f;
    static constexpr float GRAVITY         = -36.0f;
    static constexpr float JUMP_VEL        = 30.0f;  // 2× original — doubles height and air time
    static constexpr float BLOCKER_CLEAR_Y = 1.00f;
    static constexpr float LANE_SPEED      = 45.0f;  // units/sec for smooth lane slide

    void initMode(float speed, int numObstacles);
    void updateCharacter(float dt);
    void updateObstacles(float dt);
    void checkCollisions();
    void renderObstacles();
    void syncCharacterModel();
    void updateCharacterAnimation();
};

#endif // SCENE_H
