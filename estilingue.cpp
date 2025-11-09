#include <GL/glut.h>
#include <GL/glu.h>
#include <bullet/btBulletDynamicsCommon.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include "Loads.h"
#include "passaros/passaro.h"
#include "passaros/Red.h"
#include "SlingshotManager.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Includes de Áudio (exemplo) ---
// Adicione os includes de áudio aqui se estiver usando (SDL_mixer.h, etc.)
// #include <SDL2/SDL.h>
// #include <SDL2/SDL_mixer.h>
// Mix_Chunk *som_esticar = nullptr;
// Mix_Chunk *som_soltar = nullptr;
// --- Fim Áudio ---


const int WIDTH = 1280;
const int HEIGHT = 720;

// Mundo de física Bullet
btDiscreteDynamicsWorld* dynamicsWorld;
btAlignedObjectArray<btCollisionShape*> collisionShapes;
btCollisionShape* projectileShape = nullptr;
btCollisionShape* boxShape = nullptr;

btBroadphaseInterface* broadphase = nullptr;
btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
btCollisionDispatcher* dispatcher = nullptr;
btSequentialImpulseConstraintSolver* solver = nullptr;

// NOVO: Estrutura para Material MTL
// NOVO: Estrutura para carregar e renderizar modelos OBJ com MTL


// Modelos OBJ globais
OBJModel treeModel;
bool treeModelLoaded = false;

// ... (Classe Tree permanece a mesma) ...
class Tree {
public:
    float x, y, z;
    float scale;
    float rotation;
    
    Tree(float posX, float posY, float posZ, float s = 1.0f) 
        : x(posX), y(posY), z(posZ), scale(s) {
        rotation = (rand() % 360);
    }
    
    void draw() {
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(rotation, 0, 1, 0);
        glScalef(scale, scale, scale);
        
        if (treeModelLoaded) {
            glColor3f(0.3f, 0.5f, 0.2f);
            treeModel.draw();
        } else {
            drawProceduralTree();
        }
        
        glPopMatrix();
    }
    
private:
    void drawProceduralTree() {
        float height = 4.0f;
        float trunkRadius = 0.2f;
        float foliageRadius = 1.0f;
        
        glColor3f(0.4f, 0.25f, 0.1f);
        glRotatef(-90, 1, 0, 0);
        GLUquadric* trunk = gluNewQuadric();
        gluCylinder(trunk, trunkRadius, trunkRadius * 0.8f, height * 0.6f, 8, 8);
        gluDeleteQuadric(trunk);
        glRotatef(90, 1, 0, 0);
        
        glColor3f(0.2f, 0.5f, 0.2f);
        glTranslatef(0, height * 0.5f, 0);
        glutSolidSphere(foliageRadius, 12, 12);
        glTranslatef(0, height * 0.1f, 0);
        glColor3f(0.25f, 0.55f, 0.25f);
        glutSolidSphere(foliageRadius * 0.9f, 12, 12);
        glTranslatef(0, height * 0.1f, 0);
        glColor3f(0.3f, 0.6f, 0.3f);
        glutSolidSphere(foliageRadius * 0.7f, 10, 10);
    }
};
std::vector<Tree> trees;


// --- MODIFICADO: Variáveis Globais ---

// A 'struct Slingshot' e variáveis de mouse/estado foram removidas.
// Elas agora estão dentro da classe SlingshotManager.

// NOVO: Ponteiro global para o nosso gerenciador
SlingshotManager* g_slingshotManager = nullptr;


// Variáveis globais de câmera e jogo
float cameraAngleH = 45.0f;
float cameraAngleV = 20.0f;
float cameraDistance = 18.0f;
float cameraTargetY = 3.0f;

int score = 0;
int shotsRemaining = 8;
bool gameOver = false;

std::vector<btRigidBody*> targetBodies;
PassaroRed* red;
btRigidBody* createTargetBox(float mass, const btVector3& position) {
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(position);
    
    btVector3 localInertia(0, 0, 0);
    if (mass != 0.f)
        boxShape->calculateLocalInertia(mass, localInertia);
    
    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, boxShape, localInertia);
    btRigidBody* body = new btRigidBody(rbInfo);
    
    body->setFriction(1.0f);
    body->setRestitution(0.1f);
    
    dynamicsWorld->addRigidBody(body);
    targetBodies.push_back(body);
    return body;
}

