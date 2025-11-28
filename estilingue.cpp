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

#include "stb/stb_image.h"
#include "util/loads.h" 
#include "passaros/passaro.h"
#include "passaros/Red.h"
#include "blocos/BlocoDestrutivel.h"
#include "estilingue/SlingshotManager.h"
#include "blocos/ParticleManager.h"
#include "controle_audio/audio_manager.h"
#include "menu/gameMenu.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Includes de Áudio (exemplo) ---
// (espaço para seus includes de áudio)
extern AudioManager g_audioManager;

const int WIDTH = 1280;
const int HEIGHT = 720;

// Mundo de física Bullet
btDiscreteDynamicsWorld* dynamicsWorld;
btAlignedObjectArray<btCollisionShape*> collisionShapes;
btCollisionShape* projectileShape = nullptr; // Mantido para o Bullet
btCollisionShape* boxShape = nullptr;
btRigidBody* groundRigidBody = nullptr;
btBroadphaseInterface* broadphase = nullptr;
btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
btCollisionDispatcher* dispatcher = nullptr;
btSequentialImpulseConstraintSolver* solver = nullptr;
ParticleManager g_particleManager;
// AudioManager g_audioManager;
// Modelos OBJ globais
OBJModel blockModel;
OBJModel treeModel;
bool treeModelLoaded = false;
bool blockModelLoaded = false;

//MENU
GameMenu* g_menu = nullptr; // Ponteiro para o menu
GameState g_currentState = STATE_MENU;


GLuint g_skyTextureID = 0;
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
            glEnable(GL_LIGHTING);
            glDisable(GL_COLOR_MATERIAL);
            glPushMatrix();
            glTranslatef(0.0f, 0.3f, 0.0f);
            treeModel.draw();
            glPopMatrix();
            glEnable(GL_COLOR_MATERIAL);
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
float cameraDistance = 28.0f;
float cameraTargetY = 3.0f;

int score = 0;
int shotsRemaining = 8;
bool gameOver = false;

std::vector<btRigidBody*> targetBodies;
PassaroRed* red; // Ponteiro para o nosso pássaro
std::vector<BlocoDestrutivel*> blocos;
// --- Funções do Jogo ---

btRigidBody* createTargetBox(float mass, const btVector3& position, const btQuaternion& rotation = btQuaternion(0, 0, 0, 1)) {
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(position);
    startTransform.setRotation(rotation); // <-- ÚNICA LINHA ADICIONADA/MODIFICADA
    
    btVector3 localInertia(0, 0, 0);
    if (mass != 0.f)
        boxShape->calculateLocalInertia(mass, localInertia);
    
    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, boxShape, localInertia);
    btRigidBody* body = new btRigidBody(rbInfo);
    
    // Configurações de CCD e física
    body->setCcdMotionThreshold(0.5f);
    body->setFriction(2.0f);
    body->setRestitution(0.1f);
    body->setDamping(0.3f, 0.3f);
    
    dynamicsWorld->addRigidBody(body);
    targetBodies.push_back(body);
    return body;
}

GLuint loadGlobalTexture(const char* filename) {
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); 
    
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data) {
        printf("Aviso: Nao foi possivel carregar a textura %s\n", filename);
        return 0; 
    }

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

    GLuint textureID;
    glGenTextures(1, &textureID); 
    glBindTexture(GL_TEXTURE_2D, textureID); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); // <-- MUDANÇA AQUI
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    printf("Textura do ceu carregada: %s (ID: %u)\n", filename, textureID);
    return textureID;
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
    
    // btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
    
    groundRigidBody = new btRigidBody(groundRigidBodyCI);

    groundRigidBody->setFriction(2.0f);

    // groundRigidBody->setFriction(0.8f);
    // groundRigidBody->setRestitution(0.3f);
    
    dynamicsWorld->addRigidBody(groundRigidBody);

    // O projectileShape ainda é necessário para o *conceito* do SlingshotManager,
    // embora estejamos passando o 'red'
    projectileShape = new btSphereShape(0.3f); // 'red' usa este raio
    collisionShapes.push_back(projectileShape);
    
    boxShape = new btBoxShape(btVector3(0.2f, 0.2f, 1.2f));
    collisionShapes.push_back(boxShape);
    
    targetBodies.clear();
