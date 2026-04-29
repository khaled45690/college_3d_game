#include <GL/glut.h>
#include <iostream>
#include <glm/glm.hpp>
#include "Model.h"
#include "Menu.h"
#include "Scene.h"

// Window dimensions
int windowWidth = 800;
int windowHeight = 600;

// Camera (distances in metres)
float cameraDistance = 15.0f;
float cameraRotX = 20.0f;
float cameraRotY = 0.0f;
Model* g_model = nullptr;
float modelRotation = 0.0f;

// Menu
Menu* g_menu = nullptr;
bool gameStarted = false;

// Game Scene
Scene* g_scene = nullptr;

// Forward declarations
void renderHUD();

// Display callback
void display() {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Check if menu should be shown
    if (g_menu->getState() != GAME_RUNNING) {
        g_menu->render();
    } else {
        // Game is running - render 3D scene with car model

        // Set up projection matrix
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        // near=0.5m, far=800m — covers procedural road length
        gluPerspective(50.0f, (float)windowWidth / (float)windowHeight, 0.5f, 800.0f);

        // Set up model view matrix with camera
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Orbit around a point at road level (roadY) so camera always stays above the road
        const float roadY = 0.0f;
        float camX = cameraDistance * sinf(cameraRotY * 3.14159f / 180.0f) * cosf(cameraRotX * 3.14159f / 180.0f);
        float camY = cameraDistance * sinf(cameraRotX * 3.14159f / 180.0f) + roadY;
        float camZ = cameraDistance * cosf(cameraRotY * 3.14159f / 180.0f) * cosf(cameraRotX * 3.14159f / 180.0f);

        gluLookAt(camX, camY, camZ,
                  0.0f, roadY + 4.0f, 0.0f,  // look at car-roof height above road
                  0.0f, 1.0f, 0.0f);

        // Render the scene (environment, obstacles, cars, etc.)
        if (g_scene) {
            g_scene->render();
        }

        // Render HUD (2D overlay with instructions)
        renderHUD();
    }

    // Swap buffers
    glutSwapBuffers();
}

// Reshape callback
void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
}

