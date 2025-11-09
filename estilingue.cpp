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

// Corrigido para 'loads.h' (minúsculo, como no arquivo fornecido)
#include "loads.h" 
#include "passaros/passaro.h"
#include "passaros/Red.h"
#include "SlingshotManager.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Includes de Áudio (exemplo) ---
// (espaço para seus includes de áudio)


const int WIDTH = 1280;
const int HEIGHT = 720;

// Mundo de física Bullet
btDiscreteDynamicsWorld* dynamicsWorld;
btAlignedObjectArray<btCollisionShape*> collisionShapes;
btCollisionShape* projectileShape = nullptr; // Mantido para o Bullet
btCollisionShape* boxShape = nullptr;

btBroadphaseInterface* broadphase = nullptr;
btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
btCollisionDispatcher* dispatcher = nullptr;
btSequentialImpulseConstraintSolver* solver = nullptr;

// Modelos OBJ globais
OBJModel treeModel;
bool treeModelLoaded = false;

// --- Classe Tree (sem alterações) ---
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
            // glColor3f(0.3f, 0.5f, 0.2f);
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


// --- Variáveis Globais ---
SlingshotManager* g_slingshotManager = nullptr; // Ponteiro para o gerenciador

// Variáveis globais de câmera e jogo
float cameraAngleH = 45.0f;
float cameraAngleV = 20.0f;
float cameraDistance = 18.0f;
float cameraTargetY = 3.0f;

int score = 0;
int shotsRemaining = 8;
bool gameOver = false;

std::vector<btRigidBody*> targetBodies;
PassaroRed* red; // Ponteiro para o nosso pássaro

// --- Funções do Jogo ---

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

    // O projectileShape ainda é necessário para o *conceito* do SlingshotManager,
    // embora estejamos passando o 'red'
    projectileShape = new btSphereShape(0.3f); // 'red' usa este raio
    collisionShapes.push_back(projectileShape);
    
    boxShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
    collisionShapes.push_back(boxShape);
    
    targetBodies.clear();
    
    // (Criação de caixas e árvores)
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

//
// --- [ INÍCIO DA ÁREA DE EXCLUSÃO ] ---
//
// As funções:
// - clearProjectile()
// - createProjectileInPouch()
// - drawCylinder()
// - drawWoodenBase()
// - drawElastic()
// - isTargetInAimLine()
// - drawAimLine()
// - updateElasticPhysics()
// - updatePouchPosition()
// - screenToWorld()
//
// ... FORAM REMOVIDAS DESTA ÁREA ...
//
// Elas agora são métodos privados da classe SlingshotManager.
//
// --- [ FIM DA ÁREA DE EXCLUSÃO ] ---
//


// --- Funções de Renderização da Cena ---

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
    
    // (Código do céu, nuvens, sol, montanhas - sem alterações)
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
    // CORRIGIDO: Usa texto estático, já que 'pullDepth' não é mais global.
    sprintf(depthText, "Profundidade: (pressione Q/E)"); 
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


