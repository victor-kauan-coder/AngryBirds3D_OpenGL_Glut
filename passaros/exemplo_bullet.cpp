#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <vector>
#include "passaro.h"
#include "../openGL_util.h"

/**
 * EXEMPLO DE USO DO SISTEMA DE PÁSSAROS COM BULLET PHYSICS
 * 
 * Este exemplo mostra como:
 * 1. Inicializar o mundo de física do Bullet
 * 2. Criar pássaros com física realista
 * 3. Adicionar obstáculos (porcos, estruturas)
 * 4. Detectar colisões
 * 5. Lançar pássaros com física realista
 */

// ===== CONFIGURAÇÃO DO BULLET PHYSICS =====

// Componentes principais do Bullet
btBroadphaseInterface* broadphase;
btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld;

// Objetos do mundo
std::vector<btRigidBody*> objetosFisicos;

/**
 * Inicializa o mundo de física do Bullet
 */
void inicializarBullet() {
    // Configuração de colisão
    collisionConfiguration = new btDefaultCollisionConfiguration();
    
    // Dispatcher de colisão
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    
    // Broadphase (otimização de detecção de colisão)
    broadphase = new btDbvtBroadphase();
    
    // Solver de restrições
    solver = new btSequentialImpulseConstraintSolver();
    
    // Cria o mundo de física
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    
    // Define gravidade (Y para cima no OpenGL)
    dynamicsWorld->setGravity(btVector3(0, -9.8f, 0));
    
    printf("Bullet Physics inicializado com sucesso!\n");
}

/**
 * Limpa recursos do Bullet
 */
void limparBullet() {
    // Remove todos os rigid bodies
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState()) {
            delete body->getMotionState();
        }
        dynamicsWorld->removeCollisionObject(obj);
        delete obj;
    }
    
    // Limpa objetos adicionais
    for (auto* obj : objetosFisicos) {
        if (obj->getCollisionShape()) {
            delete obj->getCollisionShape();
        }
    }
    objetosFisicos.clear();
    
    // Deleta mundo e componentes
    delete dynamicsWorld;
    delete solver;
    delete broadphase;
    delete dispatcher;
    delete collisionConfiguration;
    
    printf("Bullet Physics limpo!\n");
}

/**
 * Cria um chão estático
 */
void criarChao() {
    btCollisionShape* groundShape = new btBoxShape(btVector3(50.0f, 0.5f, 50.0f));
    
    btTransform groundTransform;
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, -0.5f, 0));
    
    btScalar mass = 0.0f; // Massa 0 = objeto estático
    btVector3 localInertia(0, 0, 0);
    
    btDefaultMotionState* motionState = new btDefaultMotionState(groundTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, groundShape, localInertia);
    
    btRigidBody* body = new btRigidBody(rbInfo);
    body->setFriction(0.8f);
    body->setRestitution(0.3f);
    
    dynamicsWorld->addRigidBody(body);
    objetosFisicos.push_back(body);
}

/**
 * Desenha o chão
 */
void desenharChao() {
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-50, 0, -50);
    glVertex3f(-50, 0, 50);
    glVertex3f(50, 0, 50);
    glVertex3f(50, 0, -50);
    glEnd();
}

/**
 * Cria uma caixa obstáculo (porco ou estrutura)
 */
btRigidBody* criarCaixa(float x, float y, float z, float tamanho, float massa) {
    btCollisionShape* boxShape = new btBoxShape(btVector3(tamanho/2, tamanho/2, tamanho/2));
    
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(x, y, z));
    
    btVector3 localInertia(0, 0, 0);
    if (massa > 0.0f) {
        boxShape->calculateLocalInertia(massa, localInertia);
    }
    
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(massa, motionState, boxShape, localInertia);
    rbInfo.m_restitution = 0.4f;
    rbInfo.m_friction = 0.5f;
    
    btRigidBody* body = new btRigidBody(rbInfo);
    dynamicsWorld->addRigidBody(body);
    objetosFisicos.push_back(body);
    
    return body;
}

/**
 * Desenha uma caixa do Bullet
 */
void desenharCaixa(btRigidBody* body, float tamanho) {
    btTransform transform;
    body->getMotionState()->getWorldTransform(transform);
    
    float matriz[16];
    transform.getOpenGLMatrix(matriz);
    
    glPushMatrix();
    glMultMatrixf(matriz);
    glColor3f(0.0f, 0.8f, 0.0f); // Verde (porco)
    glutSolidCube(tamanho);
    glPopMatrix();
}

// ===== EXEMPLO DE CLASSE RED COM BULLET =====

class PassaroRed : public Passaro {
public:
    PassaroRed() : Passaro(0, 0, 0, 0.5f, 1.0f) {
        setCor(1.0f, 0.0f, 0.0f); // Vermelho
        setMassa(1.0f);
        setTipo("Red");
        setRestituicao(0.6f);
        setFriccao(0.5f);
    }
    
    void usarHabilidade() override {
        // Red não tem habilidade especial
        printf("SQUAWK! Red não tem habilidade especial!\n");
    }
};

class PassaroChuck : public Passaro {
private:
    bool habilidadeUsada;
    
public:
    PassaroChuck() : Passaro(0, 0, 0, 0.4f, 0.9f), habilidadeUsada(false) {
        setCor(1.0f, 1.0f, 0.0f); // Amarelo
        setMassa(0.8f);
        setTipo("Chuck");
        setRestituicao(0.7f);
    }
    