//     float boxMass = 2.0f;
// float Y_BASE = 0.51f; 
//     float Y_NIVEL2 = 1.52f; // Base (0.5) + Altura (1.0) + Folga
//     float Y_NIVEL3 = 2.53f; // Nível 2 (1.5) + Altura (1.0) + Folga
//     float Y_NIVEL4 = 3.54f; // Nível 3 (2.5) + Altura (1.0) + Folga

//     // --- Definindo a Posição e Rotações ---
//     btVector3 centroTorre(0.0f, 0.0f, -15.0f); // Posição central da construção
//     float offset = 2.5f; // Distância do centro para os blocos
    
//     // Rotação Padrão (longo no eixo Z, como no seu btBoxShape)
//     btQuaternion rotZ(0, 0, 0, 1);
//     // Rotação 90 graus (gira em torno do Y para ficar longo no eixo X)
//     btQuaternion rotX(btVector3(0, 1, 0), M_PI / 2.0); 

//     // --- Construindo a Torre ---
//     // Nível 1 (Base, longo em Z)
//     createTargetBox(boxMass, centroTorre + btVector3(-offset, Y_BASE, 0), rotZ);
//     createTargetBox(boxMass, centroTorre + btVector3( offset, Y_BASE, 0), rotZ);

//     // Nível 2 (Meio, longo em X)
//     createTargetBox(boxMass, centroTorre + btVector3(0, Y_NIVEL2, -offset), rotX);
//     createTargetBox(boxMass, centroTorre + btVector3(0, Y_NIVEL2,  offset), rotX);

//     // Nível 3 (Meio, longo em Z)
//     createTargetBox(boxMass, centroTorre + btVector3(-offset, Y_NIVEL3, 0), rotZ);
//     createTargetBox(boxMass, centroTorre + btVector3( offset, Y_NIVEL3, 0), rotZ);

//     // Nível 4 (Topo, longo em X)
//     createTargetBox(boxMass, centroTorre + btVector3(0, Y_NIVEL4, -offset), rotX);
//     createTargetBox(boxMass, centroTorre + btVector3(0, Y_NIVEL4,  offset), rotX);
    float boxMass = 8.0f; // (Este valor agora é definido dentro da classe)

    // Definições da construção
    float H = 1.0f;  float G = 0.02f;
    float Y_NIVEL1 = 0.5f + G*1; float Y_NIVEL2 = Y_NIVEL1 + H + G;
    btVector3 centro(0.0f, 0.0f, -20.0f);
    float L = 3.0f + G; float W = 0.5f + G;
    btQuaternion rotZ(0, 0, 0, 1);
    btQuaternion rotX(btVector3(0, 1, 0), M_PI / 2.0);
    
    // (Caminhos para os seus modelos .obj)
    const char* modeloBarra = "Objetos/bloco_barra2.obj";
    const char* modeloPlaca = "Objetos/bloco_placa.obj"; // (Exemplo)

    float blockW = 1.0f; 
    float blockH = 6.0f; 
    float blockD = 1.0f; 

    // --- Definições de Layout Uniforme (Uma Parede) ---
    int numBlocos = 12; 
    float espacamentoX = blockW + 0.5f; 
    
    // CORREÇÃO AQUI: REMOVA 'float'
    Y_NIVEL1 = (blockH/ 2.0f) + 0.02f; // (0.25f + 0.02f = 0.27f)
    
    btVector3 centroParede(0.0f, 0.0f, -15.0f); 
    
    // CORREÇÃO AQUI: REMOVA 'btQuaternion'
    rotZ = btQuaternion(0, 0, 0, 1);

    // Calcula o X inicial para centralizar a parede
    float startX = -(float(numBlocos - 1) * espacamentoX) / 2.0f;

    // Array de materiais para variar
    MaterialTipo materiais[] = { MaterialTipo::MADEIRA, MaterialTipo::GELO, MaterialTipo::PEDRA };
    int numMateriais = 3;

    // --- Loop para criar os 12 blocos ---
    for (int i = 0; i < numBlocos; i++) {
        // Escolhe um material (Madeira, Gelo, Pedra, Madeira, Gelo...)
        MaterialTipo tipo = materiais[i % numMateriais]; 
        
        // Calcula a posição X deste bloco
        float posX = startX + (i * espacamentoX);
        
        // 1. Cria o novo bloco

        BlocoDestrutivel* bloco = new BlocoDestrutivel(tipo, modeloBarra, blockW, blockH, blockD);
        
        // 2. Inicializa a física na posição correta
        bloco->inicializarFisica(dynamicsWorld, centroParede + btVector3(posX, Y_NIVEL1, 0.0f), rotZ);
        
        // 3. Adiciona à lista
        blocos.push_back(bloco);
    }

    // NÍVEL 3: Teto (Gelo - Placa)
    // (Criando uma placa de 6x1x6)
    // BlocoDestrutivel* b4 = new BlocoDestrutivel(MaterialTipo::GELO, modeloBarra, 6.0f, 1.0f, 6.0f);
    // b4->inicializarFisica(dynamicsWorld, centro + btVector3(-L/2, Y_NIVEL3, 0), rotX);
    // blocos.push_back(b4);
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

