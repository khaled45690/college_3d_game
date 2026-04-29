#include "Menu.h"
#include <iostream>
#include <glm/glm.hpp>

Menu::Menu()
    : currentState(MENU_MAIN),
      selectedDifficulty(DIFFICULTY_EASY),
      backgroundTexture(0),
      selectedButtonIndex(0) {
    createMainMenuButtons();
    createDifficultyButtons();
}

Menu::~Menu() {
    if (backgroundTexture != 0) {
        glDeleteTextures(1, &backgroundTexture);
    }
}

bool Menu::loadBackground(const std::string& imagePath) {
    backgroundImage = cv::imread(imagePath);
    if (backgroundImage.empty()) {
        std::cerr << "Failed to load background image: " << imagePath << std::endl;
        return false;
    }

    std::cout << "Loaded background image: " << imagePath << std::endl;
    std::cout << "Image size: " << backgroundImage.cols << "x" << backgroundImage.rows << std::endl;

    // Convert BGR to RGB (OpenCV loads as BGR, but OpenGL expects RGB)
    cv::Mat imageRGB;
    cv::cvtColor(backgroundImage, imageRGB, cv::COLOR_BGR2RGB);

    backgroundTexture = loadTextureFromMat(imageRGB);
    return backgroundTexture != 0;
}

GLuint Menu::loadTextureFromMat(const cv::Mat& image) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image.data);

    std::cout << "Texture created with ID: " << texture << std::endl;
    return texture;
}

void Menu::createMainMenuButtons() {
    mainMenuButtons.clear();
    mainMenuButtons.push_back({400.0f - 75.0f, 200.0f, 150.0f, 60.0f, "START GAME", true});
    mainMenuButtons.push_back({400.0f - 75.0f, 300.0f, 150.0f, 60.0f, "INSTRUCTIONS", false});
    mainMenuButtons.push_back({400.0f - 75.0f, 400.0f, 150.0f, 60.0f, "EXIT", false});
}

void Menu::createDifficultyButtons() {
    difficultyButtons.clear();
    difficultyButtons.push_back({300.0f - 50.0f, 250.0f, 100.0f, 60.0f, "EASY", true});
    difficultyButtons.push_back({500.0f - 50.0f, 250.0f, 100.0f, 60.0f, "HARD", false});
}

