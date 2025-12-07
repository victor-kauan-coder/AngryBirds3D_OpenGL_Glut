#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include "../util/loads.h"
#include "../util/openGL_util.h"
#include "porco.h"
#include <iostream>
#include "../controle_audio/audio_manager.h"
#include <cstdlib>

extern AudioManager g_audioManager;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

Porco::Porco(float posX, float posY, float posZ, float raio, float escalaInicial)
    : rigidBody(nullptr),
      colisaoShape(nullptr),
      motionState(nullptr),
      mundoFisica(nullptr),
      escala(escalaInicial),
      raioColisao(0.5f),
      ativo(true),
      vidaMaxima(100.0f),
      vida(2.5f),
      massa(2.0f),
      tipo("Porco"),
      restituicao(0.5f),
      friccao(9.0f),
      amortecimentoLinear(0.2f),
      amortecimentoAngular(0.2f),
      timerPulo(0.0f),
      intervaloPulo(0.0f),
      tempoSemDano(0.0f) { // 3 segundos para sumir após sair da posição
    
    carregarModelo("Objetos/porco.obj");
    carregarMTL("Objetos/porco.mtl");
}

Porco::~Porco() {
    limparFisica();
}

void Porco::inicializarFisica(btDiscreteDynamicsWorld* mundo, float posX, float posY, float posZ) {
    mundoFisica = mundo;

    // Porcos podem ser caixas ou esferas. Esfera é mais simples.
    colisaoShape = new btSphereShape(raioColisao * escala * 0.3f);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(posX, posY, posZ));

    motionState = new btDefaultMotionState(transform);

    btVector3 inerciaLocal(0, 0, 0);
    if (massa > 0.0f) {
        colisaoShape->calculateLocalInertia(massa, inerciaLocal);
    }

    btRigidBody::btRigidBodyConstructionInfo rbInfo(massa, motionState, colisaoShape, inerciaLocal);
    rbInfo.m_restitution = restituicao;
    rbInfo.m_friction = friccao;
    rbInfo.m_linearDamping = amortecimentoLinear;
    rbInfo.m_angularDamping = amortecimentoAngular;

    rigidBody = new btRigidBody(rbInfo);
    
    // Adiciona um ponteiro para o objeto Porco no corpo rígido para fácil acesso durante colisões
    rigidBody->setUserPointer(this);

    // Habilita CCD (Continuous Collision Detection) para evitar tunneling em alta velocidade
    rigidBody->setCcdMotionThreshold(raioColisao * escala * 0.5f);
    rigidBody->setCcdSweptSphereRadius(raioColisao * escala * 0.2f);

    mundoFisica->addRigidBody(rigidBody);
    
    // Força a ativação do corpo para que a gravidade seja aplicada imediatamente
    rigidBody->activate(true);


    timerPulo = 0.0f;
    intervaloPulo = randomFloat(1.0f, 4.0f);

    tempoSemDano = 0.0f;

    g_audioManager.playPassaro(SomTipo::PORCO, 60);
}

void Porco::limparFisica() {
    if (mundoFisica && rigidBody) {
        mundoFisica->removeRigidBody(rigidBody);
    }
    delete rigidBody;
    rigidBody = nullptr;
    delete motionState;
    motionState = nullptr;
    delete colisaoShape;
    colisaoShape = nullptr;
}

bool Porco::carregarModelo(const char* caminhoOBJ) {
    return modelo.loadFromFile(caminhoOBJ);
}

bool Porco::carregarMTL(const char* caminhoMTL) {
    return modelo.loadMTL(caminhoMTL);
}

void Porco::desenhar() {
    if (!ativo || !rigidBody) return;

    btTransform transform;
    rigidBody->getMotionState()->getWorldTransform(transform);

    float matriz[16];
    transform.getOpenGLMatrix(matriz);

    glPushMatrix();
    glMultMatrixf(matriz);
    glScalef(escala, escala, escala);
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f); // Ajuste de rotação se necessário

    // Muda a cor com base na vida
    float r = 1.0f - (vida / vidaMaxima);
    float g = vida / vidaMaxima;
    glColor3f(r, g, 0.0f);

    if (!modelo.vertices.empty()) {
        modelo.draw();
    } else {
        glutSolidSphere(raioColisao, 20, 20);
    }

    glPopMatrix();
}

void Porco::atualizar(float deltaTime) {
    tempoSemDano += deltaTime;

    // Só processa o pulo se estiver sem levar dano há mais de 4 segundos
    if (tempoSemDano > 4.0f) {
        timerPulo += deltaTime;

        if (timerPulo >= intervaloPulo) {
            btVector3 vel = rigidBody->getLinearVelocity();
            
            // Só pula se estiver quase parado no eixo Y
            if (std::abs(vel.y()) < 0.2f) { 
                rigidBody->activate(true); 
                float forcaPulo = massa * 3.5f; 
                rigidBody->applyCentralImpulse(btVector3(0, forcaPulo, 0));
            }
            g_audioManager.playPassaro(SomTipo::PORCO_PULANDO, 60);

            timerPulo = 0.0f;
            intervaloPulo = randomFloat(2.0f, 5.0f);
        }
    } else {
        // Se levou dano recentemente, zera o timer do pulo para ele não pular imediatamente
        timerPulo = 0.0f;
    }
}

void Porco::tomarDano(float dano) {
    if (!ativo) return;

    tempoSemDano = 0.0f;

    g_audioManager.playPassaro(SomTipo::DANO_PORCO, 60);
    vida -= dano;
    std::cout << "Porco tomou " << dano << " de dano. Vida restante: " << vida << std::endl;

    if (vida <= 0) {
        std::cout << "Porco derrotado!" << std::endl;
        vida = 0;
        setAtivo(false);
        
        // Remove o corpo do mundo da física para que não interaja mais
        if (mundoFisica && rigidBody) {
            mundoFisica->removeRigidBody(rigidBody);
        }
    }
}

void Porco::resetar(float posX, float posY, float posZ) {
    if (!rigidBody) return;

    // Se o corpo foi removido, precisa ser adicionado de volta
    if (!rigidBody->isInWorld()) {
        mundoFisica->addRigidBody(rigidBody);
    }

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(posX, posY, posZ));

    rigidBody->setWorldTransform(transform);
    rigidBody->getMotionState()->setWorldTransform(transform);

    rigidBody->setLinearVelocity(btVector3(0, 0, 0));
    rigidBody->setAngularVelocity(btVector3(0, 0, 0));
    rigidBody->clearForces();
    rigidBody->setActivationState(ISLAND_SLEEPING);

    vida = vidaMaxima;
    setAtivo(true);
    
    timerPulo = 0.0f;
    intervaloPulo = randomFloat(1.0f, 3.0f);
    
    tempoSemDano = 0.0f; // <--- ADICIONE ISTO
}

btVector3 Porco::getPosicao() const {
    if (!rigidBody) return btVector3(0, 0, 0);
    btTransform transform;
    rigidBody->getMotionState()->getWorldTransform(transform);
    return transform.getOrigin();
}

btVector3 Porco::getVelocidade() const {
    if (!rigidBody) return btVector3(0, 0, 0);
    return rigidBody->getLinearVelocity();
}
