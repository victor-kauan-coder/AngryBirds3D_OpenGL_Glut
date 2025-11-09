#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
// #include "../openGL_util.h"
#include "passaro.h"

/**
 * Exemplo de classe derivada: Red (Pássaro vermelho - padrão)
 * Este é o pássaro básico do Angry Birds
 */
class PassaroRed : public Passaro {
public:
    PassaroRed(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f)
        : Passaro(posX, posY, posZ, 0.5f, 1.0f) {
        setCor(1.0f, 0.0f, 0.0f); // Vermelho
        setMassa(1.0f);
        setTipo("Red");
        carregarModelo("Objetos/red.obj");
        carregarMTL("Objetos/red.mtl");
        
    }
    
    void usarHabilidade() override {
        // Red não tem habilidade especial, apenas é forte e pesado
        // Poderia emitir um grito ou algo assim
    }
};

// ===== DEMONSTRAÇÃO DE USO =====

// Variáveis globais para a demonstração
PassaroRed* red;

// Bullet Physics
btBroadphaseInterface* broadphase;
btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld;

// Chão
btRigidBody* groundRigidBody = nullptr;
btCollisionShape* groundShape = nullptr;

float tempoDecorrido = 0.0f;

// Variáveis da câmera
float cameraAngleX = 20.0f;
float cameraAngleY = 45.0f;
float cameraDistance = 10.0f;

// Debug de colisão
bool mostrarColisao = true;


void setupLighting() {
    // Habilita iluminação
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    // Configuração da luz
    GLfloat light_position[] = { 5.0f, 5.0f, 5.0f, 1.0f };
    GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
}

void drawGrid() {
    // Desenha um grid de referência
    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_LINES);
    for (float i = -10.0f; i <= 10.0f; i += 1.0f) {
        glVertex3f(i, 0.0f, -10.0f);
        glVertex3f(i, 0.0f, 10.0f);
        glVertex3f(-10.0f, 0.0f, i);
        glVertex3f(10.0f, 0.0f, i);
    }
    glEnd();
    
    // Eixos X, Y, Z
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); // X = vermelho
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(2.0f, 0.0f, 0.0f);
    
    glColor3f(0.0f, 1.0f, 0.0f); // Y = verde
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 2.0f, 0.0f);
    
    glColor3f(0.0f, 0.0f, 1.0f); // Z = azul
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 2.0f);
    glEnd();
    glEnable(GL_LIGHTING);
}

void desenharEsferaColisao(btRigidBody* rigidBody, float raio, float escala) {
    if (!rigidBody) return;
    
    btTransform transform;
    rigidBody->getMotionState()->getWorldTransform(transform);
    btVector3 pos = transform.getOrigin();
    
    glPushMatrix();
    glTranslatef(pos.x(), pos.y(), pos.z());
    
    // Desabilita iluminação para o wireframe
    glDisable(GL_LIGHTING);
    
    // Verde brilhante para a esfera de colisão
    glColor3f(0.0f, 1.0f, 0.0f);
    
    // Desenha wireframe
    glutWireSphere(raio * escala, 16, 16);
    
    // Reabilita iluminação
    glEnable(GL_LIGHTING);
    
    glPopMatrix();
}

void desenharChaoColisao() {
    if (!groundRigidBody || !groundShape) return;
    
    // Obtém a posição real do chão
    btTransform transform;
    groundRigidBody->getMotionState()->getWorldTransform(transform);
    btVector3 pos = transform.getOrigin();
    
    // Obtém o tamanho da box shape
    btBoxShape* boxShape = static_cast<btBoxShape*>(groundShape);
    btVector3 halfExtents = boxShape->getHalfExtentsWithMargin();
    
    // Desenha o wireframe do chão na posição real
    glDisable(GL_LIGHTING);
    glColor3f(0.5f, 0.5f, 0.5f);
    
    glPushMatrix();
    // Usa a posição real do rigidBody
    glTranslatef(pos.x(), pos.y(), pos.z());
    
    // Aplica a rotação do rigidBody
    btQuaternion rot = transform.getRotation();
    btVector3 axis = rot.getAxis();
    float angle = rot.getAngle();
    if (angle != 0.0f) {
        glRotatef(angle * 180.0f / M_PI, axis.x(), axis.y(), axis.z());
    }
    
    // Escala com o tamanho real da box
    glScalef(halfExtents.x(), halfExtents.y(), halfExtents.z());
    glutWireCube(2.0f);
    glPopMatrix();
    
    glEnable(GL_LIGHTING);
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    setupCamera();
    setupLighting();
    
    // Desenha o grid de referência
    drawGrid();
    
    // Desenha o pássaro
    if (red) red->desenhar();
    
    // Desenha as esferas de colisão (debug)
    if (mostrarColisao) {
        if (red && red->getRigidBody()) {
            desenharEsferaColisao(red->getRigidBody(), red->getRaioColisao(), red->getEscala());
        }
        desenharChaoColisao();
    }
    
    glutSwapBuffers();
}