// void drawSky() {
//     glDisable(GL_LIGHTING);
//     glDisable(GL_DEPTH_TEST);
    
//     glMatrixMode(GL_PROJECTION);
//     glPushMatrix();
//     glLoadIdentity();
//     glOrtho(0, 1, 0, 1, -1, 1);
    
//     glMatrixMode(GL_MODELVIEW);
//     glPushMatrix();
//     glLoadIdentity();
    
//     // (Código do céu, nuvens, sol, montanhas - sem alterações)
//     glBegin(GL_QUADS);
//     glColor3f(0.4f, 0.6f, 0.9f);
//     glVertex2f(0, 1);
//     glVertex2f(1, 1);
//     glColor3f(0.7f, 0.85f, 0.95f);
//     glVertex2f(1, 0.3f);
//     glVertex2f(0, 0.3f);
//     glEnd();
    
//     glColor3f(1.0f, 0.95f, 0.7f);
//     glBegin(GL_TRIANGLE_FAN);
//     glVertex2f(0.8f, 0.8f);
//     for (int i = 0; i <= 20; i++) {
//         float angle = (float)i / 20.0f * 2.0f * M_PI;
//         glVertex2f(0.8f + cos(angle) * 0.08f, 0.8f + sin(angle) * 0.08f);
//     }
//     glEnd();
    
//     glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
    
//     glBegin(GL_TRIANGLE_FAN);
//     glVertex2f(0.2f, 0.85f);
//     for (int i = 0; i <= 20; i++) {
//         float angle = (float)i / 20.0f * 2.0f * M_PI;
//         glVertex2f(0.2f + cos(angle) * 0.06f, 0.85f + sin(angle) * 0.03f);
//     }
//     glEnd();
    
//     glBegin(GL_TRIANGLE_FAN);
//     glVertex2f(0.25f, 0.87f);
//     for (int i = 0; i <= 20; i++) {
//         float angle = (float)i / 20.0f * 2.0f * M_PI;
//         glVertex2f(0.25f + cos(angle) * 0.05f, 0.87f + sin(angle) * 0.025f);
//     }
//     glEnd();
    
//     glBegin(GL_TRIANGLE_FAN);
//     glVertex2f(0.55f, 0.9f);
//     for (int i = 0; i <= 20; i++) {
//         float angle = (float)i / 20.0f * 2.0f * M_PI;
//         glVertex2f(0.55f + cos(angle) * 0.07f, 0.9f + sin(angle) * 0.035f);
//     }
//     glEnd();
    
//     glBegin(GL_TRIANGLE_FAN);
//     glVertex2f(0.6f, 0.88f);
//     for (int i = 0; i <= 20; i++) {
//         float angle = (float)i / 20.0f * 2.0f * M_PI;
//         glVertex2f(0.6f + cos(angle) * 0.05f, 0.88f + sin(angle) * 0.025f);
//     }
//     glEnd();
    
