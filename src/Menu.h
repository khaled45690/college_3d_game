#ifndef MENU_H
#define MENU_H

#include <GL/glut.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

enum MenuState {
    MENU_MAIN,
    MENU_DIFFICULTY,
    MENU_INSTRUCTIONS,
    GAME_RUNNING
};

enum Difficulty {
    DIFFICULTY_EASY,
    DIFFICULTY_HARD
};

struct Button {
    float x, y;
    float width, height;
    std::string label;
    bool selected;
};

class Menu {
public:
    Menu();
    ~Menu();

    bool loadBackground(const std::string& imagePath);
    void render();
    void handleKeypress(unsigned char key);
    void handleSpecialKeypress(int key);

    MenuState getState() const { return currentState; }
    Difficulty getDifficulty() const { return selectedDifficulty; }
    void setState(MenuState state) { currentState = state; }
    void reset();

private:
    MenuState currentState;
    Difficulty selectedDifficulty;

    GLuint backgroundTexture;
    cv::Mat backgroundImage;

    std::vector<Button> mainMenuButtons;
    std::vector<Button> difficultyButtons;
    int selectedButtonIndex;

    void renderMainMenu();
    void renderDifficultyMenu();
    void renderInstructions();
    void renderButton(const Button& button);
    void createMainMenuButtons();
    void createDifficultyButtons();

    GLuint loadTextureFromMat(const cv::Mat& image);
    void renderBackground();
};

#endif // MENU_H
