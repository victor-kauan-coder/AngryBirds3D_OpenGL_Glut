#include "gameMenu.h"
#include <cstdio>
#include <cmath>

// Ajuste o caminho conforme sua pasta. 
// Se "stb" está na raiz e este arquivo em "Menu", usamos "../stb/"
#include "../stb/stb_image.h" 

extern AudioManager g_audioManager;

GameMenu::GameMenu(int width, int height) {
    updateDimensions(width, height);
    currentVolume = 20.0f; 
    
    volSlider.value = 0.5f; 
    volSlider.isDragging = false;

    // --- CARREGAR A IMAGEM AQUI ---
    // Certifique-se de criar essa imagem na pasta correta!
    bgTextureID = loadTexture("Objetos/texturas/menu_background.png");
}

// --- Nova Função de Carregamento ---
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

    // Configurações de filtro (para a imagem não ficar pixelada ao esticar)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

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

    // Ajustei um pouco a posição para baixo, assumindo que a imagem de fundo tenha um logo em cima
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

// Mantendo o botão quadrado (sem arredondamento)
void GameMenu::drawButton(Button& btn) {
    if (btn.isHovered) glColor3f(0.3f, 0.7f, 0.3f); 
    else glColor3f(0.2f, 0.5f, 0.2f); 

    glBegin(GL_QUADS);
        glVertex2f(btn.x, btn.y);
        glVertex2f(btn.x + btn.w, btn.y);
        glVertex2f(btn.x + btn.w, btn.y + btn.h);
        glVertex2f(btn.x, btn.y + btn.h);
    glEnd();

    // Borda simples
    glColor3f(1.0f, 1.0f, 1.0f);
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
    glColor3f(0.8f, 0.8f, 0.8f); // Fundo do slider mais claro para contrastar com imagem
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

    // Texto com fundo preto pequeno para leitura
    char volStr[32];
    sprintf(volStr, "Volume: %d%%", (int)(volSlider.value * 100));
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(volSlider.x, volSlider.y + 30, volStr);
}

void GameMenu::draw(GameState currentState) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, screenWidth, 0, screenHeight);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // --- DESENHAR IMAGEM DE FUNDO ---
    if (bgTextureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, bgTextureID);
        glColor3f(1.0f, 1.0f, 1.0f); // Branco para manter cores originais

        glBegin(GL_QUADS);
            // Mapeia a textura inteira para a tela inteira
            glTexCoord2f(0, 1); glVertex2f(0, 0);             // Inferior Esq (Note a inversão Y se necessário)
            glTexCoord2f(1, 1); glVertex2f(screenWidth, 0);   // Inferior Dir
            glTexCoord2f(1, 0); glVertex2f(screenWidth, screenHeight); // Superior Dir
            glTexCoord2f(0, 0); glVertex2f(0, screenHeight);  // Superior Esq
        glEnd();

        glDisable(GL_TEXTURE_2D);
    } else {
        // Fallback: Se não carregou a imagem, pinta de cinza escuro
        glColor3f(0.1f, 0.1f, 0.1f);
        glBegin(GL_QUADS);
            glVertex2f(0, 0);
            glVertex2f(screenWidth, 0);
            glVertex2f(screenWidth, screenHeight);
            glVertex2f(0, screenHeight);
        glEnd();
    }

    // --- Desenha os Botões ---
    if (currentState == STATE_MENU) {
        // Título (opcional, se a imagem já tiver logo, remova isso)
        // glColor3f(1.0f, 0.8f, 0.0f);
        // drawText(screenWidth/2 - 80, screenHeight - 150, "ESTILINGUE 3D", GLUT_BITMAP_TIMES_ROMAN_24);
        
        drawButton(btnPlay);
        drawButton(btnSettings);
        drawButton(btnExit);
    } 
    else if (currentState == STATE_SETTINGS) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(screenWidth/2 - 60, screenHeight - 100, "CONFIGURACOES", GLUT_BITMAP_TIMES_ROMAN_24);
        
        drawSlider();
        drawButton(btnBack);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

// ... (Mantenha as funções isMouseOver, handleMouseClick e handleMouseMotion iguais) ...
bool GameMenu::isMouseOver(float mx, float my, Button& btn) {
    float glY = screenHeight - my;
    return (mx >= btn.x && mx <= btn.x + btn.w && 
            glY >= btn.y && glY <= btn.y + btn.h);
}

GameState GameMenu::handleMouseClick(int button, int state, int x, int y, GameState currentState) {
    if (state != GLUT_DOWN) {
        volSlider.isDragging = false;
        return currentState;
    }

    if (currentState == STATE_MENU) {
        if (isMouseOver(x, y, btnPlay)) return STATE_GAME;
        if (isMouseOver(x, y, btnSettings)) return STATE_SETTINGS;
        if (isMouseOver(x, y, btnExit)) return STATE_EXIT;
    }
    else if (currentState == STATE_SETTINGS) {
        if (isMouseOver(x, y, btnBack)) return STATE_MENU;
        
        float glY = screenHeight - y;
        if (x >= volSlider.x && x <= volSlider.x + volSlider.w &&
            glY >= volSlider.y - 10 && glY <= volSlider.y + volSlider.h + 10) {
            volSlider.isDragging = true;
            handleMouseMotion(x, y, currentState);
        }
    }
    return currentState;
}

void GameMenu::handleMouseMotion(int x, int y, GameState currentState) {
    if (currentState == STATE_MENU) {
        btnPlay.isHovered = isMouseOver(x, y, btnPlay);
        btnSettings.isHovered = isMouseOver(x, y, btnSettings);
        btnExit.isHovered = isMouseOver(x, y, btnExit);
    } else if (currentState == STATE_SETTINGS) {
        btnBack.isHovered = isMouseOver(x, y, btnBack);

        if (volSlider.isDragging) {
            float val = (x - volSlider.x) / volSlider.w;
            if (val < 0.0f) val = 0.0f;
            if (val > 1.0f) val = 1.0f;
            volSlider.value = val;
            
            g_audioManager.setVolume(val * 100.0f); 
        }
    }
}