//     glColor3f(0.5f, 0.6f, 0.7f);
//     glBegin(GL_TRIANGLES);
//     glVertex2f(0.0f, 0.3f);
//     glVertex2f(0.2f, 0.55f);
//     glVertex2f(0.4f, 0.3f);
    
//     glVertex2f(0.3f, 0.3f);
//     glVertex2f(0.5f, 0.6f);
//     glVertex2f(0.7f, 0.3f);
    
//     glVertex2f(0.6f, 0.3f);
//     glVertex2f(0.85f, 0.5f);
//     glVertex2f(1.0f, 0.3f);
//     glEnd();
    
//     glPopMatrix();
//     glMatrixMode(GL_PROJECTION);
//     glPopMatrix();
//     glMatrixMode(GL_MODELVIEW);
    
//     glEnable(GL_DEPTH_TEST);
//     glEnable(GL_LIGHTING);
// }
void drawSky() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST); // O céu não deve testar profundidade
    glDepthMask(GL_FALSE);    // O céu não deve ESCREVER na profundidade

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_skyTextureID);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // Aumenta a área de visualização verticalmente para "dar zoom out" no céu
    // Aumente o segundo parâmetro (top) para ver mais céu.
    glOrtho(0, 1, 0, 1.2, -1, 1); // Exemplo: 0 a 1.5 no Y para "zoom out"
                                  // Se 1.5 for demais, tente 1.2 ou 1.3

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f); // Cor branca (para não manchar a textura)

    glBegin(GL_QUADS);
        // Canto inferior esquerdo (onde o chão encontra o céu)
        glTexCoord2f(0, 0); glVertex2f(0, 0); 
        // Canto inferior direito
        glTexCoord2f(1, 0); glVertex2f(1, 0); 
        // Canto superior direito
        // Ajuste o glTexCoord2f(..., Y_TEXTURA_MAX) para puxar mais céu para baixo
        // Se a imagem tiver mais céu na parte de cima que você quer mostrar:
        glTexCoord2f(1, 1); glVertex2f(1, 1.5); // O 1.5 aqui corresponde ao glOrtho
        // Canto superior esquerdo
        glTexCoord2f(0, 1); glVertex2f(0, 1.5); // O 1.5 aqui corresponde ao glOrtho
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE); // Habilita a escrita na profundidade novamente
    glEnable(GL_DEPTH_TEST); // Habilita o teste de profundidade novamente
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
//modificações para mostrar a tela inicial
void mouse(int button, int state, int x, int y) {
    if (g_currentState == STATE_GAME) {
        // Jogo normal
        if (g_slingshotManager) {
            g_slingshotManager->handleMouseClick(button, state, x, y);
        }
    } 
    else {
        // Menu
        if (g_menu) {
            GameState newState = g_menu->handleMouseClick(button, state, x, y, g_currentState);
            
            if (newState == STATE_EXIT) {
                exit(0);
            }
            g_currentState = newState;
        }
    }
}

void mouseMotion(int x, int y) {
    if (g_currentState == STATE_GAME) {
        if (g_slingshotManager) g_slingshotManager->handleMouseDrag(x, y);
    } else {
        if (g_menu) g_menu->handleMouseMotion(x, y, g_currentState);
    }
}

void passiveMouseMotion(int x, int y) {
    if (g_currentState == STATE_GAME) {
        if (g_slingshotManager) g_slingshotManager->handlePassiveMouseMotion(x, y);
    } else {
        if (g_menu) g_menu->handleMouseMotion(x, y, g_currentState);
    }
}

