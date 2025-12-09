#include "gameMenu.h"
#include <cstdio>
#include <cmath>

#include "../stb/stb_image.h" 

extern AudioManager g_audioManager;

GameMenu::GameMenu(int width, int height) {
    updateDimensions(width, height);
    currentVolume = 35.0f; 
    
    volSlider.value = 0.35f; 
    volSlider.isDragging = false;

    //imagem da tela de menu
    bgTextureID = loadTexture("Objetos/texturas/menu_background.png");
}


GLuint GameMenu::loadTexture(const char* filename) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    
    if (!data) {
        printf("ERRO MENU: Nao foi possivel carregar imagem: %s\n", filename);
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    //configurações de filtro (para a imagem não ficar pixelada ao esticar)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    //envia os pixels da memória RAM para a memória de vídeo (VRAM)
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    //libera da ram
    stbi_image_free(data);
    return textureID;
}

void GameMenu::updateDimensions(int width, int height) {
    screenWidth = width;
    screenHeight = height;

    float cx = width / 2.0f;
    float cy = height / 2.0f;
    float btnW = 200.0f;
    float btnH = 50.0f;
    float spacing = 70.0f;

    float startY = cy - 50.0f; 

    btnPlay = {cx - btnW/2, startY + spacing, btnW, btnH, "JOGAR", false};
    btnSettings = {cx - btnW/2, startY, btnW, btnH, "CONFIGURACOES", false};
    btnExit = {cx - btnW/2, startY - spacing, btnW, btnH, "SAIR", false};

    btnBack = {cx - btnW/2, cy - 150, btnW, btnH, "VOLTAR", false};
    volSlider = {cx - 150, cy, 300, 10, volSlider.value, false};
}

void GameMenu::drawText(float x, float y, std::string text, void* font) {
    glRasterPos2f(x-10, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}


void GameMenu::drawButton(Button& btn) {
    if (btn.isHovered) glColor3f(0.3f, 0.7f, 0.3f); 
    else glColor3f(0.2f, 0.5f, 0.2f); 

    glBegin(GL_QUADS);
        glVertex2f(btn.x, btn.y);
        glVertex2f(btn.x + btn.w, btn.y);
        glVertex2f(btn.x + btn.w, btn.y + btn.h);
        glVertex2f(btn.x, btn.y + btn.h);
    glEnd();

    // Borda 
    glColor3f(1.0f, 1.0f, 1.0f); //borada branca
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(btn.x, btn.y);
        glVertex2f(btn.x + btn.w, btn.y);
        glVertex2f(btn.x + btn.w, btn.y + btn.h);
        glVertex2f(btn.x, btn.y + btn.h);
    glEnd();

    // Texto
    float textW = btn.label.length() * 9.0f; 
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(btn.x + (btn.w - textW)/2, btn.y + (btn.h/2) - 5, btn.label);
}

void GameMenu::drawSlider() {
    glColor3f(0.8f, 0.8f, 0.8f);
    glBegin(GL_QUADS);
        glVertex2f(volSlider.x, volSlider.y - 2);
        glVertex2f(volSlider.x + volSlider.w, volSlider.y - 2);
        glVertex2f(volSlider.x + volSlider.w, volSlider.y + volSlider.h + 2);
        glVertex2f(volSlider.x, volSlider.y + volSlider.h + 2);
    glEnd();

    float fillWidth = volSlider.w * volSlider.value;
    glColor3f(0.2f, 0.8f, 0.2f);
    glBegin(GL_QUADS);
        glVertex2f(volSlider.x, volSlider.y);
        glVertex2f(volSlider.x + fillWidth, volSlider.y);
        glVertex2f(volSlider.x + fillWidth, volSlider.y + volSlider.h);
        glVertex2f(volSlider.x, volSlider.y + volSlider.h);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    float kx = volSlider.x + fillWidth;
    float ky = volSlider.y + volSlider.h/2;
    float ks = 10.0f; 
    glBegin(GL_QUADS);
        glVertex2f(kx - ks, ky - ks);
        glVertex2f(kx + ks, ky - ks);
        glVertex2f(kx + ks, ky + ks);
        glVertex2f(kx - ks, ky + ks);
    glEnd();

    char volStr[32];
    sprintf(volStr, "Volume: %d%%", (int)(volSlider.value * 100));
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(volSlider.x, volSlider.y + 30, volStr);
}

void GameMenu::draw(GameState currentState) {
    // desliga luz e profundidade para desenhar o menu 2d sem interferencia
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    // configura a projecao para 2d
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); // salva a camera do jogo
    glLoadIdentity();
    gluOrtho2D(0, screenWidth, 0, screenHeight); // define coordenadas da tela
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if (bgTextureID != 0) {
        // desenha a imagem de fundo se tiver textura carregada
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, bgTextureID);
        glColor3f(1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);
            // mapeia a textura na tela toda
            glTexCoord2f(0, 1); glVertex2f(0, 0);            
            glTexCoord2f(1, 1); glVertex2f(screenWidth, 0);  
            glTexCoord2f(1, 0); glVertex2f(screenWidth, screenHeight); 
            glTexCoord2f(0, 0); glVertex2f(0, screenHeight); 
        glEnd();

        glDisable(GL_TEXTURE_2D);
    } else {
        // pinta um fundo cinza caso a imagem falhe
        glColor3f(0.1f, 0.1f, 0.1f);
        glBegin(GL_QUADS);
            glVertex2f(0, 0);
            glVertex2f(screenWidth, 0);
            glVertex2f(screenWidth, screenHeight);
            glVertex2f(0, screenHeight);
        glEnd();
    }

    if (currentState == STATE_MENU) {
        // desenha os botoes do menu principal
        drawButton(btnPlay);
        drawButton(btnSettings);
        drawButton(btnExit);
    } 
    else if (currentState == STATE_SETTINGS) {
        // desenha a tela de configuracoes e slider
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(screenWidth/2 - 60, screenHeight - 100, "CONFIGURACOES", GLUT_BITMAP_TIMES_ROMAN_24);
        drawSlider();
        drawButton(btnBack);
    }

    // restaura a camera 3d do jogo e liga a profundidade de volta
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