void initBullet() {
    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();
    
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
    
    btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);
    collisionShapes.push_back(groundShape);
    
    btDefaultMotionState* groundMotionState = new btDefaultMotionState(
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
    
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(
        0, groundMotionState, groundShape, btVector3(0, 0, 0));
    
    btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
    
    groundRigidBody->setFriction(0.8f);
    groundRigidBody->setRestitution(0.3f);
    
    dynamicsWorld->addRigidBody(groundRigidBody);
    projectileShape = new btSphereShape(0.3f);
    collisionShapes.push_back(projectileShape);
    
    boxShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
    collisionShapes.push_back(boxShape);
    
    targetBodies.clear();
    
    float boxMass = 1.0f;
    
    createTargetBox(boxMass, btVector3(-2.0f, 0.5f, -8.0f));
    createTargetBox(boxMass, btVector3(-1.0f, 0.5f, -8.0f));
    createTargetBox(boxMass, btVector3(0.0f, 0.5f, -8.0f));
    createTargetBox(boxMass, btVector3(1.0f, 0.5f, -8.0f));
    createTargetBox(boxMass, btVector3(-0.5f, 1.5f, -8.0f));
    createTargetBox(boxMass, btVector3(0.5f, 1.5f, -8.0f));
    
    createTargetBox(boxMass, btVector3(-3.0f, 0.5f, -12.0f));
    createTargetBox(boxMass, btVector3(-3.0f, 1.5f, -12.0f));
    createTargetBox(boxMass, btVector3(-3.0f, 2.5f, -12.0f));
    createTargetBox(boxMass, btVector3(2.0f, 0.5f, -12.0f));
    createTargetBox(boxMass, btVector3(2.0f, 1.5f, -12.0f));
    
    createTargetBox(boxMass, btVector3(0.0f, 0.5f, -16.0f));
    createTargetBox(boxMass, btVector3(-1.0f, 0.5f, -16.0f));
    createTargetBox(boxMass, btVector3(1.0f, 0.5f, -16.0f));
    createTargetBox(boxMass, btVector3(0.0f, 1.5f, -16.0f));
    
    createTargetBox(boxMass, btVector3(-5.0f, 0.5f, -10.0f));
    createTargetBox(boxMass, btVector3(-5.0f, 1.5f, -10.0f));
    createTargetBox(boxMass, btVector3(5.0f, 0.5f, -14.0f));
    createTargetBox(boxMass, btVector3(5.0f, 1.5f, -14.0f));
    
    trees.clear();
    trees.push_back(Tree(-8.0f, 0.0f, -10.0f, 10.0f));
    trees.push_back(Tree(-9.0f, 0.0f, -10.0f, 12.5f));
    trees.push_back(Tree(8.0f, 0.0f, -25.0f, 12.5f));
    trees.push_back(Tree(29.0f, 0.0f, -12.0f, 12.5f));
    trees.push_back(Tree(-14.0f, 0.0f, -18.0f, 13.5f));
    trees.push_back(Tree(13.5f, 0.0f, -20.0f, 14.8f));
    trees.push_back(Tree(-20.0f, 0.0f, -15.0f, 22.0f));
    trees.push_back(Tree(10.0f, 0.0f, -18.0f, 21.1f));
}

void clearProjectile() {
    if (red && red->getRigidBody()) {
        red->limparFisica();
    }
}

void createProjectileInPouch() {
    clearProjectile();
    
    float restX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
    float restY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
    float restZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
    
    slingshot.pouchX = restX;
    slingshot.pouchY = restY;
    slingshot.pouchZ = restZ;
    
    // SIMPLIFICADO: inicializarFisica já cria tudo!
    red->inicializarFisica(dynamicsWorld, restX, restY, restZ);
    
    // Configura propriedades físicas adicionais
    if (red->getRigidBody()) {
        red->getRigidBody()->setCollisionFlags(
            red->getRigidBody()->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT
        );
        red->getRigidBody()->setActivationState(DISABLE_DEACTIVATION);
    }
}