void timer(int value) {
    const float deltaTime = 0.016f; // ~60 FPS
    tempoDecorrido += deltaTime;
    
    // Atualiza o mundo de física do Bullet
    if (dynamicsWorld) {
        dynamicsWorld->stepSimulation(deltaTime, 10);
    }
    
    // Atualiza física dos pássaros
    if (red) red->atualizar(deltaTime);
    
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 27: // ESC
            exit(0);
            break;
        case '1': // Lançar Red
            if (red) {
                red->lancar(5.0f, 3.0f, 0.0f);
            }
            break;
        case 'r':
        case 'R': // Resetar
            if (red) {
                red->resetar(0.0f, 2.0f, 0.0f);
            }
            break;
        case 'c':
        case 'C': // Toggle colisão
            mostrarColisao = !mostrarColisao;
            printf("Colisao debug: %s\n", mostrarColisao ? "ATIVADO" : "DESATIVADO");
            break;
    }
}

void cleanupBullet() {
    // Remove os rigid bodies do mundo
    if (dynamicsWorld) {
        if (red && red->getRigidBody()) {
            dynamicsWorld->removeRigidBody(red->getRigidBody());
        }
        if (groundRigidBody) {
            dynamicsWorld->removeRigidBody(groundRigidBody);
        }
    }
    
    // Deleta o chão
    if (groundRigidBody) {
        delete groundRigidBody->getMotionState();
        delete groundRigidBody;
    }
    delete groundShape;
    
    // Deleta os objetos do Bullet na ordem correta
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;
}



int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Angry Birds - Sistema de Passaros");
    
    // Configurações OpenGL
    glEnable(GL_DEPTH_TEST);              
    glDisable(GL_CULL_FACE);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Azul céu
    
    // ===== INICIALIZAR BULLET PHYSICS =====
    printf("Inicializando Bullet Physics...\n");
    
    // Configuração da detecção de colisão
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    
    // Broadphase (detecção de colisão em larga escala)
    broadphase = new btDbvtBroadphase();
    // Solver de restrições
    solver = new btSequentialImpulseConstraintSolver();
    
    // Cria o mundo de física
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    
    // Define a gravidade
    dynamicsWorld->setGravity(btVector3(0, -9.8f, 0));
    
    printf("Mundo de fisica criado!\n");
    
    // ===== CRIAR CHÃO =====
    groundShape = new btBoxShape(btVector3(50.0f, 0.5f, 50.0f));
    btDefaultMotionState* groundMotionState = new btDefaultMotionState(
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0))
    );
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(
        0, groundMotionState, groundShape, btVector3(0, 0, 0)
    );
    groundRigidBody = new btRigidBody(groundRigidBodyCI);
    dynamicsWorld->addRigidBody(groundRigidBody);
    
    printf("Chao criado em posicao: (%.2f, %.2f, %.2f)\n", 
           groundRigidBody->getWorldTransform().getOrigin().x(),
           groundRigidBody->getWorldTransform().getOrigin().y(),
           groundRigidBody->getWorldTransform().getOrigin().z());
    
    // ===== CRIAR PÁSSAROS =====
    red = new PassaroRed(0.0f, 2.0f, 0.0f);
    
    // Carregar modelo 3D
    // printf("Carregando modelo OBJ...\n");
    // if (red->carregarModelo("Objetos/red.obj")) {
    //     printf("Modelo carregado com sucesso!\n");
    // } else {
    //     printf("AVISO: Falha ao carregar modelo. Usando esfera padrao.\n");
    // }
    
    // IMPORTANTE: Inicializar física do pássaro
    red->inicializarFisica(dynamicsWorld, 0.0f, 2.0f, 0.0f);
    printf("Fisica do passaro inicializada!\n");

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timer, 0);
    
    printf("\n=== Controles ===\n");
    printf("1 - Lancar Red (vermelho)\n");
    printf("R - Resetar passaro\n");
    printf("C - Toggle debug de colisao (wireframe)\n");
    printf("ESC - Sair\n");
    printf("\n");
    
    glutMainLoop();
    
    // Limpeza
    printf("Encerrando...\n");
    delete red;
    cleanupBullet();
    
    return 0;
}
