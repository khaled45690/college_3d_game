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
float cameraDistance = 20.0f;
float cameraRotX = 20.0f;
float cameraRotY = 0.0f;
Model* g_model = nullptr;
float modelRotation = 0.0f;

// Menu
Menu* g_menu = nullptr;
bool gameStarted = false;
bool g_gameActive = false;  // true once initEasyMode() has been called for this session
MenuState g_lastMenuState = MENU_MAIN;

// Game Scene
Scene* g_scene = nullptr;

// Forward declarations
void renderHUD();

// Display callback
void display() {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Check if menu should be shown
    MenuState curMenuState = g_menu->getState();
    if (curMenuState != GAME_RUNNING) {
        // Only drop the active flag when actually leaving the game, not every menu frame
        if (g_lastMenuState == GAME_RUNNING)
            g_gameActive = false;
        g_lastMenuState = curMenuState;
        g_menu->render();
    } else {
        g_lastMenuState = GAME_RUNNING;
        // Initialize game on the first frame of GAME_RUNNING
        if (!g_gameActive) {
            if (g_menu->getDifficulty() == DIFFICULTY_HARD)
                g_scene->initHardMode();
            else
                g_scene->initEasyMode();
            g_gameActive   = true;
            cameraDistance = 20.0f;
            cameraRotX     = 25.0f;
            cameraRotY     = 0.0f;
        }

        // Set up projection matrix
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        // near=0.5m, far=2400m — covers 3× scaled road length
        gluPerspective(50.0f, (float)windowWidth / (float)windowHeight, 0.5f, 2400.0f);

        // Set up model view matrix with camera
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Camera follows character position (X and Z)
        const float roadY = 0.0f;
        float trackX = g_scene ? g_scene->getCharX() : 0.0f;
        float trackZ = g_scene ? g_scene->getCharZ() : 0.0f;
        float camX = cameraDistance * sinf(cameraRotY * 3.14159f / 180.0f) * cosf(cameraRotX * 3.14159f / 180.0f);
        float camY = cameraDistance * sinf(cameraRotX * 3.14159f / 180.0f) + roadY;
        float camZ = cameraDistance * cosf(cameraRotY * 3.14159f / 180.0f) * cosf(cameraRotX * 3.14159f / 180.0f);

        gluLookAt(camX + trackX, camY, camZ + trackZ,
                  trackX, roadY + 6.0f, trackZ - 15.0f,
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

    // Win / lose overlay
    if (g_scene) {
        GameResult result = g_scene->getResult();
        if (result != GAME_PLAYING) {
            float bx = windowWidth  / 2.0f - 160.0f;
            float by = windowHeight / 2.0f - 70.0f;

            // Semi-transparent background panel
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.0f, 0.0f, 0.0f, 0.70f);
            glBegin(GL_QUADS);
            glVertex2f(bx,          by);
            glVertex2f(bx + 320.0f, by);
            glVertex2f(bx + 320.0f, by + 140.0f);
            glVertex2f(bx,          by + 140.0f);
            glEnd();
            glDisable(GL_BLEND);

            // Result text (large)
            if (result == GAME_WON) {
                glColor3f(0.2f, 1.0f, 0.3f);
                glRasterPos2f(bx + 90.0f, by + 48.0f);
                for (char c : std::string("YOU WIN!"))
                    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
            } else {
                glColor3f(1.0f, 0.25f, 0.25f);
                glRasterPos2f(bx + 80.0f, by + 48.0f);
                for (char c : std::string("GAME OVER"))
                    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
            }

            // Prompt
            glColor3f(1.0f, 1.0f, 1.0f);
            glRasterPos2f(bx + 25.0f, by + 95.0f);
            for (char c : std::string("Press R to play again   |   ESC to quit"))
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
        }
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
    static int lastTime = 0;
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    if (lastTime == 0) lastTime = currentTime;
    float dt = (currentTime - lastTime) / 1000.0f;
    if (dt > 0.05f) dt = 0.05f;  // cap at 50 ms so big pauses don't explode physics
    lastTime = currentTime;

    if (g_scene && g_menu && g_menu->getState() == GAME_RUNNING)
        g_scene->update(dt);

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
        case 'r': // Restart game if over, otherwise reset camera
        case 'R':
            if (g_scene && g_scene->getResult() != GAME_PLAYING) {
                if (g_menu->getDifficulty() == DIFFICULTY_HARD)
                    g_scene->initHardMode();
                else
                    g_scene->initEasyMode();
            } else {
                cameraRotX = 25.0f;
                cameraRotY = 0.0f;
                cameraDistance = 20.0f;
            }
            break;
        case ' ':   // Jump
            if (g_scene) g_scene->jump();
            break;
        case 'a':
        case 'A':   // Move left
            if (g_scene) g_scene->moveLeft();
            break;
        case 'd':
        case 'D':   // Move right
            if (g_scene) g_scene->moveRight();
            break;
        case 'w':  // Zoom in
        case 'W':
            cameraDistance -= 10.0f;
            if (cameraDistance < 10.0f) cameraDistance = 10.0f;
            break;
        case 's':  // Zoom out
        case 'S':
            cameraDistance += 10.0f;
            if (cameraDistance > 6000.0f) cameraDistance = 6000.0f;
            break;
        case '8':  // Move forward (zoom in)
            cameraDistance -= 10.0f;
            if (cameraDistance < 5.0f) cameraDistance = 5.0f;
            break;
        case '5':  // Move backward (zoom out)
            cameraDistance += 10.0f;
            if (cameraDistance > 6000.0f) cameraDistance = 6000.0f;
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
    if (g_menu->getState() != GAME_RUNNING) {
        g_menu->handleSpecialKeypress(key);
        glutPostRedisplay();
        return;
    }
    switch (key) {
        case GLUT_KEY_LEFT:
            if (g_scene) g_scene->moveLeft();
            break;
        case GLUT_KEY_RIGHT:
            if (g_scene) g_scene->moveRight();
            break;
    }
    glutPostRedisplay();
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

    // Obstacles and the player character are managed by Scene::initEasyMode().
    // Static scene objects (buildings, character) are loaded below.

    // ── Buildings (FBX) ──────────────────────────────────────────────────
    // Native size: 12m wide × 8.5m tall × 9.3m deep.
    // rotation.y=90 aligns the 12m axis along the road (world -Z).
    // scale=15 → 15m along road, ~10.6m tall, ~11.6m deep.
    // Sidewalk outer edge X=8.5 + half-depth 5.8 → centre X=14.3 ≈ 14.
    // 5 buildings per side, 20m spacing: Z centres -20, -40, -60, -80, -100.
    const glm::vec3 bldgScale(45.0f, 45.0f, 45.0f);
    float bldgZ[] = { -60.0f, -120.0f, -180.0f, -240.0f, -300.0f };

    for (int i = 0; i < 5; ++i) {
        g_scene->addAssimpModel("bldg_r" + std::to_string(i),
                                "objects/Free_Building/fbxBuilding.fbx",
                                glm::vec3(42.0f, 0.0f, bldgZ[i]),
                                bldgScale,
                                glm::vec3(0, 90, 0));

        g_scene->addAssimpModel("bldg_l" + std::to_string(i),
                                "objects/Free_Building/fbxBuilding.fbx",
                                glm::vec3(-42.0f, 0.0f, bldgZ[i]),
                                bldgScale,
                                glm::vec3(0, 270, 0));
    }

    // ── Character (Assimp FBX) ────────────────────────────────────────────
    g_scene->addAssimpModel("character",
                            "Action Adventure Pack/main.fbx",
                            glm::vec3(0.0f, 0.0f, 5.0f),
                            glm::vec3(4.5f, 4.5f, 4.5f),
                            glm::vec3(0, 180, 0));

    // Load Mixamo animation clips onto the character.
    // Place your downloaded FBX files in objects/fbx_Clean/.
    // The character FBX itself may already contain a "run" clip — if Assimp
    // finds one it is stored automatically; loadAnimation() adds extra clips.
    g_scene->loadAssimpAnimation("character",
                                  "Character_Pack/running.fbx", "run");
    g_scene->loadAssimpAnimation("character",
                                  "Character_Pack/jump.fbx",    "jump");

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