bool GameMenu::isMouseOver(float mx, float my, Button& btn) {
    // inverte o y porque o opengl e diferente do windows
    float glY = screenHeight - my;
    // verifica se o mouse ta dentro do retangulo do botao
    return (mx >= btn.x && mx <= btn.x + btn.w && 
            glY >= btn.y && glY <= btn.y + btn.h);
}

GameState GameMenu::handleMouseClick(int button, int state, int x, int y, GameState currentState) {
    // se soltar o botao, para de arrastar o volume
    if (state != GLUT_DOWN) {
        volSlider.isDragging = false;
        return currentState;
    }

    if (currentState == STATE_MENU) {
        // verifica cliques nos botoes do menu principal e troca estado
        if (isMouseOver(x, y, btnPlay)) return STATE_GAME;
        if (isMouseOver(x, y, btnSettings)) return STATE_SETTINGS;
        if (isMouseOver(x, y, btnExit)) return STATE_EXIT;
    }
    else if (currentState == STATE_SETTINGS) {
        // botao de voltar pro menu
        if (isMouseOver(x, y, btnBack)) return STATE_MENU;
        
        float glY = screenHeight - y;
        // verifica se clicou na barra de volume
        if (x >= volSlider.x && x <= volSlider.x + volSlider.w &&
            glY >= volSlider.y - 10 && glY <= volSlider.y + volSlider.h + 10) {
            volSlider.isDragging = true;
            handleMouseMotion(x, y, currentState); // ja atualiza o volume na hora do clique
        }
    }
    return currentState;
}

void GameMenu::handleMouseMotion(int x, int y, GameState currentState) {
    if (currentState == STATE_MENU) {
        // atualiza o efeito de mouse em cima dos botoes (hover)
        btnPlay.isHovered = isMouseOver(x, y, btnPlay);
        btnSettings.isHovered = isMouseOver(x, y, btnSettings);
        btnExit.isHovered = isMouseOver(x, y, btnExit);
    } else if (currentState == STATE_SETTINGS) {
        btnBack.isHovered = isMouseOver(x, y, btnBack);

        if (volSlider.isDragging) {
            // calcula a porcentagem do slider baseado na posicao do mouse
            float val = (x - volSlider.x) / volSlider.w;
            // limita o valor entre 0 e 1
            if (val < 0.0f) val = 0.0f;
            if (val > 1.0f) val = 1.0f;
            volSlider.value = val;
            
            // atualiza o volume real do jogo
            g_audioManager.setVolume(val * 100.0f); 
        }
    }
}