// Render HUD (2D overlay with instructions)
void renderHUD() {
    // Set up 2D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Disable depth testing for 2D rendering
    glDisable(GL_DEPTH_TEST);

    glColor3f(1.0f, 1.0f, 1.0f);  // White text

    // Left side instructions
    glRasterPos2f(10.0f, 50.0f);
    std::string left_label = "LEFT ARROW / A";
    for (char c : left_label) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    glRasterPos2f(10.0f, 80.0f);
    std::string left_desc = "Move Left";
    for (char c : left_desc) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    // Right side instructions
    glRasterPos2f(windowWidth - 200.0f, 50.0f);
    std::string right_label = "RIGHT ARROW / D";
    for (char c : right_label) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    glRasterPos2f(windowWidth - 200.0f, 80.0f);
    std::string right_desc = "Move Right";
    for (char c : right_desc) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    // Center - Jump instruction
    glRasterPos2f(windowWidth / 2.0f - 80.0f, windowHeight - 50.0f);
    std::string jump_label = "SPACE BAR - Jump";
    for (char c : jump_label) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    // Re-enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Restore projection
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// Idle callback (for animation)
void idle() {
    glutPostRedisplay();
}

// Keyboard callback
void keyboard(unsigned char key, int x, int y) {
    // If menu is active, handle menu input
    if (g_menu->getState() != GAME_RUNNING) {
        g_menu->handleKeypress(key);
        glutPostRedisplay();
        return;
    }

    // Otherwise, handle game input
    switch (key) {
        case 27: // ESC key
            exit(0);
            break;
        case 'r': // Reset camera
        case 'R':
            cameraRotX = 20.0f;
            cameraRotY = 0.0f;
            cameraDistance = 15.0f;
            break;
        case 'w':  // Zoom in
        case 'W':
            cameraDistance -= 10.0f;
            if (cameraDistance < 20.0f) cameraDistance = 20.0f;
            break;
        case 's':  // Zoom out
        case 'S':
            cameraDistance += 10.0f;
            if (cameraDistance > 2000.0f) cameraDistance = 2000.0f;
            break;
        case '8':  // Move forward (zoom in)
            cameraDistance -= 10.0f;
            if (cameraDistance < 5.0f) cameraDistance = 5.0f;
            break;
        case '5':  // Move backward (zoom out)
            cameraDistance += 10.0f;
            if (cameraDistance > 2000.0f) cameraDistance = 2000.0f;
            break;
        case '4':  // Rotate camera left
            cameraRotY -= 4.0f;
            break;
        case '6':  // Rotate camera right
            cameraRotY += 4.0f;
            break;
        case '2':  // Look down
            cameraRotX -= 3.0f;
            if (cameraRotX < -89.0f) cameraRotX = -89.0f;
            break;
        case '9':  // Look up / tilt up
            cameraRotX += 3.0f;
            if (cameraRotX > 89.0f) cameraRotX = 89.0f;
            break;
    }
    glutPostRedisplay();
}

// Special keyboard callback (arrow keys, function keys, etc.)
void special(int key, int x, int y) {
    // If menu is active, handle menu input
    if (g_menu->getState() != GAME_RUNNING) {
        g_menu->handleSpecialKeypress(key);
        glutPostRedisplay();
        return;
    }
}

// Mouse motion callback
void motion(int x, int y) {
    // Not implemented yet, but can be used for camera control
}

// Initialize OpenGL
void init() {
    glClearColor(0.45f, 0.60f, 0.75f, 1.0f);  // sky blue
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);  // keeps normals correct after glScalef

    // Lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Key light: warm, upper-left-front (directional, w=0)
    GLfloat lightPos[] = {  5.0f, 10.0f,  8.0f, 0.0f };
    GLfloat lightDif[] = {  0.90f, 0.88f, 0.82f, 1.0f };
    GLfloat lightAmb[] = {  0.20f, 0.20f, 0.25f, 1.0f };
    GLfloat lightSpc[] = {  0.60f, 0.60f, 0.60f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDif);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmb);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpc);

    // Soft fill light: cool, opposite side
    glEnable(GL_LIGHT1);
    GLfloat fillPos[] = { -4.0f, 3.0f, -5.0f, 0.0f };
    GLfloat fillDif[] = {  0.25f, 0.28f, 0.38f, 1.0f };
    GLfloat fillAmb[] = {  0.0f,  0.0f,  0.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, fillPos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  fillDif);
    glLightfv(GL_LIGHT1, GL_AMBIENT,  fillAmb);
    glLightfv(GL_LIGHT1, GL_SPECULAR, fillAmb);

    GLfloat globalAmb[] = { 0.20f, 0.20f, 0.22f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
}

