#include "LoadingScreen.h"
#include <cstdio>
#include <string>

#include "../stb/stb_image.h" 

bool jogoCarregado = false;
GLuint texturaLoadingBG = 0;

GLuint carregarTexturaLoading(const char* filename) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    } else {
        printf("ERRO: Nao foi possivel carregar imagem de loading: %s\n", filename);
    }
    stbi_image_free(data);
    return textureID;
}

void desenharTextoLoading(float x, float y, const char *string, void* fonte) {

    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(x + 2, y - 2);
    for (const char* c = string; *c != '\0'; c++) glutBitmapCharacter(fonte, *c);

    // Texto
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    for (const char* c = string; *c != '\0'; c++) glutBitmapCharacter(fonte, *c);
}

void InitLoadingScreen() {

    texturaLoadingBG = carregarTexturaLoading("Objetos/texturas/angry-birds.png");
}

void DrawLoadingScreen(int w, int h) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, texturaLoadingBG);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0, 0);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(w, 0);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(w, h);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0, h);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    desenharTextoLoading(50, 50, "Carregando...", GLUT_BITMAP_HELVETICA_18);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}