void drawCylinder(float x1, float y1, float z1, float x2, float y2, float z2, float radius) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    float length = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (length == 0.0) return;
    
    float z_axis[3] = {0.0, 0.0, 1.0};
    float vector[3] = {dx, dy, dz};
    vector[0] /= length;
    vector[1] /= length;
    vector[2] /= length;
    
    float axis[3];
    axis[0] = z_axis[1] * vector[2] - z_axis[2] * vector[1];
    axis[1] = z_axis[2] * vector[0] - z_axis[0] * vector[2];
    axis[2] = z_axis[0] * vector[1] - z_axis[1] * vector[0];
    
    float dot_product = z_axis[0]*vector[0] + z_axis[1]*vector[1] + z_axis[2]*vector[2];
    float angle = acos(dot_product) * 180.0 / M_PI;
    
    if (dot_product < -0.99999) {
        axis[0] = 1.0;
        axis[1] = 0.0;
        axis[2] = 0.0;
        angle = 180.0;
    }
    
    glPushMatrix();
    glTranslatef(x1, y1, z1);
    glRotatef(angle, axis[0], axis[1], axis[2]);
    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, radius, radius, length, 16, 16);
    gluDeleteQuadric(quad);
    glPopMatrix();
}

void drawWoodenBase() {
    glColor3f(0.5f, 0.35f, 0.05f);
    
    drawCylinder(slingshot.baseX, slingshot.baseY, slingshot.baseZ,
                 slingshot.leftArmX, slingshot.leftArmY, slingshot.leftArmZ,
                 0.3);
    
    drawCylinder(slingshot.leftArmX, slingshot.leftArmY, slingshot.leftArmZ,
                 slingshot.leftTopX, slingshot.leftTopY, slingshot.leftTopZ,
                 0.2);
    
    drawCylinder(slingshot.rightArmX, slingshot.rightArmY, slingshot.rightArmZ,
                 slingshot.rightTopX, slingshot.rightTopY, slingshot.rightTopZ,
                 0.2);
}

void drawElastic() {
    glColor3f(0.05f, 0.05f, 0.05f);
    glLineWidth(12.0f);
    glBegin(GL_LINES);
    glVertex3f(slingshot.leftTopX, slingshot.leftTopY, slingshot.leftTopZ);
    glVertex3f(slingshot.pouchX, slingshot.pouchY, slingshot.pouchZ);
    glVertex3f(slingshot.rightTopX, slingshot.rightTopY, slingshot.rightTopZ);
    glVertex3f(slingshot.pouchX, slingshot.pouchY, slingshot.pouchZ);
    glEnd();
}