void keyboard(unsigned char key, int x, int y) {
    // --- 1. Lógica Global de Navegação (ESC) ---
    if (key == 27) { // Tecla ESC
        if (g_currentState == STATE_GAME) {
            g_currentState = STATE_MENU; // Pausa o jogo e abre o menu
            return;
        } 
        else if (g_currentState == STATE_SETTINGS) {
            g_currentState = STATE_MENU; // Volta das configurações para o menu principal
            return;
        } 
        else if (g_currentState == STATE_MENU) {
            exit(0); // Se já estiver no menu principal, fecha o programa
        }
    }

    // --- 2. Controles de Jogo (Só funcionam se estiver jogando) ---
    if (g_currentState == STATE_GAME) {
        
        // Delega as teclas do estilingue (Q/E)
        if (g_slingshotManager) {
            g_slingshotManager->handleKeyboard(key);
        }

        switch(key) {
            case 'r':
            case 'R':
                // --- Lógica de Reset (Mantida do seu código original) ---
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
                
                // Recria o pássaro 'red'
                if(red) {
                     red->resetar(0,0,0); // Posição inicial
                }

                printf("Jogo reiniciado!\n");
                break;
        }
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
glEnable(GL_LIGHTING); //
    glEnable(GL_LIGHT0);
    
    GLfloat lightPos[] = {10.0f, 15.0f, 10.0f, 1.0f};
    
    // MUDE ISTO (Deixa as sombras mais claras):
    // DE: GLfloat lightAmb[] = {0.4f, 0.4f, 0.4f, 1.0f};
    GLfloat lightAmb[] = {0.8f, 0.8f, 0.8f, 1.0f};

    // MUDE ISTO (Mantém a luz principal clara):
    // DE: GLfloat lightDif[] = {1.0f, 1.0f, 0.9f, 1.0f};
    GLfloat lightDif[] = {1.0f, 1.0f, 1.0f, 1.0f};

    // MUDE ISTO (Remove o brilho "realista"):
    // DE: GLfloat lightSpec[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat lightSpec[] = {0.0f, 0.0f, 0.0f, 1.0f};
    
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
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
    
    //desenha os blocos
    //corrigi o bug da hitbox invisivel 
    for (auto& bloco : blocos) {
        glPushMatrix();
        // glTranslatef(0.0f, -2.5f, 0.0f);
        bloco->desenhar();
        glPopMatrix();
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
                
                if (blockModelLoaded) {
                    // Se o modelo carregou, desenha o .obj
                    glScalef(5.0, 5.0, 5.0);
                    blockModel.draw();
                } else {
                    // Senão, desenha o cubo antigo como fallback
                    glutSolidCube(1.0);
                }
                
                // (Contorno da caixa)
                // glDisable(GL_LIGHTING);
                // glColor3f(0.3f, 0.2f, 0.1f);
                // glLineWidth(2.0f);
                // glBegin(GL_LINES);
                // glVertex3f(-0.5f, 0.0f, -0.5f);
                // glVertex3f(0.5f, 0.0f, -0.5f);
                // glVertex3f(-0.5f, 0.0f, 0.5f);
                // glVertex3f(0.5f, 0.0f, 0.5f);
                // glEnd();
                // glEnable(GL_LIGHTING);
            }
            
            glPopMatrix();
        }
    }
    
    glMaterialf(GL_FRONT, GL_SHININESS, 10.0f);

    g_particleManager.draw();
    

    if (g_currentState == STATE_GAME) {
        drawHUD(); // Só desenha o HUD (pontos, mira) se estiver jogando
        if (g_slingshotManager) g_slingshotManager->draw(); // Desenha estilingue
    } 
    else {
        // Se NÃO estiver jogando (Menu ou Settings), desenha o menu por cima
        if (g_menu) g_menu->draw(g_currentState);
    }
    
    glutSwapBuffers();
}