    void usarHabilidade() override {
        if (!habilidadeUsada && isEmVoo() && getRigidBody()) {
            // Acelera drasticamente
            btVector3 vel = getRigidBody()->getLinearVelocity();
            getRigidBody()->setLinearVelocity(vel * 2.5f);
            habilidadeUsada = true;
            printf("ZOOM! Chuck acelerou!\n");
        }
    }
    
    void resetar(float x, float y, float z) override {
        Passaro::resetar(x, y, z);
        habilidadeUsada = false;
    }
};

class PassaroBomb : public Passaro {
private:
    bool explodiu;
    
public:
    PassaroBomb() : Passaro(0, 0, 0, 0.6f, 1.2f), explodiu(false) {
        setCor(0.1f, 0.1f, 0.1f); // Preto
        setMassa(1.5f);
        setTipo("Bomb");
        setRestituicao(0.3f);
    }
    
    void usarHabilidade() override {
        if (!explodiu && getRigidBody()) {
            explodiu = true;
            printf("BOOM! Bomb explodiu!\n");
            
            // Para o pássaro
            getRigidBody()->setLinearVelocity(btVector3(0, 0, 0));
            
            // Aqui você poderia aplicar força em objetos próximos
            // simulando uma explosão
        }
    }
    
    void resetar(float x, float y, float z) override {
        Passaro::resetar(x, y, z);
        explodiu = false;
    }
};

// ===== VARIÁVEIS GLOBAIS DO JOGO =====

std::vector<Passaro*> passaros;
std::vector<btRigidBody*> obstaculos;
int passaroAtual = 0;
float tempoDecorrido = 0.0f;

// ===== FUNÇÕES DE RENDERIZAÇÃO =====

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Desenha o chão
    desenharChao();
    
    // Desenha obstáculos
    for (auto* obstaculo : obstaculos) {
        desenharCaixa(obstaculo, 1.0f);
    }
    
    // Desenha pássaros
    for (auto* passaro : passaros) {
        passaro->desenhar();
    }
    
    glutSwapBuffers();
}

void timer(int value) {
    const float deltaTime = 1.0f / 60.0f; // 60 FPS
    
    // Atualiza o mundo de física
    if (dynamicsWorld) {
        dynamicsWorld->stepSimulation(deltaTime, 10);
    }
    
    // Atualiza lógica dos pássaros
    for (auto* passaro : passaros) {
        passaro->atualizar(deltaTime);
        
        // Verifica colisões
        if (passaro->verificarColisao()) {
            // printf("%s colidiu!\n", passaro->getTipo().c_str());
        }
    }
    
    tempoDecorrido += deltaTime;
    
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case '1': // Lança o pássaro atual
            if (passaroAtual < passaros.size()) {
                passaros[passaroAtual]->lancar(10.0f, 5.0f, 0.0f);
                printf("Lançou %s!\n", passaros[passaroAtual]->getTipo().c_str());
            }
            break;
            
        case ' ': // Usa habilidade
            if (passaroAtual < passaros.size()) {
                passaros[passaroAtual]->usarHabilidade();
            }
            break;
            
        case 'n': // Próximo pássaro
            if (passaroAtual < passaros.size() - 1) {
                passaroAtual++;
                printf("Próximo pássaro: %s\n", passaros[passaroAtual]->getTipo().c_str());
            }
            break;
            
        case 'r': // Reseta tudo
            passaroAtual = 0;
            for (size_t i = 0; i < passaros.size(); i++) {
                passaros[i]->resetar(-10.0f, 1.0f + i * 2.0f, 0.0f);
            }
            printf("Resetado!\n");
            break;
            
        case 27: // ESC
            // Limpa tudo
            for (auto* passaro : passaros) {
                delete passaro;
            }
            limparBullet();
            exit(0);
            break;
    }
}

// ===== MAIN =====

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Angry Birds 3D - Bullet Physics");
    
    init();
    glEnable(GL_DEPTH_TEST);
    
    // Inicializa Bullet Physics
    inicializarBullet();
    
    // Cria o chão
    criarChao();
    
    // Cria obstáculos (porcos e estruturas)
    printf("Criando obstáculos...\n");
    for (int i = 0; i < 3; i++) {
        float x = 5.0f + i * 2.0f;
        obstaculos.push_back(criarCaixa(x, 0.5f, 0.0f, 1.0f, 0.5f));
    }
    
    // Cria os pássaros
    printf("Criando pássaros...\n");
    
    PassaroRed* red = new PassaroRed();
    red->inicializarFisica(dynamicsWorld, -10.0f, 1.0f, 0.0f);
    passaros.push_back(red);
    
    PassaroChuck* chuck = new PassaroChuck();
    chuck->inicializarFisica(dynamicsWorld, -10.0f, 3.0f, 0.0f);
    passaros.push_back(chuck);
    
    PassaroBomb* bomb = new PassaroBomb();
    bomb->inicializarFisica(dynamicsWorld, -10.0f, 5.0f, 0.0f);
    passaros.push_back(bomb);
    
    // Configura callbacks
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timer, 0);
    
    printf("\n=== CONTROLES ===\n");
    printf("1 - Lançar pássaro atual\n");
    printf("ESPAÇO - Usar habilidade especial\n");
    printf("N - Próximo pássaro\n");
    printf("R - Resetar cena\n");
    printf("ESC - Sair\n\n");
    
    printf("Pássaros disponíveis:\n");
    for (size_t i = 0; i < passaros.size(); i++) {
        printf("  %zu. %s\n", i+1, passaros[i]->getTipo().c_str());
    }
    printf("\nPressione 1 para lançar!\n");
    
    glutMainLoop();
    
    return 0;
}
