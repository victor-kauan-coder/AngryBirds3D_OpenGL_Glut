#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include "../util/loads.h"
#include "../util/openGL_util.h"
#include "porco.h"
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Porco::Porco(float posX, float posY, float posZ, float raio, float escalaInicial)
    : rigidBody(nullptr),
      colisaoShape(nullptr),
      motionState(nullptr),
      mundoFisica(nullptr),
      escala(escalaInicial),
      raioColisao(raio),
      ativo(true),
      vidaMaxima(100.0f),
      vida(10.0f),
      massa(2.0f),
      tipo("Porco"),
      restituicao(0.5f),
      friccao(0.8f),
      amortecimentoLinear(0.2f),
      amortecimentoAngular(0.2f),
      posicaoInicial(0,0,0),
      saiuDaPosicao(false),
      tempoDesdeSaida(0.0f),
      tempoParaSumir(3.0f) { // 3 segundos para sumir após sair da posição
    
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
    
    // Define a posição inicial para controle de desaparecimento
    posicaoInicial = btVector3(posX, posY, posZ);
    saiuDaPosicao = false;
    tempoDesdeSaida = 0.0f;
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
    if (!ativo) return;
    
    // Verifica se saiu da posição inicial
    if (!saiuDaPosicao && rigidBody) {
        btVector3 posAtual = getPosicao();
        float dist = posAtual.distance(posicaoInicial);
        // Se moveu mais que 1.0 unidade (ajuste conforme necessário)
        if (dist > 1.0f) { 
            saiuDaPosicao = true;
            // std::cout << "Porco saiu da posicao inicial!" << std::endl;
        }
    }

    // Se já saiu da posição, conta o tempo para sumir
    if (saiuDaPosicao) {
        tempoDesdeSaida += deltaTime;
        if (tempoDesdeSaida >= tempoParaSumir) {
            std::cout << "Porco desapareceu apos sair da posicao!" << std::endl;
            tomarDano(vidaMaxima + 1000.0f); // Mata instantaneamente
        }
    }
}

void Porco::tomarDano(float dano) {
    if (!ativo) return;

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
    
    // Reseta lógica de desaparecimento
    posicaoInicial = btVector3(posX, posY, posZ);
    saiuDaPosicao = false;
    tempoDesdeSaida = 0.0f;
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
