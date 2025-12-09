#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H

#include <GL/glut.h>



extern bool jogoCarregado;
extern GLuint texturaLoadingBG;

void InitLoadingScreen();
void DrawLoadingScreen(int larguraJanela, int alturaJanela);

#endif