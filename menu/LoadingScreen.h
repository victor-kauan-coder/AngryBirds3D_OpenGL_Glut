#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H

#include <GL/glut.h>

// Variável global compartilhada (extern significa que ela existe, mas está definida no .cpp)

extern bool jogoCarregado;
extern GLuint texturaLoadingBG;
// Funções que o estilingue.cpp vai chamar
void InitLoadingScreen();
void DrawLoadingScreen(int larguraJanela, int alturaJanela);

#endif