void Menu::render() {
    // Set up 2D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Disable depth testing for 2D rendering
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Render background
    renderBackground();

    // Render appropriate menu
    switch (currentState) {
        case MENU_MAIN:
            renderMainMenu();
            break;
        case MENU_DIFFICULTY:
            renderDifficultyMenu();
            break;
        case MENU_INSTRUCTIONS:
            renderInstructions();
            break;
        default:
            break;
    }

    // Re-enable depth testing
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    // Restore projection
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void Menu::renderBackground() {
    if (backgroundTexture == 0) return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(800.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(800.0f, 600.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 600.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void Menu::renderMainMenu() {
    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(250.0f, 100.0f);
    std::string title = "PEPSI MAN 3D";
    for (char c : title) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    for (const auto& button : mainMenuButtons) {
        renderButton(button);
    }
}

void Menu::renderDifficultyMenu() {
    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(250.0f, 100.0f);
    std::string title = "SELECT DIFFICULTY";
    for (char c : title) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    for (const auto& button : difficultyButtons) {
        renderButton(button);
    }
}

void Menu::renderInstructions() {
    glColor3f(0.0f, 0.0f, 0.0f);

    // Title
    glRasterPos2f(200.0f, 60.0f);
    std::string title = "HOW TO PLAY";
    for (char c : title) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // Instructions
    glRasterPos2f(100.0f, 150.0f);
    std::string line1 = "LEFT ARROW / A  - Move Left";
    for (char c : line1) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    glRasterPos2f(100.0f, 180.0f);
    std::string line2 = "RIGHT ARROW / D - Move Right";
    for (char c : line2) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    glRasterPos2f(100.0f, 210.0f);
    std::string line3 = "SPACE BAR       - Jump";
    for (char c : line3) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    glRasterPos2f(100.0f, 240.0f);
    std::string line4 = "Collect cans and avoid obstacles!";
    for (char c : line4) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    glRasterPos2f(200.0f, 350.0f);
    std::string line5 = "Press SPACE to continue...";
    for (char c : line5) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }
}

void Menu::renderButton(const Button& button) {
    if (button.selected) {
        glColor3f(1.0f, 0.8f, 0.0f);  // Yellow for selected
    } else {
        glColor3f(0.5f, 0.5f, 0.5f);  // Gray for unselected
    }

    // Draw button background
    glBegin(GL_QUADS);
    glVertex2f(button.x, button.y);
    glVertex2f(button.x + button.width, button.y);
    glVertex2f(button.x + button.width, button.y + button.height);
    glVertex2f(button.x, button.y + button.height);
    glEnd();

    // Draw button border
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(button.x, button.y);
    glVertex2f(button.x + button.width, button.y);
    glVertex2f(button.x + button.width, button.y + button.height);
    glVertex2f(button.x, button.y + button.height);
    glEnd();
    glLineWidth(1.0f);

    // Draw button text
    glColor3f(0.0f, 0.0f, 0.0f);
    float textX = button.x + (button.width - button.label.length() * 8) / 2.0f;
    float textY = button.y + button.height / 2.0f + 5.0f;
    glRasterPos2f(textX, textY);

    for (char c : button.label) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }
}

void Menu::handleKeypress(unsigned char key) {
    switch (currentState) {
        case MENU_MAIN:
            if (key == 13) {  // Enter key
                if (selectedButtonIndex == 0) {
                    currentState = MENU_DIFFICULTY;
                    selectedButtonIndex = 0;
                    difficultyButtons[0].selected = true;
                    difficultyButtons[1].selected = false;
                } else if (selectedButtonIndex == 1) {
                    currentState = MENU_INSTRUCTIONS;
                } else if (selectedButtonIndex == 2) {
                    exit(0);
                }
            }
            break;

        case MENU_DIFFICULTY:
            if (key == 13) {  // Enter key
                selectedDifficulty = (selectedButtonIndex == 0) ? DIFFICULTY_EASY : DIFFICULTY_HARD;
                currentState = GAME_RUNNING;
            }
            break;

        case MENU_INSTRUCTIONS:
            if (key == 32 || key == 13) {  // Space or Enter
                currentState = MENU_MAIN;
                selectedButtonIndex = 0;
                mainMenuButtons[0].selected = true;
                mainMenuButtons[1].selected = false;
                mainMenuButtons[2].selected = false;
            }
            break;

        default:
            break;
    }
}

void Menu::handleSpecialKeypress(int key) {
    if (currentState == MENU_MAIN) {
        if (key == GLUT_KEY_UP) {
            selectedButtonIndex = (selectedButtonIndex - 1 + mainMenuButtons.size()) % mainMenuButtons.size();
        } else if (key == GLUT_KEY_DOWN) {
            selectedButtonIndex = (selectedButtonIndex + 1) % mainMenuButtons.size();
        }

        for (size_t i = 0; i < mainMenuButtons.size(); i++) {
            mainMenuButtons[i].selected = (i == selectedButtonIndex);
        }
    } else if (currentState == MENU_DIFFICULTY) {
        if (key == GLUT_KEY_LEFT) {
            selectedButtonIndex = 0;
        } else if (key == GLUT_KEY_RIGHT) {
            selectedButtonIndex = 1;
        }

        difficultyButtons[0].selected = (selectedButtonIndex == 0);
        difficultyButtons[1].selected = (selectedButtonIndex == 1);
    }
}

void Menu::reset() {
    currentState = MENU_MAIN;
    selectedDifficulty = DIFFICULTY_EASY;
    selectedButtonIndex = 0;
    createMainMenuButtons();
    createDifficultyButtons();
}