void drawSky() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.6f, 0.9f);
    glVertex2f(0, 1);
    glVertex2f(1, 1);
    glColor3f(0.7f, 0.85f, 0.95f);
    glVertex2f(1, 0.3f);
    glVertex2f(0, 0.3f);
    glEnd();
    
    glColor3f(1.0f, 0.95f, 0.7f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.8f, 0.8f);
    for (int i = 0; i <= 20; i++) {
        float angle = (float)i / 20.0f * 2.0f * M_PI;
        glVertex2f(0.8f + cos(angle) * 0.08f, 0.8f + sin(angle) * 0.08f);
    }
    glEnd();
    
    glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
    
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.2f, 0.85f);
    for (int i = 0; i <= 20; i++) {
        float angle = (float)i / 20.0f * 2.0f * M_PI;
        glVertex2f(0.2f + cos(angle) * 0.06f, 0.85f + sin(angle) * 0.03f);
    }
    glEnd();
    
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.25f, 0.87f);
    for (int i = 0; i <= 20; i++) {
        float angle = (float)i / 20.0f * 2.0f * M_PI;
        glVertex2f(0.25f + cos(angle) * 0.05f, 0.87f + sin(angle) * 0.025f);
    }
    glEnd();
    
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.55f, 0.9f);
    for (int i = 0; i <= 20; i++) {
        float angle = (float)i / 20.0f * 2.0f * M_PI;
        glVertex2f(0.55f + cos(angle) * 0.07f, 0.9f + sin(angle) * 0.035f);
    }
    glEnd();
    
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.6f, 0.88f);
    for (int i = 0; i <= 20; i++) {
        float angle = (float)i / 20.0f * 2.0f * M_PI;
        glVertex2f(0.6f + cos(angle) * 0.05f, 0.88f + sin(angle) * 0.025f);
    }
    glEnd();
    
    glColor3f(0.5f, 0.6f, 0.7f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0.0f, 0.3f);
    glVertex2f(0.2f, 0.55f);
    glVertex2f(0.4f, 0.3f);
    
    glVertex2f(0.3f, 0.3f);
    glVertex2f(0.5f, 0.6f);
    glVertex2f(0.7f, 0.3f);
    
    glVertex2f(0.6f, 0.3f);
    glVertex2f(0.85f, 0.5f);
    glVertex2f(1.0f, 0.3f);
    glEnd();
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void drawGround() {
    glDisable(GL_LIGHTING);
    
    glColor3f(0.3f, 0.6f, 0.3f);
    glBegin(GL_QUADS);
    glVertex3f(-40.0f, 0.0f, -40.0f);
    glVertex3f(-40.0f, 0.0f, 40.0f);
    glVertex3f(40.0f, 0.0f, 40.0f);
    glVertex3f(40.0f, 0.0f, -40.0f);
    glEnd();
    
    glColor3f(0.25f, 0.5f, 0.25f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (float x = -40.0f; x <= 40.0f; x += 2.0f) {
        glVertex3f(x, 0.01f, -40.0f);
        glVertex3f(x, 0.01f, 40.0f);
    }
    for (float z = -40.0f; z <= 40.0f; z += 2.0f) {
        glVertex3f(-40.0f, 0.01f, z);
        glVertex3f(40.0f, 0.01f, z);
    }
    glEnd();
    
    glEnable(GL_LIGHTING);
}

bool isTargetInAimLine(btRigidBody* target) {
    if (!slingshot.isPulling || !red || !red->getRigidBody()) return false;
    
    float restX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
    float restY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
    float restZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
    
    float dirX = restX - slingshot.pouchX;
    float dirY = restY - slingshot.pouchY;
    float dirZ = restZ - slingshot.pouchZ;
    
    float length = sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ);
    if (length < 0.01f) return false;
    
    dirX /= length;
    dirY /= length;
    dirZ /= length;
    
    btTransform trans;
    target->getMotionState()->getWorldTransform(trans);
    btVector3 targetPos = trans.getOrigin();
    
    float toTargetX = targetPos.x() - slingshot.pouchX;
    float toTargetY = targetPos.y() - slingshot.pouchY;
    float toTargetZ = targetPos.z() - slingshot.pouchZ;
    
    float targetDist = sqrt(toTargetX*toTargetX + toTargetY*toTargetY + toTargetZ*toTargetZ);
    if (targetDist < 0.01f) return false;
    
    toTargetX /= targetDist;
    toTargetY /= targetDist;
    toTargetZ /= targetDist;
    
    float dot = dirX * toTargetX + dirY * toTargetY + dirZ * toTargetZ;
    
    return dot > 0.95f;
}

void drawAimLine() {
    if (slingshot.isPulling && red && red->getRigidBody()) {
        glDisable(GL_LIGHTING);
        
        float restX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
        float restY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
        float restZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
        
        float dirX = restX - slingshot.pouchX;
        float dirY = restY - slingshot.pouchY;
        float dirZ = restZ - slingshot.pouchZ;
        
        float length = sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ);
        if (length > 0.01f) {
            dirX /= length;
            dirY /= length;
            dirZ /= length;
            
            glColor4f(1.0f, 1.0f, 0.0f, 0.8f);
            glLineWidth(3.0f);
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(3, 0xAAAA);
            
            glBegin(GL_LINES);
            glVertex3f(slingshot.pouchX, slingshot.pouchY, slingshot.pouchZ);
            glVertex3f(slingshot.pouchX + dirX * 15.0f, 
                      slingshot.pouchY + dirY * 15.0f, 
                      slingshot.pouchZ + dirZ * 15.0f);
            glEnd();
            
            glDisable(GL_LINE_STIPPLE);
        }
        
        glEnable(GL_LIGHTING);
    }
}

