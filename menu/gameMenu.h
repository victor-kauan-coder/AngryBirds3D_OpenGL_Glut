#ifndef GAME_MENU_H
#define GAME_MENU_H

#include <GL/glut.h>
#include <string>
#include "../controle_audio/audio_manager.h"

// Estados possíveis do jogo
enum GameState {
    STATE_MENU,
    STATE_SETTINGS,
    STATE_GAME,
    STATE_EXIT
};

class GameMenu {
private:
    int screenWidth;
    int screenHeight;
    float currentVolume;
    
    // --- NOVO: Variável para a textura do fundo ---
    GLuint bgTextureID; 

    struct Button {
        float x, y, w, h;
        std::string label;
        bool isHovered;
    };

    Button btnPlay;
    Button btnSettings;
    Button btnExit;
    Button btnBack;

    struct Slider {
        float x, y, w, h;
        float value; 
        bool isDragging;
    } volSlider;

    // Função auxiliar interna para carregar a imagem
    GLuint loadTexture(const char* filename);

    void drawButton(Button& btn);
    void drawText(float x, float y, std::string text, void* font = GLUT_BITMAP_HELVETICA_18);
    void drawSlider();
    bool isMouseOver(float mx, float my, Button& btn);

public:
    GameMenu(int width, int height);
    void draw(GameState currentState);
    GameState handleMouseClick(int button, int state, int x, int y, GameState currentState);
    void handleMouseMotion(int x, int y, GameState currentState);
    void updateDimensions(int width, int height);
};

#endif