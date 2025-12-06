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
#include "passaros/chuck.h"
#include "passaros/bomb.h"
#include "passaros/blue.h"
#include "blocos/BlocoDestrutivel.h"
#include "porcos/porco.h"
//manager do estilingue
#include "estilingue/SlingshotManager.h"
#include "canhao/canhao.h"
#include "blocos/ParticleManager.h"
//manager do audio
#include "controle_audio/audio_manager.h"
//menu inicial 
#include "menu/gameMenu.h"
//tela de carregamento 
#include "menu/LoadingScreen.h" // <--- Adicione isto
//elementos do cenario
#include "cenario/Tree.h"
//manager da iluminação 
#include "cenario/LightingManager.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Includes de Áudio (exemplo) ---
// (espaço para seus includes de áudio)
extern AudioManager g_audioManager;

// Vetor para pássaros extras (habilidade do Blue)
std::vector<Passaro*> extraBirds;

const int WIDTH = 1280;
const int HEIGHT = 720;


// --- VARIÁVEIS GLOBAIS NOVAS ---
//variaveis para o frame do jogo 
int frameCount = 0;
int previousTime = 0;
float fps = 0.0f;
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

//vetor para armazenar as arvores 
std::vector<Tree> trees;
// --- Variáveis Globais ---
SlingshotManager* g_slingshotManager = nullptr; // ponteiro para o gerenciador do estilingue
LightingManager g_lightingManager; //manager da iluuminação
// Variáveis globais de câmera e jogo
float cameraAngleH = 0.0f;
float cameraAngleV = 5.0f;
float cameraDistance = 33.5f;
float cameraTargetY = 3.0f;

int score = 0;
int shotsRemaining = 8;
bool gameOver = false;

std::vector<btRigidBody*> targetBodies;
std::vector<Passaro*>::iterator itPassaroAtual; // Iterador para a fila
Passaro* passaroAtual = nullptr;
Cannon* canhao = nullptr;
// Fila de pássaros
std::vector<Passaro*> filaPassaros;
std::vector<BlocoDestrutivel*> blocos;
std::vector<Porco*> porcos;
std::vector<Cannon*> canhoes;
// --- Funções do Jogo ---el*> blocos;
// --- Funções do Jogo ---

void proximoPassaro() {
    // Avança para o próximo pássaro na fila
    itPassaroAtual++;
    
    // Verifica se ainda existem pássaros na fila
    if (itPassaroAtual != filaPassaros.end()) {
        passaroAtual = *itPassaroAtual;
        
        // Reseta a posição do novo pássaro (por segurança)
        passaroAtual->resetar(0, 0, 0);
        
        // Atualiza o gerenciador do estilingue com o novo pássaro
        if (g_slingshotManager) {
            g_slingshotManager->setProjectile(passaroAtual);
        }
        
        printf("Proximo passaro carregado: %s\n", passaroAtual->getTipo().c_str());
    } else {
        // Acabaram os pássaros
        passaroAtual = nullptr;
        if (g_slingshotManager) {
            g_slingshotManager->setProjectile(nullptr);
        }
        printf("Fim dos passaros!\n");
        
        // Se ainda houver alvos, é Game Over (Derrota)
        // Se não houver alvos, a vitória já deve ter sido detectada em outro lugar
        if (!targetBodies.empty()) {
            gameOver = true;
        }
    }
}

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

void cleanupBullet() {
    // Limpa pássaros extras
    for (auto* bird : extraBirds) {
        delete bird;
    }
    extraBirds.clear();

    // Limpa o mundo Bullet antigo para evitar vazamentos e referências inválidas
    if (dynamicsWorld) {
        // Remove todos os objetos
        for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
            btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState()) {
                delete body->getMotionState();
            }
            dynamicsWorld->removeCollisionObject(obj);
            delete obj;
        }
        delete dynamicsWorld;
        dynamicsWorld = nullptr;
    }
    if (solver) { delete solver; solver = nullptr; }
    if (dispatcher) { delete dispatcher; dispatcher = nullptr; }
    if (collisionConfiguration) { delete collisionConfiguration; collisionConfiguration = nullptr; }
    if (broadphase) { delete broadphase; broadphase = nullptr; }
    
    collisionShapes.clear();
    targetBodies.clear();
    // groundRigidBody, projectileShape, boxShape são deletados no loop de objetos ou limpos acima
    groundRigidBody = nullptr;
    projectileShape = nullptr;
    boxShape = nullptr;
}