// Main entry point
int main(int argc, char** argv) {
    std::cout << "Starting Pepsi Man 3D - With Menu System" << std::endl;

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);

    // Create window
    int window = glutCreateWindow("Pepsi Man 3D");
    std::cout << "Window created: " << window << std::endl;

    // Initialize OpenGL
    init();

    // Create and load test model (for menu preview)
    g_model = new Model();
    if (!g_model->load("assets/models/cube.obj")) {
        std::cerr << "Failed to load test model!" << std::endl;
        return 1;
    }

    // Create and populate game scene
    g_scene = new Scene();

    // All scales below are in METRES (bounding-box normalisation converts each
    // model to a unit cube first, so scale = desired real-world size in metres).
    std::cout << "Loading game scene..." << std::endl;

    // Road is drawn procedurally by Scene::road — no model needed.
    // Road surface sits at y=0; small Y lift keeps cars above the surface.
    const float roadY = 0.2f;
    const float sidewalkY = 0.18f;  // sidewalk top (CURB_H)

    // ── Player car (center lane, near camera) ─────────────────────────────
    g_scene->addModel("player_car", "assets/models/old_car.obj",
                      glm::vec3(0, roadY, 8.0f),
                      glm::vec3(4.0f, 4.0f, 4.0f),
                      glm::vec3(0, 0, 0));

    // ── Roadblocks (just ahead, blocking left and right lanes) ────────────
    g_scene->addModel("roadblock_1", "assets/models/RoadBlockade_02.obj",
                      glm::vec3(-3.0f, roadY, 2.0f),
                      glm::vec3(2.0f, 2.0f, 2.0f),
                      glm::vec3(0, 0, 0));

    g_scene->addModel("roadblock_2", "assets/models/RoadBlockade_02.obj",
                      glm::vec3(3.0f, roadY, 2.0f),
                      glm::vec3(2.0f, 2.0f, 2.0f),
                      glm::vec3(0, 0, 0));

    // ── Parked Golf cars on sidewalk (right kerb, parallel parking) ───────
    g_scene->addModel("parked_r1", "assets/models/Golf.obj",
                      glm::vec3(5.8f, sidewalkY, 4.0f),
                      glm::vec3(4.0f, 4.0f, 4.0f),
                      glm::vec3(0, 0, 0));

    g_scene->addModel("parked_r2", "assets/models/Golf.obj",
                      glm::vec3(5.8f, sidewalkY, -7.0f),
                      glm::vec3(4.0f, 4.0f, 4.0f),
                      glm::vec3(0, 0, 0));

    // ── Parked Golf cars (left kerb) ──────────────────────────────────────
    g_scene->addModel("parked_l1", "assets/models/Golf.obj",
                      glm::vec3(-5.8f, sidewalkY, 0.0f),
                      glm::vec3(4.0f, 4.0f, 4.0f),
                      glm::vec3(0, 180, 0));

    g_scene->addModel("parked_l2", "assets/models/Golf.obj",
                      glm::vec3(-5.8f, sidewalkY, -10.0f),
                      glm::vec3(4.0f, 4.0f, 4.0f),
                      glm::vec3(0, 180, 0));

    // ── Buildings (FBX) ──────────────────────────────────────────────────
    // Native size: 12m wide × 8.5m tall × 9.3m deep.
    // rotation.y=90 aligns the 12m axis along the road (world -Z).
    // scale=15 → 15m along road, ~10.6m tall, ~11.6m deep.
    // Sidewalk outer edge X=8.5 + half-depth 5.8 → centre X=14.3 ≈ 14.
    // 5 buildings per side, 20m spacing: Z centres -20, -40, -60, -80, -100.
    const glm::vec3 bldgScale(15.0f, 15.0f, 15.0f);
    float bldgZ[] = { -20.0f, -40.0f, -60.0f, -80.0f, -100.0f };

    for (int i = 0; i < 5; ++i) {
        g_scene->addAssimpModel("bldg_r" + std::to_string(i),
                                "objects/Free_Building/fbxBuilding.fbx",
                                glm::vec3(14.0f, 0.0f, bldgZ[i]),
                                bldgScale,
                                glm::vec3(0, 90, 0));

        g_scene->addAssimpModel("bldg_l" + std::to_string(i),
                                "objects/Free_Building/fbxBuilding.fbx",
                                glm::vec3(-14.0f, 0.0f, bldgZ[i]),
                                bldgScale,
                                glm::vec3(0, 270, 0));
    }

    // ── Character (Assimp FBX) ────────────────────────────────────────────
    g_scene->addAssimpModel("character",
                            "objects/fbx_Clean/fbx Clean.fbx",
                            glm::vec3(0.0f, roadY, 5.0f),
                            glm::vec3(2.0f, 2.0f, 2.0f),
                            glm::vec3(-90, 0, 0));

    std::cout << "Game scene loaded successfully!" << std::endl;

    // Create and load menu
    g_menu = new Menu();
    if (!g_menu->loadBackground("background_start_menu.png")) {
        std::cerr << "Warning: Could not load menu background. Using default..." << std::endl;
    }

    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMotionFunc(motion);

    std::cout << "Menu initialized!" << std::endl;
    std::cout << "Navigation: UP/DOWN arrows to select, ENTER to confirm" << std::endl;
    std::cout << "\nStarting main loop..." << std::endl;

    // Enter main loop
    glutMainLoop();

    // Cleanup
    delete g_model;
    delete g_scene;
    delete g_menu;
    return 0;
}
