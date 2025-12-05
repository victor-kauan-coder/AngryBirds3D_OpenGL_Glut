#pragma once

#include <GL/glut.h>

class LightingManager {
private:
    // Definição das cores da luz (Ambiente, Difusa, Especular)
    // Usamos os valores que você ajustou anteriormente (mais claro, sem brilho excessivo)
    GLfloat lightAmb[4] = {0.8f, 0.8f, 0.8f, 1.0f};  // Luz ambiente forte
    GLfloat lightDif[4] = {1.0f, 1.0f, 1.0f, 1.0f};  // Luz difusa branca
    GLfloat lightSpec[4] = {0.5f, 0.5f, 0.5f, 1.0f}; // Sem brilho especular (fosco)
    
    // Posição da luz (X, Y, Z, W)
    // W = 1.0f indica uma luz posicional (lâmpada/sol)
    GLfloat lightPos[4] = {10.0f, 25.0f, 10.0f, 1.0f}; 

public:
    LightingManager() {}

    /**
     * @brief Configura as cores e ativa a luz. 
     * Chame isso na função init() do main.
     */
    void init() {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        
        // Configura as propriedades da Luz 0
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDif);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
        
        // Configurações globais de material para receber a luz corretamente
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        
        // Configura brilho padrão dos materiais
        GLfloat matSpec[] = {0.8f, 0.8f, 0.8f, 1.0f};
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
        glMaterialf(GL_FRONT, GL_SHININESS, 20.0f);
    }

    /**
     * @brief Aplica a posição da luz.
     * Chame isso no início da função display().
     */
    void apply() {
        // Redefine a posição a cada frame (caso a câmera se mova, a luz fica fixa no mundo)
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    }

    // Setters caso você queira mudar a luz durante o jogo (ex: mudar para noite)
    void setPosition(float x, float y, float z) {
        lightPos[0] = x; lightPos[1] = y; lightPos[2] = z;
    }
    
    void setAmbient(float r, float g, float b) {
        lightAmb[0] = r; lightAmb[1] = g; lightAmb[2] = b;
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    }
};