void timer(int value) {
    if (g_currentState == STATE_GAME)
    {  
        float deltaTime = 1.0f / 60.0f;
    
        // Simula a física
        dynamicsWorld->stepSimulation(1.0f / 60.0f, 10, 1.0f / 180.0f);
        
        for (auto& bloco : blocos) {
            bloco->update(deltaTime);
            bloco->clearContactFlag();
        }
    
        g_particleManager.update(deltaTime);
    
        //bloco adicionado para lidar com a colisao dos blocos
        int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
        for (int i = 0; i < numManifolds; i++) {
            btPersistentManifold* contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
            const btCollisionObject* obA = contactManifold->getBody0();
            const btCollisionObject* obB = contactManifold->getBody1();
    
            // Tenta encontrar o pássaro e um bloco
            BlocoDestrutivel* bloco = nullptr;
            
            for (auto& b : blocos) {
                if (b->getRigidBody() == obA || b->getRigidBody() == obB) {
                    bloco = b;
                    break;
                }
            }
            
            bool passaroEnvolvido = (obA == red->getRigidBody() || obB == red->getRigidBody());
    
            // Se o pássaro colidiu com um bloco
            if (bloco && passaroEnvolvido) {
                float impulsoTotal = 0;
                if (bloco->registerContact()){
                    // g_audioManager.playPassaro(SomTipo::COLISAO_PASSARO);
                    // g_audioManager.playColisao(bloco->getTipo(),70);
                    bloco->clearContactFlag();
                }
                
                for (int j = 0; j < contactManifold->getNumContacts(); j++) {
                    impulsoTotal += contactManifold->getContactPoint(j).getAppliedImpulse();
                }
                
                // Converte o impulso da física em "dano"
                float dano = impulsoTotal * 0.1f; 
                if (dano > 0.5f) { // Limite mínimo para registrar dano
                    bloco->aplicarDano(dano);
                }
    
            }
            bool blocoEnvolvido = (bloco != nullptr);
            bool chaoEnvolvido = (obA == groundRigidBody || obB == groundRigidBody);
    
            if (blocoEnvolvido && chaoEnvolvido) {
                float impulsoTotal = 0;
                for (int j = 0; j < contactManifold->getNumContacts(); j++) {
                    impulsoTotal += contactManifold->getContactPoint(j).getAppliedImpulse();
                }
                float dano = impulsoTotal * 0.1f; // Ajuste o fator conforme a força desejada
                if (dano > 0.5f) { // Limite mínimo para registrar dano
                    bloco->aplicarDano(dano);
                }
            }
        }
    
        // 2. Limpa blocos destruídos
        for (int i = blocos.size() - 1; i >= 0; i--) {
            if (blocos[i]->isDestruido()) {
                blocos[i]->limparFisica(dynamicsWorld);
                delete blocos[i];
                blocos.erase(blocos.begin() + i);
                // score += blocos[i]->getPontuacao(); // Adiciona pontuação
            }
        }
    
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
    glShadeModel(GL_FLAT);
    glEnable(GL_LINE_SMOOTH);
    glDisable(GL_CULL_FACE);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(0.7f, 0.85f, 0.95f, 1.0f);
    
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    GLfloat spec[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT, GL_SHININESS, 20.0f);
    

    g_skyTextureID = loadGlobalTexture("Objetos/texturas/fundo_ceu_borrado.png"); 
    if (g_skyTextureID == 0) {
        printf("ERRO: Falha ao carregar a textura do ceu.\n");
    }
    // 1. Inicializa a física
    initBullet();

    g_menu = new GameMenu(WIDTH, HEIGHT);
    
    //carrega as texturas dos blocos
            for (auto& bloco : blocos) {
                bloco->carregarTexturas();
            }

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
        "tree3.obj"
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
    
    //bloco do tipo barra
    printf("\nTentando carregar modelo OBJ do bloco...\n");
    const char* blockPaths[] = {
        "Objetos/bloco.obj",
        "./Objetos/bloco.obj",
        "../Objetos/bloco.obj",
        "bloco.obj"
    };
    blockModelLoaded = false;
    for (const char* path : blockPaths) {
        printf("  Tentando: %s\n", path);
        if (blockModel.loadFromFile(path)) {
            blockModelLoaded = true;
            printf("  ✓ Modelo do bloco carregado de: %s\n", path);
            break;
        }
    }
    if (!blockModelLoaded) {
        printf("  Aviso: Modelo OBJ do bloco nao encontrado. Usando cubo procedural.\n");
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

    if (!g_audioManager.initAudio()) {
        printf("AVISO: Audio desabilitado devido a falha na inicializacao.\n");
    }
    
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
    
    g_audioManager.cleanup();
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