void initBullet() {
    // Garante que está limpo antes de iniciar
    if (dynamicsWorld) {
        cleanupBullet();
    }

    // 1. Inicialização do Mundo Físico
    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();
    
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
    
    // 2. Chão
    btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);
    collisionShapes.push_back(groundShape);
    
    btDefaultMotionState* groundMotionState = new btDefaultMotionState(
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
    
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(
        0, groundMotionState, groundShape, btVector3(0, 0, 0));
    
    groundRigidBody = new btRigidBody(groundRigidBodyCI);
    groundRigidBody->setFriction(2.0f);
    dynamicsWorld->addRigidBody(groundRigidBody);

    // 3. Shapes Auxiliares
    projectileShape = new btSphereShape(0.3f); 
    collisionShapes.push_back(projectileShape);
    
    boxShape = new btBoxShape(btVector3(0.2f, 0.2f, 1.2f));
    collisionShapes.push_back(boxShape);
    
    targetBodies.clear(); 

    // ==========================================
    // === CONSTRUÇÃO: MINI FORTALEZA ===
    // (Configurada para seu Auto-Scale)
    // ==========================================
// ==========================================================
    // === 1. DEFINIÇÕES GLOBAIS (NÃO APAGUE) ===
    // ==========================================================
    // --- Criando Porcos ---
    // (Movido para depois da construção da fortaleza para usar as coordenadas corretas)


    // NÍVEL 3: Teto (Gelo - Placa)
    // NÍVEL 3: Teto (Gelo - Placa)
    // (Criando uma placa de 6x1x6)
    // BlocoDestrutivel* b4 = new BlocoDestrutivel(MaterialTipo::GELO, modeloBarra, 6.0f, 1.0f, 6.0f);
    // b4->inicializarFisica(dynamicsWorld, centro + btVector3(-L/2, Y_NIVEL3, 0), rotX);
    // blocos.push_back(b4);
    const char* pathPilar = "Objetos/bloco_barra.obj"; 
    const char* pathPlaca = "Objetos/bloco_placa.obj"; 
    
    // Escala e Unidade
    float escala = 0.5f; 
    float unit = escala; 

    // Dimensões dos Blocos
    float pilarW = 1.0f * unit; 
    float pilarH = 6.0f * unit; 
    float pilarD = 1.0f * unit; 

    float placaW = 6.0f * unit; 
    float placaH = 1.0f * unit; 
    float placaD = 6.0f * unit; 

    // --- POSIÇÃO ---
    // Z = -20.0f (Perto o suficiente para ver os detalhes)
    btVector3 centro(0.0f, 0.0f, -50.0f); 
    btQuaternion rot(0, 0, 0, 1);         

   float distanciaSeparacao = 35.0f * unit; 
    float avancoZ = 15.0f * unit; 
    float margin = 0.00f; 
    float distZ_Pilar = 2.0f * unit; 

    // Define para onde os canhões vão mirar (Ex: Posição inicial do estilingue)
    btVector3 alvoCanhao(0.0f, 5.0f, 30.0f); 

    auto estabilizarRigidBody = [&](btRigidBody* rb) {
        if(rb) {
            // 1. ACORDA O BLOCO: A gravidade vai puxá-lo para baixo imediatamente
            rb->activate(true); 
            
            // 2. FREIO DE MÃO (Damping Alto):
            // Isso impede que a torre desmorone violentamente ao nascer.
            // Eles vão cair devagar até se acomodarem.
            rb->setDamping(0.9f, 0.9f); 

            // 3. ATRITO E PESO
            rb->setFriction(2.0f);
            // rb->setMassProps(rb->getInvMass() == 0 ? 0 : 1.0f/rb->getInvMass(), btVector3(0,0,0)); // Opcional
        }
    };

    for (int idTorre = 0; idTorre < 3; idTorre++) {
        
        float centroXTorre;
        float centroZTorre;

        switch(idTorre){
            case 0: centroXTorre = centro.x() - (distanciaSeparacao/2.0f); centroZTorre = centro.z(); break;
            case 1: centroXTorre = centro.x() + (distanciaSeparacao/2.0f); centroZTorre = centro.z(); break;
            case 2: centroXTorre = centro.x(); centroZTorre = centro.z() + avancoZ; break;
        }
      
        float currentY = 0.0f; 
        int totalAndares = 6;      
        int raioFixo = 1; 

        for (int andar = 0; andar < totalAndares; andar++) {
            
            MaterialTipo mat;
            if (andar <= 1) mat = MaterialTipo::PEDRA;      
            else if (andar <= 3) mat = MaterialTipo::MADEIRA; 
            else mat = MaterialTipo::GELO;                  

            int raioAtual = raioFixo; 
            int fileirasProfundidade = 1; 

            float yPilar = currentY + (pilarH / 2.0f);
            float yPlaca = currentY + pilarH + (placaH / 2.0f) + margin;

            for (int rowZ = 0; rowZ < fileirasProfundidade; rowZ++) {
                
                float offsetZ = 3.0f; 
                if (fileirasProfundidade > 1) {
                     offsetZ = (rowZ == 0) ? -(placaD / 2.0f) : (placaD / 2.0f);
                }

                for (int i = -raioAtual; i <= raioAtual; i++) {
                    
                    float xPos = centroXTorre + (i * placaW); 
                    float zPos = centroZTorre + offsetZ;

                    // Cria Pilares
                    blocos.push_back(new BlocoDestrutivel(mat, pathPilar, pilarW, pilarH, pilarD));
                    blocos.back()->inicializarFisica(dynamicsWorld, btVector3(xPos, yPilar, zPos + distZ_Pilar), rot);
                    estabilizarRigidBody(blocos.back()->getRigidBody());

                    blocos.push_back(new BlocoDestrutivel(mat, pathPilar, pilarW, pilarH, pilarD));
                    blocos.back()->inicializarFisica(dynamicsWorld, btVector3(xPos, yPilar, zPos - distZ_Pilar), rot);
                    estabilizarRigidBody(blocos.back()->getRigidBody());

                    // Cria Placa
                    blocos.push_back(new BlocoDestrutivel(mat, pathPlaca, placaW, placaH, placaD));
                    blocos.back()->inicializarFisica(dynamicsWorld, btVector3(xPos, yPlaca, zPos), rot);
                    estabilizarRigidBody(blocos.back()->getRigidBody());

                    // --- INIMIGOS ---
                    if (i == 0) { 
                        float yFinal = yPlaca + (placaH / 2.0f) + margin + 1.0f;

                        // PORCOS
                        if (idTorre != 2 && (andar == 1 || andar == 3)) {
                            Porco* p = new Porco(xPos + -1.1f, yFinal, zPos); 
                            p->inicializarFisica(dynamicsWorld, xPos - 1.1f, yFinal, zPos); 
                            estabilizarRigidBody(p->getRigidBody());
                            porcos.push_back(p);
                        }
                        
                        // CANHÕES (Torre central)
                        if (idTorre == 2 && andar == 2) {
                            Cannon* c = new Cannon(xPos + 1.0f, yFinal, zPos, dynamicsWorld, alvoCanhao);
                            estabilizarRigidBody(c->getRigidBody());
                            canhoes.push_back(c); 
                        }
                    }
                }
            }
            currentY += pilarH + placaH + margin;
        }

        // --- TOPO DA TORRE ---
        float yTopoPilar = currentY + (pilarH / 2.0f);
        float yTopoPlaca = currentY + pilarH + (placaH / 2.0f) + margin;
        
        blocos.push_back(new BlocoDestrutivel(MaterialTipo::MADEIRA, pathPilar, pilarW, pilarH, pilarD));
        blocos.back()->inicializarFisica(dynamicsWorld, btVector3(centroXTorre, yTopoPilar, centroZTorre), rot);
        estabilizarRigidBody(blocos.back()->getRigidBody());

        blocos.push_back(new BlocoDestrutivel(MaterialTipo::MADEIRA, pathPlaca, placaW, placaH, placaD));
        blocos.back()->inicializarFisica(dynamicsWorld, btVector3(centroXTorre, yTopoPlaca, centroZTorre), rot);
        estabilizarRigidBody(blocos.back()->getRigidBody());

        // Inimigo no Topo
        float yTopoFinal = yTopoPlaca + (placaH / 2.0f) + margin + 1.5f;

        if (idTorre == 2) { 
             // Rei Porco no meio
             Porco* p = new Porco(centroXTorre, yTopoFinal, centroZTorre);
             p->inicializarFisica(dynamicsWorld, centroXTorre, yTopoFinal, centroZTorre);
             estabilizarRigidBody(p->getRigidBody());
             porcos.push_back(p);
        }
    }
    // --- ÁRVORES ---
    trees.clear();
    // Par 1: Extremos distantes
    trees.push_back(Tree( 55.0f, 0.0f, -55.0f, 19.0f));
    trees.push_back(Tree(-55.0f, 0.0f, -55.0f, 19.0f));

    // Par 2: Preenchendo o fundo
    trees.push_back(Tree( 35.0f, 0.0f, -60.0f, 17.5f));
    trees.push_back(Tree(-35.0f, 0.0f, -60.0f, 17.5f));

    // Par 3: Mais próximas do centro (mas atrás das torres)
    trees.push_back(Tree( 12.0f, 0.0f, -65.0f, 18.0f));
    trees.push_back(Tree(-12.0f, 0.0f, -65.0f, 18.0f));


    // === CAMADA 2: CAMPO MÉDIO (Ao redor das Torres) ===
    // Espalhadas entre Z -40 e -48
    
    // Par 4: Variação de tamanho
    trees.push_back(Tree( 42.0f, 0.0f, -48.0f, 16.0f));
    trees.push_back(Tree(-42.0f, 0.0f, -48.0f, 16.0f));

    // Par 5: Quebrando a linearidade
    trees.push_back(Tree( 22.0f, 0.0f, -42.0f, 15.0f));
    trees.push_back(Tree(-22.0f, 0.0f, -42.0f, 15.0f));

    // Par 6: Atrás das torres laterais
    trees.push_back(Tree( 28.0f, 0.0f, -52.0f, 16.5f));
    trees.push_back(Tree(-28.0f, 0.0f, -52.0f, 16.5f));


    // === CAMADA 3: PRIMEIRO PLANO (Moldura) ===
    // Mais perto da câmera (Z -25 a -35), mas bem abertas no X
    // para não tapar as torres nem o pássaro.

    // Par 7: Laterais próximas
    trees.push_back(Tree( 45.0f, 0.0f, -30.0f, 14.5f));
    trees.push_back(Tree(-45.0f, 0.0f, -30.0f, 14.5f));

    // Par 8: O "limite" da tela
    trees.push_back(Tree( 32.0f, 0.0f, -22.0f, 13.0f));
    trees.push_back(Tree(-32.0f, 0.0f, -22.0f, 13.0f));
}

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
    float tamanho = 60.0f;
    glVertex3f(-tamanho, 0.0f, -tamanho);
    glVertex3f(-tamanho, 0.0f, tamanho);
    glVertex3f(tamanho, 0.0f, tamanho);
    glVertex3f(tamanho, 0.0f, -tamanho);
    glEnd();
    
    glColor3f(0.25f, 0.5f, 0.25f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (float x = -tamanho; x <= tamanho; x += 2.0f) {
        glVertex3f(x, 0.01f, -tamanho);
        glVertex3f(x, 0.01f, tamanho);
    }
    for (float z = -tamanho; z <= tamanho; z += 2.0f) {
        glVertex3f(-tamanho, 0.01f, z);
        glVertex3f(tamanho, 0.01f, z);
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

    if (fps >= 55.0f) glColor3f(0.0f, 1.0f, 0.0f); // Verde
    else if (fps >= 30.0f) glColor3f(1.0f, 1.0f, 0.0f); // Amarelo
    else glColor3f(1.0f, 0.0f, 0.0f); // Vermelho

    glRasterPos2f(WIDTH - 120, HEIGHT - 30); // Canto superior direito
    char fpsText[30];
    sprintf(fpsText, "FPS: %.1f", fps);
    for (char* c = fpsText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c); // Fonte negrito
    }

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
    if (passaroAtual && passaroAtual->isEmVoo() && button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN ) {
        passaroAtual->usarHabilidade();
    }
    if (passaroAtual && passaroAtual->isEmVoo() && button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN ) {
        passaroAtual->usarHabilidade();
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
            
            // Limpa a física do pássaro atual (pois pertence ao mundo antigo)
            if (passaroAtual) {
                passaroAtual->limparFisica();
                passaroAtual->resetar(0,0,0);
            }

            // 1. Deleta o SlingshotManager ANTES de limpar o mundo
            // Isso permite que o destrutor remova o corpo do mundo e delete a memória corretamente
            if (g_slingshotManager) {
                delete g_slingshotManager;
                g_slingshotManager = nullptr;
            }

            // 2. Recria o mundo (cleanupBullet é chamado dentro)
            initBullet(); 
            
            // 3. Cria o novo SlingshotManager com o NOVO mundo
            g_slingshotManager = new SlingshotManager(dynamicsWorld, passaroAtual, &shotsRemaining, &gameOver);
            
            // Reseta o pássaro atual para garantir que ele crie física no novo mundo se necessário
            // (Na verdade, o SlingshotManager vai cuidar disso quando setProjectile for chamado ou no update)
            if(passaroAtual) {
                // passaroAtual->resetar(0,0,0); // Já chamado acima
                if (g_slingshotManager) {
                    g_slingshotManager->setProjectile(passaroAtual);
                }
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
    if (!jogoCarregado) {
        DrawLoadingScreen(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        return; // <--- IMPEDE A EXECUÇÃO DO RESTO DA FUNÇÃO
    }
    frameCount++;
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    int timeInterval = currentTime - previousTime;

    if (timeInterval > 1000) { // Atualiza a cada 1 segundo (1000ms)
        fps = frameCount / (timeInterval / 1000.0f);
        previousTime = currentTime;
        frameCount = 0;
    }
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
    
    //aplicando a iluminação          
    g_lightingManager.apply();
    
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
    if (passaroAtual) {
        glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
        
        if (passaroAtual->getRigidBody()) {
            // Se tem corpo físico (voando ou sendo arrastado), usa a física
            passaroAtual->desenhar(); 
        } else if (g_slingshotManager) {
            // Se não tem corpo físico (esperando no estilingue), desenha na posição da malha
            float px, py, pz;
            g_slingshotManager->getPouchPosition(px, py, pz);
            passaroAtual->desenharEmPosicao(px, py, pz);
        }
    }

    // Desenha pássaros extras (Blue clones)
    for (auto* bird : extraBirds) {
        if (bird->isAtivo()) {
            bird->desenhar();
        }
    }
    
    //desenha os blocos
    //corrigi o bug da hitbox invisivel 
    for (auto& bloco : blocos) {
        glPushMatrix();
        // glTranslatef(0.0f, -2.5f, 0.0f);
        bloco->desenhar();
        glPopMatrix();
    }

    // Desenha os porcos
    for (auto& porco : porcos) {
        porco->desenhar();
    }

    // Desenha os canhões
    // --- EM VEZ DE: canhao->desenhar(); ---
    // FAÇA ISTO:
    for (auto& c : canhoes) {
        c->desenhar();
    }
    

    // Desenha todos os outros corpos rígidos (as caixas)
    // for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
    //     btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
    //     btRigidBody* body = btRigidBody::upcast(obj);
        
    //     if (body && body->getMotionState() && body->getInvMass() != 0) {
            
    //         // Pula o 'passaroAtual', pois ele já foi desenhado acima
    //         if (passaroAtual && body == passaroAtual->getRigidBody()) {
    //             continue;
    //         }
            
    //         btTransform trans;
    //         body->getMotionState()->getWorldTransform(trans);
    //         btScalar m[16];
    //         trans.getOpenGLMatrix(m);
            
    //         glPushMatrix();
    //         glMultMatrixf(m);
            
    //         btCollisionShape* shape = body->getCollisionShape();
            
    //         // CORREÇÃO: Mudado de 'else if' para 'if'
    //         // Desenha apenas as caixas-alvo
    //         if (shape == boxShape) {
    //             glMaterialf(GL_FRONT, GL_SHININESS, 5.0f);
                
    //             // Verifica se o alvo está na mira
    //             if (g_slingshotManager && g_slingshotManager->isTargetInAimLine(body)) {
    //                 glColor3f(0.2f, 1.0f, 0.2f); // Verde
    //             } else {
    //                 glColor3f(0.6f, 0.4f, 0.2f); // Madeira
    //             }
                
    //             if (blockModelLoaded) {
    //                 // Se o modelo carregou, desenha o .obj
    //                 glScalef(5.0, 5.0, 5.0);
    //                 blockModel.draw();
    //             } else {
    //                 // Senão, desenha o cubo antigo como fallback
    //                 glutSolidCube(1.0);
    //             }
                
    //             // (Contorno da caixa)
    //             // glDisable(GL_LIGHTING);
    //             // glColor3f(0.3f, 0.2f, 0.1f);
    //             // glLineWidth(2.0f);
    //             // glBegin(GL_LINES);
    //             // glVertex3f(-0.5f, 0.0f, -0.5f);
    //             // glVertex3f(0.5f, 0.0f, -0.5f);
    //             // glVertex3f(-0.5f, 0.0f, 0.5f);
    //             // glVertex3f(0.5f, 0.0f, 0.5f);
    //             // glEnd();
    //             // glEnable(GL_LIGHTING);
    //         }
            
    //         glPopMatrix();
    //     }
    // }
    
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

    float deltaTime = 1.0f / 60.0f;
    // --- CORREÇÃO 1: TEMPO DE SEGURANÇA ---
    // Variável estática para contar quanto tempo o jogo está rodando.
    // Usada para impedir que a torre quebre sozinha enquanto se acomoda (settling).
    static float tempoDecorrido = 0.0f;
    tempoDecorrido += deltaTime;

    // DEBUG: Verifica se o estilingue está no mundo físico (A cada 2 segundos)
        // static int frameCountDebug = 0;
        // frameCountDebug++;
        // if (frameCountDebug % 120 == 0 && g_slingshotManager && dynamicsWorld) {
        //     btRigidBody* sb = g_slingshotManager->getRigidBody();
        //     bool found = false;
        //     for(int i=0; i<dynamicsWorld->getNumCollisionObjects(); i++) {
        //         if(dynamicsWorld->getCollisionObjectArray()[i] == sb) found = true;
        //     }
        //     if(!found) {
        //         printf("ALERTA CRITICO: SlingshotBody (%p) NAO esta no dynamicsWorld (%p)!\n", sb, dynamicsWorld);
        //         printf("Total objetos no mundo: %d\n", dynamicsWorld->getNumCollisionObjects());
        //     } else {
        //          // printf("INFO: SlingshotBody presente.\n");
        //     }
        // }

    // Simula a física
    dynamicsWorld->stepSimulation(1.0f / 60.0f, 10, 1.0f / 180.0f);
    
    // Atualiza lógica dos blocos
    for (auto& bloco : blocos) {
        bloco->update(deltaTime);
        bloco->clearContactFlag();

        // --- CORREÇÃO: LIBERAR FÍSICA APÓS ESTABILIZAR ---
        // Se já passaram 3 segundos (tempo suficiente para a torre se acomodar no chão)
        if (tempoDecorrido > 3.0f) {
            btRigidBody* rb = bloco->getRigidBody();
            if (rb) {
                // Se o damping ainda está alto (0.9), voltamos para o normal (0.1 ou 0.0)
                if (rb->getLinearDamping() > 0.5f) {
                    rb->setDamping(0.1f, 0.1f);
                }
                
                // Opcional: Agora sim podemos deixar eles dormirem se estiverem parados
                if (rb->getLinearVelocity().length2() < 0.01f) {
                     // rb->setActivationState(ISLAND_SLEEPING); // Descomente se quiser otimizar CPU
                }
            }
        }
    }

    // Atualiza lógica do pássaro
    if (passaroAtual) {
        passaroAtual->atualizar(deltaTime);
        
    if (!passaroAtual->isAtivo()) {
            proximoPassaro();
        }
    }

    // Atualiza pássaros extras (Blue clones)
    for (auto it = extraBirds.begin(); it != extraBirds.end(); ) {
        (*it)->atualizar(deltaTime);
        if (!(*it)->isAtivo()) {
            delete *it;
            it = extraBirds.erase(it);
        } else {
            ++it;
        }
    }

    // 1. Corrige os PORCOS
        for (auto& porco : porcos) {
            if (porco->getRigidBody()) {
                // Tira o "peso" artificial e deixa cair normal
                if (porco->getRigidBody()->getLinearDamping() > 0.1f) {
                    porco->getRigidBody()->setDamping(0.0f, 0.0f); 
                }
                // Garante que a gravidade esteja ligada
                porco->getRigidBody()->activate(true); 
            }
        }

        // 2. Corrige os CANHÕES
        for (auto& c : canhoes) {
            if (c->getRigidBody()) {
                if (c->getRigidBody()->getLinearDamping() > 0.1f) {
                    c->getRigidBody()->setDamping(0.0f, 0.0f);
                }
                c->getRigidBody()->activate(true);
            }
        }


    g_particleManager.update(deltaTime);

    // ==========================================================
    // === LÓGICA DE COLISÃO E DANO ===
    // ==========================================================
    int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold* contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
        const btCollisionObject* obA = contactManifold->getBody0();
        const btCollisionObject* obB = contactManifold->getBody1();

        // Variáveis para identificar quem colidiu
        BlocoDestrutivel* blocoA = nullptr;
        BlocoDestrutivel* blocoB = nullptr;
        Porco* porco = nullptr;
        
        // Procura se obA é um bloco
        for (auto& b : blocos) {
            if (b->getRigidBody() == obA) { blocoA = b; break; }
        }
        // Procura se obB é um bloco
        for (auto& b : blocos) {
            if (b->getRigidBody() == obB) { blocoB = b; break; }
        }

        // Procura porcos
        for (auto& p : porcos) {
            if (p->getRigidBody() == obA || p->getRigidBody() == obB) {
                porco = p; break;
            }
        }

        // --- NOVA LÓGICA: Colisão Porco x Estilingue ---
       if (g_slingshotManager) {
            btRigidBody* slingshotBody = g_slingshotManager->getRigidBody();
            
            if (slingshotBody && (obA == slingshotBody || obB == slingshotBody)) {
                // Identifica o outro objeto
                const btCollisionObject* otherOb = (obA == slingshotBody) ? obB : obA;
                
                // VERIFICA SE O OBJETO É UM PORCO (PROJÉTIL)
                // Percorre a lista de porcos para ver se algum deles é o que bateu
                for (auto& p : porcos) {
                    if (p->getRigidBody() == otherOb) {
                        float velocity = p->getRigidBody()->getLinearVelocity().length();
                        
                        // Se houve impacto real
                        if (velocity > 0.5f) { 
                            // printf("DEBUG: Porco atingiu o estilingue! Vel: %.2f\n", velocity);
                            g_slingshotManager->takeDamage();
                            p->tomarDano(500.0f); // Destroi o projétil
                            // contactManifold->clearManifold(); // Opcional
                        }
                        break; // Já achou o porco, sai do loop
                    }
                }
            }
        }
                    
        // Verifica se o pássaro está na colisão
        bool passaroEnvolvido = passaroAtual && (obA == passaroAtual->getRigidBody() || obB == passaroAtual->getRigidBody());

        // Calcula a força total da batida (Impulso)
        float impulsoTotal = 0;
        for (int j = 0; j < contactManifold->getNumContacts(); j++) {
            impulsoTotal += contactManifold->getContactPoint(j).getAppliedImpulse();
        }

        // --------------------------------------------------------
        // CASO 1: BLOCO x BLOCO (Torre caindo sobre si mesma)
        // --------------------------------------------------------
        if (blocoA && blocoB) {
            // Só processa após 2 segundos para evitar quebra no spawn
            if (tempoDecorrido > 4.0f) {
                // Impulso mínimo alto (3.0) para ignorar o peso de blocos parados
                if (impulsoTotal > 8.0f) {
                    // Dano reduzido (0.15) pois colisões entre blocos são frequentes
                    float dano = impulsoTotal * 0.15f; 
                    blocoA->aplicarDano(dano);
                    blocoB->aplicarDano(dano);
                }
            }
        }

        // --------------------------------------------------------
        // CASO 2: BLOCO x PÁSSARO (Impacto direto)
        // --------------------------------------------------------
        else if ((blocoA || blocoB) && passaroEnvolvido) {
            BlocoDestrutivel* alvo = (blocoA) ? blocoA : blocoB;
            
            // Dano quadrático (Energia cinética simulada)
            float dano = (impulsoTotal * impulsoTotal) * 0.005f; 
            
            if (dano > 1.2f) { 
                alvo->aplicarDano(dano);
            }
        }

        // --------------------------------------------------------
        // CASO 3: BLOCO x CHÃO (Queda)
        // --------------------------------------------------------
        else if ((blocoA || blocoB) && (obA == groundRigidBody || obB == groundRigidBody)) {
            // Só processa após 2 segundos
            if (tempoDecorrido > 2.0f) {
                BlocoDestrutivel* alvo = (blocoA) ? blocoA : blocoB;
                
                // Verifica se houve impacto real (não apenas estar encostado)
                if (impulsoTotal > 1.0f) {
                    btVector3 vel = alvo->getRigidBody()->getLinearVelocity();
                    
                    // CORREÇÃO CRUCIAL: Usa apenas a velocidade vertical (Y) invertida.
                    // Se o bloco deslizar de lado (X/Z), ele não quebra. Só quebra se cair (Y).
                    float velocidadeQueda = -vel.getY(); 

                    if (velocidadeQueda > 5.0f) { 
                        float dano = velocidadeQueda * 3.0f; // Multiplicador para converter velocidade em dano
                        alvo->aplicarDano(dano);
                    }
                }
            }
        }

        // --------------------------------------------------------
        // CASO 4: PORCOS (Dano genérico por impacto)
        // --------------------------------------------------------
        if (porco) {
            // Proteção de tempo opcional para os porcos também
            if (tempoDecorrido > 1.0f && impulsoTotal > 1.5f) {
                porco->tomarDano(impulsoTotal * 0.2f);
            }
        }
    }

    // ==========================================================
    // === LIMPEZA E PONTUAÇÃO ===
    // ==========================================================

    // 1. Remove blocos destruídos da memória e do mundo
    for (int i = blocos.size() - 1; i >= 0; i--) {
        if (blocos[i]->isDestruido()) {
            blocos[i]->limparFisica(dynamicsWorld);
            delete blocos[i];
            blocos.erase(blocos.begin() + i);
            // score += 50; // Opcional: Pontos por destruir blocos
        }
    }

    // 2. Atualiza o estilingue
    if (g_slingshotManager) {
        g_slingshotManager->update();
    }
    
    // 3. Remove objetos que caíram do mundo (Abismo) e dá pontos
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        
        if (body && body->getCollisionShape() == boxShape) { // Assume que boxShape são os blocos/porcos
            btTransform trans;
            body->getMotionState()->getWorldTransform(trans);
            float y = trans.getOrigin().getY();
            
            // Se caiu muito abaixo do chão
            if (y < -2.0f) {
                score += 100;
                
                // Remove da lista de alvos do pássaro (se houver essa lógica)
                auto it = std::find(targetBodies.begin(), targetBodies.end(), body);
                if (it != targetBodies.end()) {
                    targetBodies.erase(it);
                }
                
                // Remove do mundo físico
                dynamicsWorld->removeRigidBody(body);
                delete body->getMotionState();
                delete body;
                
                // Nota: O ponteiro na lista 'blocos' ficará inválido aqui se não for tratado.
                // O ideal é que a limpeza acima (isDestruido) trate disso, ou marcar o bloco como destruído aqui.
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

    glEnable(GL_CULL_FACE);  // Não desenha a parte de trás dos objetos
    glCullFace(GL_BACK);     // Define que é a parte de trás a ser ignorada


    glShadeModel(GL_FLAT);
    glEnable(GL_LINE_SMOOTH);
    glDisable(GL_CULL_FACE);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_NORMALIZE);
    glClearColor(0.7f, 0.85f, 0.95f, 1.0f);
    
    g_lightingManager.init();

    g_skyTextureID = loadGlobalTexture("Objetos/texturas/fundo_ceu_borrado.png"); 
    if (g_skyTextureID == 0) {
        printf("ERRO: Falha ao carregar a textura do ceu.\n");
    }
    // 1. Inicializa a física
    initBullet();
    printf("DEBUG: initBullet concluido. dynamicsWorld = %p\n", dynamicsWorld);

    g_menu = new GameMenu(WIDTH, HEIGHT);
    
    //carrega as texturas dos blocos
            for (auto& bloco : blocos) {
                bloco->carregarTexturas();
            }

    // 2. Cria a instância do gerenciador do estilingue
    //    (O ponteiro 'red' já foi criado na 'main')
    g_slingshotManager = new SlingshotManager(dynamicsWorld, passaroAtual, &shotsRemaining, &gameOver);
    
    // 3. REMOVIDO: Bloco de inicialização da 'struct slingshot'
    
    // 4. Carregar modelo da árvore
    printf("\nTentando carregar modelo OBJ da arvore...\n");
    const char* possiblePaths[] = {
        "Objetos/arvore2.obj",
        "./Objetos/arvore2.obj",
        "../Objetos/arvore2.obj",
        "arvore2.obj",
        "tree.obj"
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

void carregarJogo(int value) {
    if (!jogoCarregado) {
        printf("=== INICIANDO CARREGAMENTO DOS RECURSOS ===\n");

        // -------------------------------------------------------
        // AQUI ESTAVA O PESO (Criação dos Pássaros = Carregar OBJ/MTL)
        // -------------------------------------------------------
        for (int i = 0; i < 8; i+=3) {
            filaPassaros.push_back(new PassaroRed(0.0f, 0.0f, 0.0f));
            filaPassaros.push_back(new PassaroChuck(0.0f, 0.0f, 0.0f));
            filaPassaros.push_back(new PassaroBomb(0.0f, 0.0f, 0.0f));
            filaPassaros.push_back(new PassaroBlue(0.0f, 0.0f, 0.0f));
        }

        // Inicializa o iterador e o primeiro pássaro
        itPassaroAtual = filaPassaros.begin();
        if (itPassaroAtual != filaPassaros.end()) {
            passaroAtual = *itPassaroAtual;
        }
        
        // Inicializa Física, Texturas do jogo, etc.
        init();

        // Inicializa Áudio
        if (!g_audioManager.initAudio()) {
            printf("AVISO: Audio desabilitado devido a falha na inicializacao.\n");
        }
        g_audioManager.setVolume(10.0f);

        // Configura os callbacks de INTERAÇÃO (Mouse/Teclado)
        // Só ativamos isso agora, para o jogador não clicar durante o loading
        glutMouseFunc(mouse);
        glutMotionFunc(mouseMotion);
        glutPassiveMotionFunc(passiveMouseMotion);
        glutKeyboardFunc(keyboard);
        glutSpecialFunc(specialKeys);
        
        // Inicia o loop de física
        glutTimerFunc(0, timer, 0);

        // --- FINALIZAÇÃO ---
        jogoCarregado = true; // Libera o display para desenhar o jogo 3D
        glutPostRedisplay();  // Força atualização da tela
        printf("=== CARREGAMENTO CONCLUIDO ===\n");
    }
}


int main(int argc, char** argv) {
    // 1. Inicialização Básica do GLUT (Janela)
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Estilingue 3D - Angry C++ Birds"); // Mudei o título :)
    
    // 2. Inicializa APENAS a Tela de Carregamento (Leve)
    InitLoadingScreen(); 

    // 3. Define callbacks visuais básicos
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    
    // 4. O TRUQUE: Chama o carregamento pesado daqui a 100ms
    // Isso dá tempo da janela abrir e desenhar a imagem de fundo antes de travar.
    glutTimerFunc(100, carregarJogo, 0);
    
    // 5. Entra no loop
    glutMainLoop();
    
    // --- LIMPEZA DE MEMÓRIA (Executado ao fechar) ---
    // (Este código permanece igual ao seu original, para limpar ao sair)
    if (passaroAtual) {
        delete passaroAtual;
        passaroAtual = nullptr;
    }
    for (auto* passaro : filaPassaros) delete passaro;
    filaPassaros.clear();
    
    // Limpeza Bullet Physics
    if (dynamicsWorld) {
        for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
            btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState()) delete body->getMotionState();
            dynamicsWorld->removeCollisionObject(obj);
            delete obj;
        }
    }
    for (int i = 0; i < collisionShapes.size(); i++) delete collisionShapes[i];
    
    g_audioManager.cleanup();
    if(g_slingshotManager) delete g_slingshotManager;
    
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;

    return 0;
}