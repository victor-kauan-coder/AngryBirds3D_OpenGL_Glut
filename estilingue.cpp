#define SDL_MAIN_HANDLED 
#include <SDL2/SDL.h>
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

bool animandoEntradaPassaro = false;
float animEntradaTimer = 0.0f;
btVector3 animStartPos;
btVector3 animEndPos;

//MENU
GameMenu* g_menu = nullptr; // Ponteiro para o menu
GameState g_currentState = STATE_MENU;
static float ultimoSomImpacto = 0.0f;

GLuint g_skyTextureID = 0;
GLuint g_heartTextureID = 0;
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
bool gameWon = false;  // Vitória
bool gameLost = false; // Derrota

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
    itPassaroAtual++;
    
    if (itPassaroAtual != filaPassaros.end()) {
        passaroAtual = *itPassaroAtual;
        
        // --- 1. CONFIGURA O PULO DO PÁSSARO ATUAL ---
        if (g_slingshotManager) {
            animandoEntradaPassaro = true;
            animEntradaTimer = 0.0f;
            
            animStartPos = passaroAtual->getPosFila();
            float px, py, pz;
            g_slingshotManager->getPouchPosition(px, py, pz);
            animEndPos = btVector3(px, py, pz);
            
            // modificar para cada passaro
            // g_audioManager.playPassaro(SomTipo::LANCAMENTO_PASSARO, 50);
        }
        
        // --- 2. FAZ A FILA ANDAR (CORRIGIDO) ---
        float gap = 1.8f; 
        auto itFila = itPassaroAtual; 
        itFila++; 
        
        for (; itFila != filaPassaros.end(); ++itFila) {
            Passaro* p = *itFila;
            if (p->isNaFila()) {
                btVector3 posAntiga = p->getPosFila();
                // Calcula o destino (um "gap" para a direita)
                btVector3 novoDestino(posAntiga.x() + gap, posAntiga.y(), posAntiga.z());
                
                // Inicia a animação de caminhada/pulo da fila
                p->caminharNaFilaPara(novoDestino);
            }
        }
        
        printf("Proximo passaro pulando e fila andando...\n");
    } else {
        passaroAtual = nullptr;
        if (g_slingshotManager) {
            g_slingshotManager->setProjectile(nullptr);
        }
        printf("Fim dos passaros!\n");
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

    float yTopoPilar = currentY + (pilarH / 2.0f);
        float yTopoPlaca = currentY + pilarH + (placaH / 2.0f) + margin;

        // CORREÇÃO CRÍTICA:
        // No loop anterior, você usou 'float offsetZ = 3.0f'.
        // O topo precisa acompanhar esse mesmo deslocamento para ficar em cima!
        float zCentroReal = centroZTorre + 3.0f; 

        // Distância dos pilares para os cantos (para ficarem na borda da placa)
        float offsetCanto = 2.2f * unit; 

        // --- CRIANDO OS 4 PILARES DE SUPORTE (Mesa) ---
        
        // 1. Frente-Esquerda
        blocos.push_back(new BlocoDestrutivel(MaterialTipo::MADEIRA, pathPilar, pilarW, pilarH, pilarD));
        blocos.back()->inicializarFisica(dynamicsWorld, btVector3(centroXTorre - offsetCanto, yTopoPilar, zCentroReal + offsetCanto), rot);
        estabilizarRigidBody(blocos.back()->getRigidBody());

        // 2. Frente-Direita
        blocos.push_back(new BlocoDestrutivel(MaterialTipo::MADEIRA, pathPilar, pilarW, pilarH, pilarD));
        blocos.back()->inicializarFisica(dynamicsWorld, btVector3(centroXTorre + offsetCanto, yTopoPilar, zCentroReal + offsetCanto), rot);
        estabilizarRigidBody(blocos.back()->getRigidBody());

        // 3. Trás-Esquerda
        blocos.push_back(new BlocoDestrutivel(MaterialTipo::MADEIRA, pathPilar, pilarW, pilarH, pilarD));
        blocos.back()->inicializarFisica(dynamicsWorld, btVector3(centroXTorre - offsetCanto, yTopoPilar, zCentroReal - offsetCanto), rot);
        estabilizarRigidBody(blocos.back()->getRigidBody());

        // 4. Trás-Direita
        blocos.push_back(new BlocoDestrutivel(MaterialTipo::MADEIRA, pathPilar, pilarW, pilarH, pilarD));
        blocos.back()->inicializarFisica(dynamicsWorld, btVector3(centroXTorre + offsetCanto, yTopoPilar, zCentroReal - offsetCanto), rot);
        estabilizarRigidBody(blocos.back()->getRigidBody());

        // --- TETO ---
        blocos.push_back(new BlocoDestrutivel(MaterialTipo::MADEIRA, pathPlaca, placaW, placaH, placaD));
        // Note o uso de 'zCentroReal' aqui também
        blocos.back()->inicializarFisica(dynamicsWorld, btVector3(centroXTorre, yTopoPlaca, zCentroReal), rot);
        estabilizarRigidBody(blocos.back()->getRigidBody());

        // --- INIMIGO NO TOPO ---
        float yTopoFinal = yTopoPlaca + (placaH / 2.0f) + margin + 1.5f;

        if (idTorre == 2) { 
             // Rei Porco no meio (Usando zCentroReal)
             Porco* p = new Porco(centroXTorre, yTopoFinal, zCentroReal);
             p->inicializarFisica(dynamicsWorld, centroXTorre, yTopoFinal, zCentroReal);
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

void drawTextCentered(const char* text, float x, float y, void* font, float r, float g, float b) {
    glColor3f(r, g, b);
    int len = 0;
    while(text[len]) len++; // strlen manual
    
    int width = 0;
    for (int i = 0; i < len; i++) {
        width += glutBitmapWidth(font, text[i]);
    }
    
    glRasterPos2f(x - (width / 2.0f), y);
    for (int i = 0; i < len; i++) {
        glutBitmapCharacter(font, text[i]);
    }
}

void drawHUD() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    // Configura projeção 2D
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // --- HUD DURANTE O JOGO ---
    if (!gameOver && !gameWon && !gameLost) {
        
        // --- ATIVA A TRANSPARÊNCIA (CORREÇÃO AQUI) ---
        // Isso garante que tanto a barra quanto os corações fiquem transparentes
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // 1. PRIMEIRO: Fundo do HUD (Barra Preta Translúcida)
        glColor4f(0.0f, 0.0f, 0.0f, 0.8f); // 50% transparente
        glBegin(GL_QUADS);
            glVertex2f(0, HEIGHT);
            glVertex2f(0, HEIGHT - 60);
            glVertex2f(WIDTH, HEIGHT - 60);
            glVertex2f(WIDTH, HEIGHT);
        glEnd();

        // 2. SEGUNDO: Corações de Vida (Desenhados SOBRE a barra)
        if (g_heartTextureID != 0 && g_slingshotManager) {
            int vidaAtual = g_slingshotManager->getHealth();
            
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, g_heartTextureID);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Branco puro para a textura

            float heartSize = 40.0f;
            float marginX = 20.0f;
            float marginY = 10.0f; 
            float padding = 5.0f;
            float startY = HEIGHT - marginY; 

            glBegin(GL_QUADS);
            for (int i = 0; i < vidaAtual; i++) {
                float x = marginX + (i * (heartSize + padding));
                
                glTexCoord2f(0, 1); glVertex2f(x, startY);
                glTexCoord2f(1, 1); glVertex2f(x + heartSize, startY);
                glTexCoord2f(1, 0); glVertex2f(x + heartSize, startY - heartSize);
                glTexCoord2f(0, 0); glVertex2f(x, startY - heartSize);
            }
            glEnd();
            glDisable(GL_TEXTURE_2D);
        } 
        // Fallback: Quadrados vermelhos se a textura falhar
        else if (g_slingshotManager) {
             int vidaAtual = g_slingshotManager->getHealth();
             glColor3f(1.0f, 0.0f, 0.0f); // Vermelho
             float heartSize = 40.0f;
             float startY = HEIGHT - 10.0f;
             glBegin(GL_QUADS);
             for (int i = 0; i < vidaAtual; i++) {
                float x = 20.0f + (i * 45.0f);
                glVertex2f(x, startY);
                glVertex2f(x + heartSize, startY);
                glVertex2f(x + heartSize, startY - heartSize);
                glVertex2f(x, startY - heartSize);
            }
            glEnd();
        }

        // --- DESLIGA O BLEND APÓS DESENHAR IMAGENS E BARRAS ---
        glDisable(GL_BLEND);

        // 3. TERCEIRO: Textos (O GLUT desenha bitmaps melhor sem Blend em alguns casos, ou com ele ligado.
        // Se o texto ficar feio, reative o Blend aqui, mas geralmente para BitmapCharacter não precisa)
        char hudText[100];
        
        float fpsR = (fps < 30) ? 1.0f : ((fps < 55) ? 1.0f : 0.0f);
        float fpsG = (fps >= 30) ? 1.0f : 0.0f;
        
        sprintf(hudText, "FPS: %.1f", fps);
        drawTextCentered(hudText, WIDTH - 80, HEIGHT - 35, GLUT_BITMAP_HELVETICA_18, fpsR, fpsG, 0.0f);

        sprintf(hudText, "Pontos: %d", score);
        drawTextCentered(hudText, WIDTH / 2, HEIGHT - 35, GLUT_BITMAP_HELVETICA_18, 1.0f, 1.0f, 1.0f);

        sprintf(hudText, "Tiros: %d", shotsRemaining);
        drawTextCentered(hudText, WIDTH / 2 + 150, HEIGHT - 35, GLUT_BITMAP_HELVETICA_18, 1.0f, 1.0f, 1.0f);
        
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(10, 30);
        const char* help = "Q/E: Profundidade  |  Mouse: Mirar e Atirar";
        for (const char* c = help; *c != '\0'; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // --- TELA DE FIM DE JOGO ---
    if (gameOver || gameWon || gameLost) {
         glEnable(GL_BLEND); // Garante transparência para o fundo escuro
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
         glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f(WIDTH, 0); glVertex2f(WIDTH, HEIGHT); glVertex2f(0, HEIGHT); glEnd();
         
         float panelW = 400; float panelH = 300; float centerX = WIDTH / 2.0f; float centerY = HEIGHT / 2.0f;
         glColor4f(0.2f, 0.2f, 0.2f, 0.9f);
         glBegin(GL_QUADS); glVertex2f(centerX - panelW/2, centerY - panelH/2); glVertex2f(centerX + panelW/2, centerY - panelH/2); glVertex2f(centerX + panelW/2, centerY + panelH/2); glVertex2f(centerX - panelW/2, centerY + panelH/2); glEnd();
         
         glLineWidth(3.0f); if (gameWon) glColor3f(0.2f, 0.8f, 0.2f); else glColor3f(0.8f, 0.2f, 0.2f);
         glBegin(GL_LINE_LOOP); glVertex2f(centerX - panelW/2, centerY - panelH/2); glVertex2f(centerX + panelW/2, centerY - panelH/2); glVertex2f(centerX + panelW/2, centerY + panelH/2); glVertex2f(centerX - panelW/2, centerY + panelH/2); glEnd();
         
         char title[50]; char scoreMsg[50]; if (gameWon) sprintf(title, "VITORIA!"); else sprintf(title, "FIM DE JOGO"); sprintf(scoreMsg, "Pontuacao Final: %d", score);
         drawTextCentered(title, centerX, centerY + 80, GLUT_BITMAP_TIMES_ROMAN_24, 1.0f, 1.0f, 1.0f); drawTextCentered(scoreMsg, centerX, centerY + 20, GLUT_BITMAP_HELVETICA_18, 1.0f, 1.0f, 0.0f);
         
         float btnW = 200; float btnH = 50; float btnY = centerY - 80; if (gameWon) glColor3f(0.2f, 0.6f, 0.2f); else glColor3f(0.8f, 0.4f, 0.0f);
         glBegin(GL_QUADS); glVertex2f(centerX - btnW/2, btnY - btnH/2); glVertex2f(centerX + btnW/2, btnY - btnH/2); glVertex2f(centerX + btnW/2, btnY + btnH/2); glVertex2f(centerX - btnW/2, btnY + btnH/2); glEnd();
         
         glColor3f(1.0f, 1.0f, 1.0f); glLineWidth(1.0f); glBegin(GL_LINE_LOOP); glVertex2f(centerX - btnW/2, btnY - btnH/2); glVertex2f(centerX + btnW/2, btnY - btnH/2); glVertex2f(centerX + btnW/2, btnY + btnH/2); glVertex2f(centerX - btnW/2, btnY + btnH/2); glEnd();
         drawTextCentered("REINICIAR", centerX, btnY - 5, GLUT_BITMAP_HELVETICA_18, 1.0f, 1.0f, 1.0f); drawTextCentered("(Ou pressione 'R')", centerX, centerY - 130, GLUT_BITMAP_HELVETICA_12, 0.7f, 0.7f, 0.7f);
    }
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void resetGame() {
    printf("--- INICIANDO RESET ---\n");

    // 1. Limpeza Geral
    if (g_slingshotManager) g_slingshotManager->setProjectile(nullptr);
    passaroAtual = nullptr;

    for (auto* p : filaPassaros) delete p;
    filaPassaros.clear();

    // Limpa inimigos
    for (auto* p : porcos) { if(p) { if(p->getRigidBody() && dynamicsWorld) dynamicsWorld->removeRigidBody(p->getRigidBody()); delete p; } }
    porcos.clear();
    for (auto* c : canhoes) { if(c) { if(c->getRigidBody() && dynamicsWorld) dynamicsWorld->removeRigidBody(c->getRigidBody()); delete c; } }
    canhoes.clear();
    for (auto* b : blocos) { if(b) { b->limparFisica(dynamicsWorld); delete b; } }
    blocos.clear();
    for (auto* e : extraBirds) { if(e) { e->limparFisica(); delete e; } }
    extraBirds.clear();

    if (g_slingshotManager) { delete g_slingshotManager; g_slingshotManager = nullptr; }

    // 2. Reinicia Física
    initBullet(); 
    for (auto& bloco : blocos) bloco->carregarTexturas();

    // 3. Reseta Variáveis
    score = 0;
    shotsRemaining = 8;
    gameOver = false;
    gameWon = false;
    gameLost = false;
    animandoEntradaPassaro = false; // Importante resetar animação

    // 4. RECRIA OS PÁSSAROS
    for (int i = 0; i < 8; i++) {
        Passaro* novoPassaro = nullptr;
        int tipo = i % 4; 
        if (tipo == 0) novoPassaro = new PassaroRed(0,0,0);
        else if (tipo == 1) novoPassaro = new PassaroChuck(0,0,0);
        else if (tipo == 2) novoPassaro = new PassaroBomb(0,0,0);
        else novoPassaro = new PassaroBlue(0,0,0);
        
        filaPassaros.push_back(novoPassaro);
    }

    // 5. POSICIONA A FILA
    float startX = -8.0f;  
    float startZ = 16.0f;  
    float gap = 1.8f;      
    float alturaChao = 0.65f; 

    for (int i = 0; i < filaPassaros.size(); i++) {
        filaPassaros[i]->setNaFila(true, btVector3(startX - (i * gap), alturaChao, startZ));
        filaPassaros[i]->setRotacaoVisual(0, 1, 0, M_PI / 2.0f); // Olhando para o cenário (M_PI / 2 para direita/fundo)
    }

    // 6. Prepara o Primeiro Pássaro (Correção do Som)
    itPassaroAtual = filaPassaros.begin();
    if (itPassaroAtual != filaPassaros.end()) {
        passaroAtual = *itPassaroAtual;
        passaroAtual->setNaFila(false, btVector3(0,0,0)); 
        
        // Garante que o pássaro está ativo para o timer não chamar proximoPassaro()
        passaroAtual->setAtivo(true);
        passaroAtual->setEmVoo(false);
        
        passaroAtual->setRotacaoVisual(0, 1, 0, M_PI); // Rotação correta no estilingue
    }

    // 7. Recria o Estilingue
    g_slingshotManager = new SlingshotManager(dynamicsWorld, passaroAtual, &shotsRemaining, &gameOver);
    if (passaroAtual) g_slingshotManager->setProjectile(passaroAtual);
    
    g_audioManager.playMusic(MusicaTipo::JOGO);
    printf("--- RESET CONCLUIDO ---\n");
}

// --- Callbacks do GLUT ---
//modificações para mostrar a tela inicial
void mouse(int button, int state, int x, int y) {
    // --- LÓGICA DE CLIQUE NO BOTÃO REINICIAR (NOVA) ---
    if (gameOver || gameWon || gameLost) {
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            // Recalcula a posição do botão (mesma lógica do drawHUD)
            float centerX = WIDTH / 2.0f;
            float centerY = HEIGHT / 2.0f;
            float btnY_center = centerY - 80;
            float btnW = 200;
            float btnH = 50;

            // Importante: O mouse do GLUT tem Y=0 no topo. 
            // O OpenGL (drawHUD) tem Y=0 no fundo.
            // Convertemos o Y do mouse:
            float mouseY_GL = HEIGHT - y; 

            // Verifica colisão do mouse com o retângulo do botão
            bool clicouX = (x >= centerX - btnW/2) && (x <= centerX + btnW/2);
            bool clicouY = (mouseY_GL >= btnY_center - btnH/2) && (mouseY_GL <= btnY_center + btnH/2);

            if (clicouX && clicouY) {
                resetGame(); // Chama o reset se clicou no botão
                return;
            }
        }
        // Se o jogo acabou, não processa mais nada (não deixa atirar o estilingue)
        return; 
    }
    // --------------------------------------------------

    if (g_currentState == STATE_GAME) {
        // Jogo normal
        if (g_slingshotManager) {
            g_slingshotManager->handleMouseClick(button, state, x, y);
        }
    } 
    else {
        // Menu
        if (g_menu) {
            GameState oldState = g_currentState; 
            GameState newState = g_menu->handleMouseClick(button, state, x, y, g_currentState);
            if (oldState != STATE_GAME && newState == STATE_GAME) {
                g_audioManager.playPassaro(SomTipo::SAINDO_MENU, 100);
                SDL_Delay(500);
                g_audioManager.stopMusic();
                g_audioManager.playMusic(MusicaTipo::JOGO);

            }

            if (newState == STATE_EXIT){ 
                exit(0);
            }
            g_currentState = newState;
        }
    }
    
    // Habilidade especial (mantida)
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
            g_audioManager.playPassaro(SomTipo::ENTRANDO_MENU, 100);
            g_audioManager.playMusic(MusicaTipo::MENU, 10);
            g_currentState = STATE_MENU; // Pausa o jogo e abre o menu
            return;
        } 
        else if (g_currentState == STATE_SETTINGS) {
            g_currentState = STATE_MENU; // Volta das configurações para o menu principal
            g_audioManager.playPassaro(SomTipo::SAINDO_MENU, 100);
            g_audioManager.stopMusic();
            g_audioManager.playMusic(MusicaTipo::JOGO, 10);
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
           resetGame();
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
        return;
    }
    frameCount++;
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    int timeInterval = currentTime - previousTime;

    if (timeInterval > 1000) {
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
        
        if (animandoEntradaPassaro) {
            // Se está na animação de entrada, desenha onde o timer calculou
            passaroAtual->desenharEmPosicao(passaroAtual->getX(), passaroAtual->getY(), passaroAtual->getZ());
        }
        else if (passaroAtual->getRigidBody()) {
            passaroAtual->desenhar(); 
        } 
        else if (g_slingshotManager) {
            float px, py, pz;
            g_slingshotManager->getPouchPosition(px, py, pz);
            passaroAtual->desenharEmPosicao(px, py, pz);
        }
    }

    // --- DESENHO DA FILA (NOVO) ---
    for (auto* p : filaPassaros) {
        // Desenha apenas se estiver no modo "Fila" e não for o atual
        if (p != passaroAtual && p->isNaFila()) {
            p->desenharNaFila(); // Usa o método novo que considera o pulinho
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

bool verificarVitoria() {
    // --- CORREÇÃO ---
    // Removemos o "if (porcos.empty()) return false".
    // Agora, se a lista estiver vazia (porque demos .erase em todos),
    // o código vai pular os loops for e retornar true (VITÓRIA),
    // que é exatamente o que queremos!

    // Verifica se tem algum porco vivo sobrando
    for (auto& p : porcos) {
        if (p->isAtivo()) return false; // Ainda tem inimigo, não ganhou.
    }
    
    // Verifica se tem algum canhão vivo sobrando
    for (auto& c : canhoes) {
        if (c->isAtivo()) return false; // Ainda tem inimigo, não ganhou.
    }
    
    // Se passou pelos loops e não retornou false (ou se as listas estão vazias),
    // significa que todos morreram.
    return true; 
}

void timer(int value) {
    float deltaTime = 1.0f / 60.0f;
    
    // Variável estática para contar quanto tempo o jogo está rodando.
    static float tempoDecorrido = 0.0f;
    tempoDecorrido += deltaTime;

    if (g_currentState != STATE_GAME) {
        glutPostRedisplay();         // Redesenha (para o menu aparecer)
        glutTimerFunc(16, timer, 0); // Mantém o loop rodando
        return;                      // <--- CANCELA A FÍSICA E A LÓGICA ABAIXO
    }

    if (gameWon || gameLost || gameOver) {
        glutPostRedisplay();
        glutTimerFunc(16, timer, 0);
        return;
    }
    // Simula a física (Verificação de segurança adicionada)
    if (dynamicsWorld) {
        dynamicsWorld->stepSimulation(deltaTime, 10, 1.0f / 180.0f);
    }
    
    // ============================================================
    // 1. ATUALIZAÇÃO DE LÓGICA DE JOGO (Sempre roda)
    // ============================================================

    if (animandoEntradaPassaro && passaroAtual) {
        animEntradaTimer += deltaTime * 1.5f; 
        
        if (animEntradaTimer >= 1.0f) {
            // CHEGOU! 
            animandoEntradaPassaro = false;
            
            passaroAtual->setNaFila(false, animEndPos); 
            passaroAtual->resetar(animEndPos.x(), animEndPos.y(), animEndPos.z());
            
            // --- CORREÇÃO DE ROTAÇÃO ---
            // Fixa a rotação olhando para os blocos (M_PI ou 180 graus).
            // Sem spins, sem virar para trás.
            passaroAtual->setRotacaoVisual(0, 1, 0, M_PI); 
            
            if (g_slingshotManager) g_slingshotManager->setProjectile(passaroAtual);
            
        } else {
            // NO AR
            float t = animEntradaTimer;
            btVector3 currentPos = animStartPos.lerp(animEndPos, t);
            float alturaPulo = sin(t * M_PI) * 5.0f;
            currentPos.setY(currentPos.y() + alturaPulo);
            
            // --- CORREÇÃO DE ROTAÇÃO DURANTE O PULO ---
            // Removemos o "t * M_PI * 4.0" (giro).
            // Mantemos ele olhando para o alvo o tempo todo (M_PI).
            // Se quiser que ele olhe para cima enquanto sobe, podemos ajustar,
            // mas M_PI fixo resolve o problema dele olhar para trás.
            passaroAtual->setRotacaoVisual(0, 1, 0, M_PI); 
            
            passaroAtual->setPosicao(currentPos.x(), currentPos.y(), currentPos.z());
        }
    }
    
    // --- ATUALIZA A FILA ---
    for (auto* p : filaPassaros) {
        if (p != passaroAtual && p->isNaFila()) {
            p->atualizarFila(deltaTime);
        }
    }

    // Atualiza Blocos
    for (auto& bloco : blocos) {
        bloco->update(deltaTime);
        bloco->clearContactFlag();
    }

    // Atualiza Pássaro Atual
    if (passaroAtual) {
        passaroAtual->atualizar(deltaTime);
        if (!passaroAtual->isAtivo()) {
            g_particleManager.createSmokeEffect(passaroAtual->getPosicao(), 20);
            proximoPassaro();
        }
    }

    // Atualiza Pássaros Extras
    for (auto it = extraBirds.begin(); it != extraBirds.end(); ) {
        (*it)->atualizar(deltaTime);
        if (!(*it)->isAtivo()) {
            g_particleManager.createSmokeEffect((*it)->getPosicao(), 20);
            delete *it;
            it = extraBirds.erase(it);
        } else {
            ++it;
        }
    }

    // Atualiza Porcos
    for (auto& porco : porcos) {
        porco->atualizar(deltaTime);
    }

    // Atualiza Canhões (CRÍTICO: ISSO FAZ ELES ATIREM)
    for (auto& c : canhoes) {
        c->atualizar(deltaTime);
    }

    g_particleManager.update(deltaTime);


    // ============================================================
    // 2. CORREÇÃO DE ESTABILIDADE FÍSICA (Roda após 2~3 seg)
    // ============================================================
    // Isso "solta o freio" (tira o amortecimento) para os objetos caírem normal
    
    if (tempoDecorrido > 2.0f) {
        // Solta os Porcos
        for (auto& porco : porcos) {
            if (porco->getRigidBody()) {
                if (porco->getRigidBody()->getLinearDamping() > 0.1f) {
                    porco->getRigidBody()->setDamping(0.0f, 0.0f); 
                }
                porco->getRigidBody()->activate(true); 
            }
        }
        // Solta os Canhões
        for (auto& c : canhoes) {
            if (c->getRigidBody()) {
                if (c->getRigidBody()->getLinearDamping() > 0.1f) {
                    c->getRigidBody()->setDamping(0.0f, 0.0f);
                }
                c->getRigidBody()->activate(true);
            }
        }
    }

    // Solta os Blocos (Um pouco depois, 3 segundos)
    if (tempoDecorrido > 3.0f) {
        for (auto& bloco : blocos) {
            btRigidBody* rb = bloco->getRigidBody();
            if (rb) {
                if (rb->getLinearDamping() > 0.5f) {
                    rb->setDamping(0.1f, 0.1f);
                }
                // Opcional: Otimização de sono para blocos parados
                if (rb->getLinearVelocity().length2() < 0.01f) {
                     // rb->setActivationState(ISLAND_SLEEPING); 
                }
            }
        }
    }

    // ============================================================
    // 3. LÓGICA DE COLISÃO E DANO
    // ============================================================
    if (dynamicsWorld) {
        int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
        for (int i = 0; i < numManifolds; i++) {
            btPersistentManifold* contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
            const btCollisionObject* obA = contactManifold->getBody0();
            const btCollisionObject* obB = contactManifold->getBody1();

            BlocoDestrutivel* blocoA = nullptr;
            BlocoDestrutivel* blocoB = nullptr;
            Porco* porco = nullptr;
            
            for (auto& b : blocos) {
                if (b->getRigidBody() == obA) { blocoA = b; break; }
            }
            for (auto& b : blocos) {
                if (b->getRigidBody() == obB) { blocoB = b; break; }
            }
            for (auto& p : porcos) {
                if (p->getRigidBody() == obA || p->getRigidBody() == obB) {
                    porco = p; break;
                }
            }

            // Colisão Estilingue x Porco (Projétil)
            if (g_slingshotManager) {
                btRigidBody* slingshotBody = g_slingshotManager->getRigidBody();
                
                if (slingshotBody && (obA == slingshotBody || obB == slingshotBody)) {
                    const btCollisionObject* otherOb = (obA == slingshotBody) ? obB : obA;
                    
                    // 1. Verifica Porcos normais (que caem da torre)
                    for (auto& p : porcos) {
                        if (p->getRigidBody() == otherOb) {
                            if (p->getRigidBody()->getLinearVelocity().length() > 0.5f) { 
                                g_slingshotManager->takeDamage();
                                p->tomarDano(500.0f);
                            }
                            break; 
                        }
                    }

                    // 2. Verifica Projéteis dos Canhões (NOVO)
                    for (auto& canhao : canhoes) {
                        // Pega a lista de balas deste canhão
                        const auto& balas = canhao->getProjectiles();
                        
                        for (auto& bala : balas) {
                            if (bala->getRigidBody() == otherOb) {
                                // Se a bala bater rápido no estilingue
                                if (bala->getRigidBody()->getLinearVelocity().length() > 0.5f) {
                                    printf("Bala de canhao acertou o estilingue!\n");
                                    g_slingshotManager->takeDamage();
                                    
                                    // A bala explode ao bater
                                    bala->tomarDano(9999.0f); 
                                    g_particleManager.createExplosion(bala->getPosicao(), btVector3(0,0,0));
                                }
                                break;
                            }
                        }
                    }
                }
            }
                        
            bool passaroEnvolvido = passaroAtual && (obA == passaroAtual->getRigidBody() || obB == passaroAtual->getRigidBody());

            float impulsoTotal = 0;
            for (int j = 0; j < contactManifold->getNumContacts(); j++) {
                impulsoTotal += contactManifold->getContactPoint(j).getAppliedImpulse();
            }

            // Dano Bloco x Bloco
            if (blocoA && blocoB) {
                if (tempoDecorrido > 4.0f && impulsoTotal > 8.0f) {
                    float dano = impulsoTotal * 0.15f; 
                    blocoA->aplicarDano(dano);
                    blocoB->aplicarDano(dano);
                }
            }
            // Dano Pássaro x Bloco
            else if ((blocoA || blocoB) && passaroEnvolvido) {
                BlocoDestrutivel* alvo = (blocoA) ? blocoA : blocoB;
                float dano = (impulsoTotal * impulsoTotal) * 0.005f; 
                if (dano > 1.2f) {
                    alvo->aplicarDano(dano);
                    // (Aqui você pode adicionar sua lógica de som se quiser)
                }
            }
            // Dano Bloco x Chão
            else if ((blocoA || blocoB) && (obA == groundRigidBody || obB == groundRigidBody)) {
                if (tempoDecorrido > 2.0f && impulsoTotal > 1.0f) {
                    BlocoDestrutivel* alvo = (blocoA) ? blocoA : blocoB;
                    float velocidadeQueda = -alvo->getRigidBody()->getLinearVelocity().getY(); 
                    if (velocidadeQueda > 5.0f) { 
                        alvo->aplicarDano(velocidadeQueda * 3.0f);
                    }
                }
            }
            // Dano Porco
            if (porco) {
                if (tempoDecorrido > 1.0f && impulsoTotal > 1.5f) {
                    porco->tomarDano(impulsoTotal * 0.2f);
                }
            }
        }
    }

    // ============================================================
    // 4. LIMPEZA E REDISPLAY
    // ============================================================

    // Limpa blocos destruídos
    for (int i = blocos.size() - 1; i >= 0; i--) {
        if (blocos[i]->isDestruido()) {
            // ADICIONADO: Soma a pontuação antes de deletar
            score += blocos[i]->getPontuacao(); 
            
            blocos[i]->limparFisica(dynamicsWorld);
            delete blocos[i];
            blocos.erase(blocos.begin() + i);
        }
    }

    // --- PORCOS (ADICIONADO) ---
    for (int i = porcos.size() - 1; i >= 0; i--) {
        // Se o porco não está ativo (morreu por dano ou caiu do mundo)
        if (!porcos[i]->isAtivo()) {
            score += 5000; // Valor fixo para cada porco (Angry Birds padrão)
            
            // Garante que a física foi limpa e deleta o objeto
            porcos[i]->limparFisica(); 
            delete porcos[i];
            porcos.erase(porcos.begin() + i);
        }
    }

    // --- CANHÕES (ADICIONADO) ---
    for (int i = canhoes.size() - 1; i >= 0; i--) {
        if (!canhoes[i]->isAtivo()) {
            score += 3000; // Valor fixo para destruir canhões
            
            // Cannon herda de Porco, então limparFisica funciona
            canhoes[i]->limparFisica(); 
            delete canhoes[i];
            canhoes.erase(canhoes.begin() + i);
        }
    }

    if (g_slingshotManager) g_slingshotManager->update();
    
    // Verifica objetos caindo no abismo (apenas para aplicar dano massivo, a pontuação é feita nos loops acima)
    if (dynamicsWorld) {
        // Blocos caindo
        for (auto& b : blocos) {
            if (!b->isDestruido() && b->getRigidBody()) {
                btTransform trans;
                b->getRigidBody()->getMotionState()->getWorldTransform(trans);
                if (trans.getOrigin().getY() < -10.0f) { 
                    b->aplicarDano(99999.0f); // Mata o bloco (score será somado no próximo frame no loop acima)
                }
            }
        }

        // Porcos caindo (Se ainda estiverem ativos e com corpo)
        for (auto& p : porcos) {
            if (p->isAtivo() && p->getRigidBody()) {
                if (p->getPosicao().getY() < -10.0f) {
                    p->tomarDano(99999.0f); // Mata o porco (score será somado no próximo frame)
                }
            }
        }

        // Canhões caindo
        for (auto& c : canhoes) {
            if (c->isAtivo() && c->getRigidBody()) {
                if (c->getPosicao().getY() < -10.0f) {
                    c->tomarDano(99999.0f);
                }
            }
        }
    }

    if (g_currentState == STATE_GAME && !gameWon && !gameLost && verificarVitoria()) {
        gameWon = true;
        g_audioManager.stopMusic(); 
        g_audioManager.playMusic(MusicaTipo::VITORIA, 50);
        printf("VITORIA: Todos os inimigos eliminados!\n");
    }

    // Derrota por falta de pássaros
    if (g_currentState == STATE_GAME && !gameWon && !gameLost && passaroAtual == nullptr) {
        gameLost = true;
        g_audioManager.stopMusic();
        g_audioManager.playMusic(MusicaTipo::DERROTA, 50);
        printf("DERROTA: Acabaram os passaros!\n");
    }
    
    // --- ADICIONE ESTE BLOCO AQUI ---
    // Derrota por Destruição do Estilingue (gameOver vira true quando toma 3 tiros)
    if (g_currentState == STATE_GAME && !gameWon && !gameLost && gameOver) {
        gameLost = true; // Marca oficialmente como derrota para evitar repetição
        
        g_audioManager.stopMusic();
        
        g_audioManager.playMusic(MusicaTipo::DERROTA, 50);
        
        printf("DERROTA: Estilingue destruido!\n");
    }
    
    if (gameWon || gameOver) { // Use 'gameOver' aqui se for a variável de derrota
        glutPostRedisplay();
        glutTimerFunc(16, timer, 0);
        return; // IMPORTANTE: Sai da função aqui
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

    g_heartTextureID = loadGlobalTexture("Objetos/texturas/heart.png");
    if (g_heartTextureID == 0) {
        printf("AVISO: Textura do coracao nao encontrada em Objetos/coracao.png!\n");
        // O jogo vai rodar, mas sem os corações.
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
        
        for (auto* p : filaPassaros) delete p;
        filaPassaros.clear();

        for (int i = 0; i < 8; i++) {
            Passaro* novoPassaro = nullptr;
            int tipo = i % 4; 
            if (tipo == 0) novoPassaro = new PassaroRed(0,0,0);
            else if (tipo == 1) novoPassaro = new PassaroChuck(0,0,0);
            else if (tipo == 2) novoPassaro = new PassaroBomb(0,0,0);
            else novoPassaro = new PassaroBlue(0,0,0);
            filaPassaros.push_back(novoPassaro);
        }

        float startX = -8.0f;  
        float startZ = 16.0f;  
        float gap = 1.8f;      
        float alturaChao = 0.65f; 

        for (int i = 0; i < filaPassaros.size(); i++) {
            filaPassaros[i]->setNaFila(true, btVector3(startX - (i * gap), alturaChao, startZ));
            
            // --- CORREÇÃO DE ROTAÇÃO ---
            // Mudei de M_PI/2 para -M_PI/2 (ou M_PI * 1.5). 
            // Agora eles olham para a direita (para o estilingue).
            filaPassaros[i]->setRotacaoVisual(0, 1, 0, -M_PI / 2.0f); 
        }

        // --- CORREÇÃO: ANIMAR O PRIMEIRO PÁSSARO ---
        itPassaroAtual = filaPassaros.begin();
        if (itPassaroAtual != filaPassaros.end()) {
            passaroAtual = *itPassaroAtual;
            
            // Configura a animação igualzinho ao proximoPassaro
            animandoEntradaPassaro = true;
            animEntradaTimer = 0.0f;
            animStartPos = passaroAtual->getPosFila();
            // Assumimos posição padrão do estilingue se o manager não estiver pronto, 
            // ou ajustamos no primeiro frame. Vamos por uma posição fixa segura perto do centro.
            animEndPos = btVector3(0.0f, 2.5f, 16.0f); 
            
            // Não tira da fila ainda!
        }
        // --- RESTANTE DA INICIALIZAÇÃO (MANTIDO IGUAL) ---
        init();

        if (!g_audioManager.initAudio()) {
            printf("AVISO: Audio desabilitado devido a falha na inicializacao.\n");
        }

        g_audioManager.setVolume(10.0f); 
        g_audioManager.playMusic(MusicaTipo::MENU, 50); 
        
        glutMouseFunc(mouse);
        glutMotionFunc(mouseMotion);
        glutPassiveMotionFunc(passiveMouseMotion);
        glutKeyboardFunc(keyboard);
        glutSpecialFunc(specialKeys);
        
        glutTimerFunc(0, timer, 0);
        jogoCarregado = true; 
        glutPostRedisplay();  
        printf("=== CARREGAMENTO CONCLUIDO ===\n");
    }
}

int main(int argc, char** argv) {
    // 1. Inicialização Básica do GLUT (Janela)
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
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