void drawHUD() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(10, HEIGHT - 140);
    glVertex2f(350, HEIGHT - 140);
    glVertex2f(350, HEIGHT - 10);
    glVertex2f(10, HEIGHT - 10);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(20, HEIGHT - 30);
    char scoreText[50];
    sprintf(scoreText, "Pontos: %d", score);
    for (char* c = scoreText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    glRasterPos2f(20, HEIGHT - 60);
    char shotsText[50];
    sprintf(shotsText, "Tiros: %d", shotsRemaining);
    for (char* c = shotsText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    glColor3f(0.8f, 0.8f, 0.8f);
    glRasterPos2f(20, HEIGHT - 90);
    char* controlText1 = (char*)"Q/E: Aumentar/Diminuir Profundidade";
    for (char* c = controlText1; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    glRasterPos2f(20, HEIGHT - 110);
    char depthText[50];
    // MODIFICADO: Não podemos mais ler 'pullDepth' globalmente.
    // Esta é uma limitação da refatoração, a menos que
    // adicionemos um 'getPullDepth()' ao manager.
    // Por enquanto, vamos remover esta linha ou comentá-la.
    // sprintf(depthText, "Profundidade: %.1f", pullDepth); // ANTIGO
    sprintf(depthText, "Profundidade: (pressione Q/E)"); // NOVO
    for (char* c = depthText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    if (gameOver) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
        glBegin(GL_QUADS);
        glVertex2f(WIDTH/2 - 200, HEIGHT/2 - 100);
        glVertex2f(WIDTH/2 + 200, HEIGHT/2 - 100);
        glVertex2f(WIDTH/2 + 200, HEIGHT/2 + 100);
        glVertex2f(WIDTH/2 - 200, HEIGHT/2 + 100);
        glEnd();
        
        glColor3f(1.0f, 1.0f, 0.0f);
        glRasterPos2f(WIDTH/2 - 80, HEIGHT/2 + 30);
        char* gameOverText = (char*)"FIM DE JOGO!";
        for (char* c = gameOverText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
        }
        
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(WIDTH/2 - 100, HEIGHT/2 - 10);
        char finalScoreText[50];
        sprintf(finalScoreText, "Pontuacao Final: %d", score);
        for (char* c = finalScoreText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
        
        glRasterPos2f(WIDTH/2 - 120, HEIGHT/2 - 50);
        char* restartText = (char*)"Pressione R para reiniciar";
        for (char* c = restartText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
    }
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void updateElasticPhysics() {
    if (!slingshot.isPulling && red && red->getRigidBody()) {
        float restX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
        float restY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
        float restZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
        
        float dx = slingshot.pouchX - restX;
        float dy = slingshot.pouchY - restY;
        float dz = slingshot.pouchZ - restZ;
        
        float k = 25.0f;
        static float vx = 0.0f, vy = 0.0f, vz = 0.0f;
        float damping = 0.88f;
        
        float fx = -k * dx;
        float fy = -k * dy;
        float fz = -k * dz;
        
        float mass = 0.08f;
        float ax = fx / mass;
        float ay = fy / mass;
        float az = fz / mass;
        
        float dt = 0.016f;
        vx += ax * dt;
        vy += ay * dt;
        vz += az * dt;
        
        vx *= damping;
        vy *= damping;
        vz *= damping;
        
        slingshot.pouchX += vx * dt;
        slingshot.pouchY += vy * dt;
        slingshot.pouchZ += vz * dt;
        
        float distance = sqrt(dx*dx + dy*dy + dz*dz);
        if (distance < 0.05f && sqrt(vx*vx + vy*vy + vz*vz) < 0.3f) {
            slingshot.pouchX = restX;
            slingshot.pouchY = restY;
            slingshot.pouchZ = restZ;
            vx = vy = vz = 0.0f;
        }
    }
}

void updatePouchPosition() {
    if (mousePressed && slingshot.isPulling) {
        int deltaX_pixels = mouseX - startMouseX;
        int deltaY_pixels = mouseY - startMouseY;
        
        float sensitivityX = 0.04f;
        float sensitivityY = 0.04f;
        
        float dx = (float)deltaX_pixels * sensitivityX;
        float dy = (float)-deltaY_pixels * sensitivityY;
        float dz = pullDepth;
        
        float maxPull = 7.0f;
        float currentDist = sqrt(dx*dx + dy*dy + dz*dz);
        
        if (currentDist > maxPull) {
            dx = (dx / currentDist) * maxPull;
            dy = (dy / currentDist) * maxPull;
            dz = (dz / currentDist) * maxPull;
        }
        
        slingshot.pouchX = startPouchX + dx;
        slingshot.pouchY = startPouchY + dy;
        slingshot.pouchZ = startPouchZ + dz;
        
        if (red->getRigidBody()) {
            btTransform trans;
            red->getRigidBody()->getMotionState()->getWorldTransform(trans);
            trans.setOrigin(btVector3(slingshot.pouchX, slingshot.pouchY, slingshot.pouchZ));
            red->getRigidBody()->getMotionState()->setWorldTransform(trans);
            red->getRigidBody()->setWorldTransform(trans);
        }
    }
}

void screenToWorld(int screenX, int screenY, float depth, float& worldX, float& worldY, float& worldZ) {
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLdouble posX, posY, posZ;
    
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    GLfloat winX = (float)screenX;
    GLfloat winY = (float)viewport[3] - (float)screenY;
    
    gluUnProject(winX, winY, depth, modelview, projection, viewport, &posX, &posY, &posZ);
    
    worldX = posX;
    worldY = posY;
    worldZ = posZ;
}

void mouse(int button, int state, int x, int y) {
    // Delega o evento para o gerenciador
    if (g_slingshotManager) {
        g_slingshotManager->handleMouseClick(button, state, x, y);
    }
}

void mouseMotion(int x, int y) {
    // Delega o evento para o gerenciador
    if (g_slingshotManager) {
        g_slingshotManager->handleMouseDrag(x, y);
    }
}

void passiveMouseMotion(int x, int y) {
    // Delega o evento para o gerenciador
    if (g_slingshotManager) {
        g_slingshotManager->handlePassiveMouseMotion(x, y);
    }
}

void keyboard(unsigned char key, int x, int y) {
    // Primeiro, delega as teclas do estilingue
    if (g_slingshotManager) {
        g_slingshotManager->handleKeyboard(key);
    }

    // Teclas globais do jogo
    switch(key) {
        case 27: // ESC
            exit(0);
            break;
            
        // Q e E foram movidos para o handleKeyboard do manager
            
        case 'r':
        case 'R':
            // Lógica de reset global
            score = 0;
            shotsRemaining = 8;
            gameOver = false;
            
            for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
                btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
                btRigidBody* body = btRigidBody::upcast(obj);
                
                if (body && body->getInvMass() != 0) {
                    dynamicsWorld->removeRigidBody(body);
                    delete body->getMotionState();
                    delete body;
                }
            }
            
            initBullet(); // Recria os alvos
            
            // NOVO: Reseta o estado do estilingue
            if (g_slingshotManager) {
                g_slingshotManager->reset();
            }
            
            printf("Jogo reiniciado!\n");
            break;
    }
}

void specialKeys(int key, int x, int y) {
    // Controles da câmera permanecem globais
    switch(key) {
        case GLUT_KEY_LEFT:
            cameraAngleH -= 3.0f;
            break;
        case GLUT_KEY_RIGHT:
            cameraAngleH += 3.0f;
            break;
        case GLUT_KEY_UP:
            cameraAngleV += 2.0f;
            if (cameraAngleV > 60.0f) cameraAngleV = 60.0f;
            break;
        case GLUT_KEY_DOWN:
            cameraAngleV -= 2.0f;
            if (cameraAngleV < 5.0f) cameraAngleV = 5.0f;
            break;
        case GLUT_KEY_PAGE_UP:
            cameraDistance -= 1.0f;
            if (cameraDistance < 8.0f) cameraDistance = 8.0f;
            break;
        case GLUT_KEY_PAGE_DOWN:
            cameraDistance += 1.0f;
            if (cameraDistance > 30.0f) cameraDistance = 30.0f;
            break;
    }
}

// --- MODIFICADO: Funções Principais (Display, Timer, Init) ---

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    drawSky();
    
    glLoadIdentity();
    
    float angleH_rad = cameraAngleH * M_PI / 180.0f;
    float angleV_rad = cameraAngleV * M_PI / 180.0f;
    
    float camX = cameraDistance * sin(angleH_rad) * cos(angleV_rad);
    float camY = cameraDistance * sin(angleV_rad);
    float camZ = cameraDistance * cos(angleH_rad) * cos(angleV_rad);
    
    gluLookAt(camX, camY + cameraTargetY, camZ,
              0.0f, cameraTargetY, -8.0f,
              0.0f, 1.0f, 0.0f);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat lightPos[] = {10.0f, 15.0f, 10.0f, 1.0f};
    GLfloat lightAmb[] = {0.4f, 0.4f, 0.4f, 1.0f};
    GLfloat lightDif[] = {1.0f, 1.0f, 0.9f, 1.0f};
    GLfloat lightSpec[] = {0.8f, 0.8f, 0.8f, 1.0f};
    
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
    
    drawGround();
    
    for (auto& tree : trees) {
        tree.draw();
    }
    
    // MODIFICADO: Chama o 'draw' do gerenciador
    if (g_slingshotManager) {
        g_slingshotManager->draw();
    }
    
    // Desenha todos os corpos rígidos no mundo
    // Desenhar o pássaro Red separadamente (ele gerencia sua própria transformação)
    if (red && red->getRigidBody()) {
        glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
        red->desenhar();
        red->setRotacaoVisual(0.0f, 1.0f, 0.0f, M_PI);
        // A rotação visual de 180° em Y já está configurada por padrão no construtor
        // Se quiser mudar: red->setRotacaoVisual(0.0f, 1.0f, 0.0f, M_PI);
    }
    
    // Desenhar outros objetos físicos (caixas, alvos)
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        
        if (body && body->getMotionState() && body->getInvMass() != 0) {
            // Pular o Red pois já foi desenhado acima
            if (red && body == red->getRigidBody()) {
                continue;
            }
            
            btTransform trans;
            body->getMotionState()->getWorldTransform(trans);
            btScalar m[16];
            trans.getOpenGLMatrix(m);
            
            glPushMatrix();
            glMultMatrixf(m);
            
            btCollisionShape* shape = body->getCollisionShape();
            
            // MODIFICADO: Verifica se o corpo é o projétil ATUAL
            btRigidBody* currentProjectile = g_slingshotManager ? g_slingshotManager->getProjectileInPouch() : nullptr;

            if (body == currentProjectile) {
                // É o projétil na malha (cinemático)
                glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
                
                if (projectileModelLoaded) {
                    glColor3f(0.8f, 0.2f, 0.2f);
                    glPushMatrix();
                    glRotatef(180, 0, 1, 0);
                    projectileModel.draw();
                    glPopMatrix();
                } else {
                    glColor3f(0.8f, 0.2f, 0.2f);
                    glutSolidSphere(0.3, 20, 20);
                }
            }
            else if (shape == projectileShape) {
                // É um projétil já lançado (dinâmico)
                glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);
                
                if (projectileModelLoaded) {
                    glColor3f(0.8f, 0.2f, 0.2f);
                    projectileModel.draw();
                } else {
                    glColor3f(0.8f, 0.2f, 0.2f);
                    glutSolidSphere(0.3, 16, 16);
                }
            }
            else if (shape == boxShape) {
                // É um alvo (caixa)
                glMaterialf(GL_FRONT, GL_SHININESS, 5.0f);
                
                // MODIFICADO: Chama a função de mira do gerenciador
                if (g_slingshotManager && g_slingshotManager->isTargetInAimLine(body)) {
                    glColor3f(0.2f, 1.0f, 0.2f);
                } else {
                    glColor3f(0.6f, 0.4f, 0.2f);
                }
                
                glutSolidCube(1.0);
                
                glDisable(GL_LIGHTING);
                glColor3f(0.3f, 0.2f, 0.1f);
                glLineWidth(2.0f);
                
                glBegin(GL_LINES);
                glVertex3f(-0.5f, 0.0f, -0.5f);
                glVertex3f(0.5f, 0.0f, -0.5f);
                glVertex3f(-0.5f, 0.0f, 0.5f);
                glVertex3f(0.5f, 0.0f, 0.5f);
                glEnd();
                
                glEnable(GL_LIGHTING);
            }
            
            glPopMatrix();
        }
    }
    
    glMaterialf(GL_FRONT, GL_SHININESS, 10.0f);
    
    // MODIFICADO: 'updateElasticPhysics' é chamado pelo 'update' do manager no 'timer'
    drawHUD();
    
    glutSwapBuffers();
}

void timer(int value) {
    // Simula a física
    dynamicsWorld->stepSimulation(1.0f / 60.0f, 10);
    
    // NOVO: Atualiza a física do estilingue (ex: malha voltando)
    if (g_slingshotManager) {
        g_slingshotManager->update();
    }
    
    // Lógica de pontuação (permanece global)
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        
        if (body && body->getCollisionShape() == boxShape) {
            btTransform trans;
            body->getMotionState()->getWorldTransform(trans);
            float y = trans.getOrigin().getY();
            
            if (y < -2.0f) {
                score += 100;
                
                auto it = std::find(targetBodies.begin(), targetBodies.end(), body);
                if (it != targetBodies.end()) {
                    targetBodies.erase(it);
                }
                
                dynamicsWorld->removeRigidBody(body);
                delete body->getMotionState();
                delete body;
            }
        }
    }
    
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(0.7f, 0.85f, 0.95f, 1.0f);
    
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    GLfloat spec[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT, GL_SHININESS, 20.0f);
    initBullet();
    
    // NOVO: Cria a instância do gerenciador do estilingue
    // Passamos ponteiros para os sistemas que ele precisa controlar
    g_slingshotManager = new SlingshotManager(dynamicsWorld, projectileShape, &shotsRemaining, &gameOver);
    slingshot.baseX = 0.0f;
    slingshot.baseY = 0.0f;
    slingshot.baseZ = 0.0f;
    
    slingshot.leftArmX = 0.0f;
    slingshot.leftArmY = 3.0f;
    slingshot.leftArmZ = 0.0f;
    
    slingshot.rightArmX = 0.0f;
    slingshot.rightArmY = 3.0f;
    slingshot.rightArmZ = 0.0f;
    
    float armHeight = 2.5f;
    float angle = 30.0f * M_PI / 180.0f;
    
    slingshot.leftTopX = slingshot.leftArmX + sin(angle) * armHeight;
    slingshot.leftTopY = slingshot.leftArmY + cos(angle) * armHeight;
    slingshot.leftTopZ = slingshot.leftArmZ;
    
    slingshot.rightTopX = slingshot.rightArmX - sin(angle) * armHeight;
    slingshot.rightTopY = slingshot.rightArmY + cos(angle) * armHeight;
    slingshot.rightTopZ = slingshot.rightArmZ;
    
    slingshot.pouchX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
    slingshot.pouchY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
    slingshot.pouchZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
    slingshot.isPulling = false;
    
    initBullet();    
    // Carregar modelo da Ã¡rvore
    printf("\nTentando carregar modelo OBJ da arvore...\n");
    const char* possiblePaths[] = {
        "Objetos/arvore2.obj",
        "./Objetos/arvore2.obj",
        "../Objetos/arvore2.obj",
        "arvore2.obj",
        "tree2.obj"
    };
    treeModelLoaded = false;
    for (const char* path : possiblePaths) {
        printf("  Tentando: %s\n", path);
        if (treeModel.loadFromFile(path)) {
            treeModelLoaded = true;
            printf("  ✓ Modelo carregado com sucesso de: %s\n", path);
            break;
        }
    }
    if (!treeModelLoaded) {
        printf("  Modelo OBJ nao encontrado em nenhum caminho. Usando geometria procedural.\n");
    }
    
    // ... (Impressão dos controles permanece a mesma) ...
    printf("\n=== ESTILINGUE 3D MELHORADO ===\n");
    printf("CONTROLES:\n");
    printf("  Mouse Esquerdo: Clique e arraste para mirar\n");
    printf("  Q: Aumentar profundidade (puxar para frente)\n");
    printf("  E: Diminuir profundidade (voltar)\n");
    printf("  Setas Esquerda/Direita: Rotacionar camera\n");
    printf("  Setas Cima/Baixo: Angulo vertical da camera\n");
    printf("  Page Up/Down: Zoom da camera\n");
    printf("  R: Reiniciar jogo\n");
    printf("  ESC: Sair\n");
    printf("\nRECURSOS:\n");
    printf("  - Arvores decorativas (modelo OBJ ou procedural)\n");
    printf("  - Projetil RED.OBJ (modelo 3D personalizado)\n");
    printf("  - Fundo com sol, nuvens e montanhas\n");
    printf("  - Alvos ficam VERDES quando na mira\n");
    printf("===================================\n\n");
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Estilingue 3D - Refatorado com Classe");
    
    // if (!init_audio()) return -1; // Exemplo de áudio
    // if (!load_sounds()) return -1; // Exemplo de áudio
    
    init();
    
    // CRÍTICO: Criar o objeto red ANTES de usar!
    red = new PassaroRed(0.0f, 0.0f, 0.0f);
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(passiveMouseMotion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(0, timer, 0);
    
    glutMainLoop();
    
    // Limpeza
    if (red) {
        delete red;  // O destrutor de Passaro já limpa a física
        red = nullptr;
    }
    
    // Limpa os target bodies
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        
        if (body && body->getMotionState()) {
            delete body->getMotionState();
        }
        dynamicsWorld->removeCollisionObject(obj);
        delete obj;
    }
    
    for (int i = 0; i < collisionShapes.size(); i++) {
        delete collisionShapes[i];
    }
    
    // NOVO: Limpa o gerenciador do estilingue
    delete g_slingshotManager;
    
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;

    // close_audio(); // Exemplo de áudio
    
    return 0;
}