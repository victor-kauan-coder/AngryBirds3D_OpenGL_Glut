#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include "loads.h"
#include "openGL_util.h"

struct Porco {
    float x, y, z;
    float scale;
    float rotation;

    Porco(float posX, float posY, float posZ, float s = 1.0f) 
        : x(posX), y(posY), z(posZ), scale(s) {
        rotation = (rand() % 360);
    }

    void draw() {
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(rotation, 0, 1, 0);
        glScalef(scale, scale, scale);

        // Desenhar o porco como uma esfera verde
        glColor3f(0.0f, 1.0f, 0.0f);
        glutSolidSphere(0.5f, 20, 20);

        glPopMatrix();
    }
};

void display(void){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    glColor3f (0.0, 0.0, 0.0); 
    Porco porco(0.0f, 0.0f, 0.0f);
    porco.draw();
    glutSwapBuffers(); 
}


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow("Porco");
    init();
    glEnable(GL_DEPTH_TEST);
    glutDisplayFunc(display);

    Porco porco(0.0f, 0.0f, -5.0f);

    glutMainLoop();
    return 0;
}