// --- Callbacks do GLUT ---

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
    // Primeiro, delega as teclas do estilingue (Q/E)
    if (g_slingshotManager) {
        g_slingshotManager->handleKeyboard(key);
    }

    // Teclas globais do jogo
    switch(key) {
        case 27: // ESC
            exit(0);
            break;
            
        case 'r':
        case 'R':
            // Lógica de reset global
            score = 0;
            shotsRemaining = 8;
            gameOver = false;
            
            // Limpa os corpos dinâmicos (caixas, pássaros)
            for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
                btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
                btRigidBody* body = btRigidBody::upcast(obj);
                
                if (body && body->getInvMass() != 0) {
                    dynamicsWorld->removeRigidBody(body);
                    delete body->getMotionState();
                    delete body;
                }
            }
            
            // Recria o mundo, caixas e árvores
            initBullet(); 
            
            // Reseta o estado do estilingue
            if (g_slingshotManager) {
                g_slingshotManager->reset();
            }
            
            // Recria o pássaro 'red' (seu ponteiro foi limpo em clearProjectile)
            // A inicialização em initBullet já limpa o 'red'
            // Mas o 'red' em si precisa ser resetado
            if(red) {
                 red->resetar(0,0,0); // Posição inicial
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

// --- Funções Principais (Display, Timer, Init) ---

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    drawSky();
    
    glLoadIdentity();
    
    // Configuração da Câmera
    float angleH_rad = cameraAngleH * M_PI / 180.0f;
    float angleV_rad = cameraAngleV * M_PI / 180.0f;
    
    float camX = cameraDistance * sin(angleH_rad) * cos(angleV_rad);
    float camY = cameraDistance * sin(angleV_rad);
    float camZ = cameraDistance * cos(angleH_rad) * cos(angleV_rad);
    
    gluLookAt(camX, camY + cameraTargetY, camZ,
              0.0f, cameraTargetY, -8.0f,
              0.0f, 1.0f, 0.0f);
    
    // Configuração da Luz
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
    
    // --- Renderização da Cena ---
    
    drawGround();
    
    for (auto& tree : trees) {
        tree.draw();
    }
    
    // Desenha o estilingue (madeira e elásticos)
    if (g_slingshotManager) {
        g_slingshotManager->draw();
    }
    
    // Desenha o pássaro 'red'
    // O 'red->desenhar()' usa a matriz do seu 'btRigidBody'
    if (red && red->getRigidBody()) {
        glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
        red->desenhar(); // A classe Passaro cuida da sua própria matriz
        // Rotação visual padrão (opcional, se 'passaro.cpp' não fizer)
        // red->setRotacaoVisual(0.0f, 1.0f, 0.0f, M_PI);
    }
    
    // Desenha todos os outros corpos rígidos (as caixas)
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        
        if (body && body->getMotionState() && body->getInvMass() != 0) {
            
            // Pula o 'red', pois ele já foi desenhado acima
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
            
            // CORREÇÃO: Mudado de 'else if' para 'if'
            // Desenha apenas as caixas-alvo
            if (shape == boxShape) {
                glMaterialf(GL_FRONT, GL_SHININESS, 5.0f);
                
                // Verifica se o alvo está na mira
                if (g_slingshotManager && g_slingshotManager->isTargetInAimLine(body)) {
                    glColor3f(0.2f, 1.0f, 0.2f); // Verde
                } else {
                    glColor3f(0.6f, 0.4f, 0.2f); // Madeira
                }
                
                glutSolidCube(1.0);
                
                // (Contorno da caixa)
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
    
    drawHUD();
    
    glutSwapBuffers();
}

void timer(int value) {
    // Simula a física
    dynamicsWorld->stepSimulation(1.0f / 60.0f, 10);
    
    // Atualiza a física do estilingue (ex: malha voltando)
    if (g_slingshotManager) {
        g_slingshotManager->update();
    }
    
    // Lógica de pontuação (sem alterações)
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
    
    // 1. Inicializa a física
    initBullet();
    
    // 2. Cria a instância do gerenciador do estilingue
    //    (O ponteiro 'red' já foi criado na 'main')
    g_slingshotManager = new SlingshotManager(dynamicsWorld, red, &shotsRemaining, &gameOver);
    
    // 3. REMOVIDO: Bloco de inicialização da 'struct slingshot'
    
    // 4. Carregar modelo da árvore
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
        printf("  Modelo OBJ nao encontrado. Usando geometria procedural.\n");
    }
    
    // (Impressão dos controles)
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
    
    // (init_audio)
    
    // CRÍTICO: Criar o objeto 'red' ANTES de chamar init()
    red = new PassaroRed(0.0f, 0.0f, 0.0f);
    
    // Agora 'init()' pode usar o ponteiro 'red'
    init();
    
    // Configura os callbacks do GLUT
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(passiveMouseMotion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(0, timer, 0);
    
    glutMainLoop();
    
    // --- Limpeza ---
    if (red) {
        delete red;  // O destrutor de Passaro deve limpar a física
        red = nullptr;
    }
    
    // Limpa os corpos e shapes do Bullet
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
    
    // Limpa o gerenciador
    delete g_slingshotManager;
    
    // Limpa o mundo Bullet
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;

    // (close_audio)
    
    return 0;
}