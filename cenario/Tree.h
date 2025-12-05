#pragma once

#include <GL/glut.h>
#include <GL/glu.h>
#include <cstdlib> // Para rand()
#include <cmath>

//carregador de OBJ
#include "../util/loads.h"

// Declara as variáveis globais que estão no seu main.cpp (estilingue.cpp)
// para que esta classe saiba que elas existem.
extern OBJModel treeModel;
extern bool treeModelLoaded;

class Tree {
public:
    float x, y, z;
    float scale;
    float rotation;
    
    Tree(float posX, float posY, float posZ, float s = 1.0f) 
        : x(posX), y(posY), z(posZ), scale(s) {
        // Rotação aleatória para dar variedade
        rotation = (float)(rand() % 360);
    }
    
    void draw() {
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(rotation, 0, 1, 0);
        glScalef(scale, scale, scale);
        
        if (treeModelLoaded) {
            // Renderiza o modelo 3D (OBJ)
            glEnable(GL_LIGHTING);
            // Desativa color material momentaneamente se o OBJ tiver seus próprios materiais
            glDisable(GL_COLOR_MATERIAL); 
            
            glPushMatrix();
            glTranslatef(0.0f, 0.3f, 0.0f); // Ajuste fino de altura para não afundar no chão
            treeModel.draw();
            glPopMatrix();
            
            glEnable(GL_COLOR_MATERIAL);
        } else {
            // Renderiza a árvore procedural (bolinhas e cilindro) caso o OBJ falhe
            drawProceduralTree();
        }
        
        glPopMatrix();
    }
    
private:
    void drawProceduralTree() {
        float height = 4.0f;
        float trunkRadius = 0.2f;
        float foliageRadius = 1.0f;
        
        // Tronco
        glColor3f(0.4f, 0.25f, 0.1f);
        glPushMatrix();
            glRotatef(-90, 1, 0, 0);
            GLUquadric* trunk = gluNewQuadric();
            gluCylinder(trunk, trunkRadius, trunkRadius * 0.8f, height * 0.6f, 8, 8);
            gluDeleteQuadric(trunk);
        glPopMatrix();
        
        // Folhagem (3 esferas empilhadas)
        glPushMatrix();
            glColor3f(0.2f, 0.5f, 0.2f);
            glTranslatef(0, height * 0.5f, 0);
            glutSolidSphere(foliageRadius, 12, 12);
            
            glTranslatef(0, height * 0.2f, 0); // Subindo um pouco mais
            glColor3f(0.25f, 0.55f, 0.25f);
            glutSolidSphere(foliageRadius * 0.9f, 12, 12);
            
            glTranslatef(0, height * 0.2f, 0); // Topo
            glColor3f(0.3f, 0.6f, 0.3f);
            glutSolidSphere(foliageRadius * 0.7f, 10, 10);
        glPopMatrix();
    }
};