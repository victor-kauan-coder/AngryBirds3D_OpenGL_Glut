#include "passaro.h"
#include <iostream>

Passaro::Passaro(float posX, float posY, float posZ, float raio, float escalaInicial)
    : rigidBody(nullptr), colisaoShape(nullptr), motionState(nullptr), mundoFisica(nullptr),
      escala(escalaInicial), raioColisao(raio), ativo(true), emVoo(false),
      corR(1.0f), corG(0.0f), corB(0.0f), massa(1.0f), tipo("Passaro Base"),
      rotacaoVisualX(0.0f), rotacaoVisualY(1.0f), rotacaoVisualZ(0.0f), anguloVisual(M_PI),
      restituicao(0.6f), friccao(0.5f), amortecimentoLinear(0.1f), amortecimentoAngular(0.1f),
      tempoVida(0.0f), tempoVoo(0.0f), tempoVidaMaximo(1.0f), tempoVooMaximo(5.0f),
      ultimaPosicao(posX, posY, posZ), // Inicializa com a posição inicial
      
      naFila(false), posFila(0,0,0), timerPuloFila(0.0f), 
      intervaloPuloFila(0.0f), pulandoFila(false), progressoPulo(0.0f),
      
      // Inicializa variáveis novas
      andandoNaFila(false), posOrigemAndar(0,0,0), posDestinoAndar(0,0,0), progressoAndar(0.0f)
      {
          intervaloPuloFila = 1.0f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f; 
}

void Passaro::setNaFila(bool status, btVector3 pos) {
    naFila = status;
    posFila = pos;
    if (status) limparFisica();
}

void Passaro::caminharNaFilaPara(btVector3 destino) {
    if (!naFila) return;
    posOrigemAndar = posFila;
    posDestinoAndar = destino;
    andandoNaFila = true;
    progressoAndar = 0.0f;
    
    // Reseta o pulo de tédio para não conflitar
    pulandoFila = false; 
    progressoPulo = 0.0f;
}

void Passaro::atualizarFila(float deltaTime) {
    if (!naFila) return;

    //Prioridade: anda ou pula
    if (andandoNaFila) {
        progressoAndar += deltaTime * 2.5f; // Velocidade do andar
        
        if (progressoAndar >= 1.0f) {
            // Chegou
            andandoNaFila = false;
            posFila = posDestinoAndar;
            progressoAndar = 0.0f;
        } else {
            // Interpolação Linear + Arco
            btVector3 currentPos = posOrigemAndar.lerp(posDestinoAndar, progressoAndar);
            float yOffset = sin(progressoAndar * M_PI) * 1.0f; // Pulo de 1 unidade de altura
            currentPos.setY(currentPos.y() + yOffset);
            posFila = currentPos;
        }
        return; 
    }

    //Animação de Tédio (Pulo no lugar)
    if (pulandoFila) {
        progressoPulo += deltaTime * 2.0f;
        if (progressoPulo >= 1.0f) {
            pulandoFila = false;
            progressoPulo = 0.0f;
        }
    } else {
        timerPuloFila += deltaTime;
        if (timerPuloFila >= intervaloPuloFila) {
            pulandoFila = true;
            timerPuloFila = 0.0f;
            intervaloPuloFila = 2.0f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f;
        }
    }
}

void Passaro::desenharNaFila() {
    if (!naFila) return;

    float yOffset = 0.0f;
    // Se estiver pulando de tédio (no lugar)
    if (pulandoFila && !andandoNaFila) {
        yOffset = sin(progressoPulo * M_PI) * 1.5f;
    }


    desenharEmPosicao(posFila.x(), posFila.y() + yOffset, posFila.z());
}

void Passaro::inicializarFisica(btDiscreteDynamicsWorld* mundo, float posX, float posY, float posZ) {
    mundoFisica = mundo;
    colisaoShape = new btSphereShape(raioColisao * escala);
    btTransform transform; transform.setIdentity(); transform.setOrigin(btVector3(posX, posY, posZ));
    motionState = new btDefaultMotionState(transform);
    btVector3 inerciaLocal(0, 0, 0);
    if (massa > 0.0f) colisaoShape->calculateLocalInertia(massa, inerciaLocal);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(massa, motionState, colisaoShape, inerciaLocal);
    rbInfo.m_restitution = restituicao; rbInfo.m_friction = friccao;        
    rbInfo.m_linearDamping = amortecimentoLinear; rbInfo.m_angularDamping = amortecimentoAngular;
    rigidBody = new btRigidBody(rbInfo);
    rigidBody->setCcdMotionThreshold(0.0001f); rigidBody->setCcdSweptSphereRadius(raioColisao * escala);
    rigidBody->setContactProcessingThreshold(0.0f);
    mundoFisica->addRigidBody(rigidBody);
    rigidBody->setActivationState(ISLAND_SLEEPING);
}

void Passaro::limparFisica() {
    if (mundoFisica && rigidBody) mundoFisica->removeRigidBody(rigidBody);
    if (rigidBody) { delete rigidBody; rigidBody = nullptr; }
    if (motionState) { delete motionState; motionState = nullptr; }
    if (colisaoShape) { delete colisaoShape; colisaoShape = nullptr; }
}

bool Passaro::carregarModelo(const char* caminhoOBJ) { return modelo.loadFromFile(caminhoOBJ); }
bool Passaro::carregarMTL(const char* caminhoMTL) { return modelo.loadMTL(caminhoMTL); }

void Passaro::desenhar() {
    if (!ativo || !rigidBody) return;
    btTransform transform; rigidBody->getMotionState()->getWorldTransform(transform);
    float matriz[16]; transform.getOpenGLMatrix(matriz);
    glPushMatrix();
    glMultMatrixf(matriz);
    float anguloDeg = anguloVisual * 180.0f / M_PI;
    glRotatef(anguloDeg, rotacaoVisualX, rotacaoVisualY, rotacaoVisualZ);
    glScalef(escala, escala, escala);
    glColor3f(corR, corG, corB);
    if (!modelo.vertices.empty()) modelo.draw();
    else glutSolidSphere(raioColisao, 20, 20);
    glPopMatrix();
    #ifdef DEBUG_COLISAO
    desenharEsferaColisao();
    #endif
}

void Passaro::desenharEmPosicao(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    float anguloDeg = anguloVisual * 180.0f / M_PI;
    glRotatef(anguloDeg, rotacaoVisualX, rotacaoVisualY, rotacaoVisualZ);
    glScalef(escala, escala, escala);
    glColor3f(corR, corG, corB);
    if (!modelo.vertices.empty()) modelo.draw();
    else glutSolidSphere(raioColisao, 20, 20);
    glPopMatrix();
}

void Passaro::atualizar(float deltaTime) {
    if (rigidBody) {
        // Atualiza a última posição conhecida
        btTransform transform; 
        rigidBody->getMotionState()->getWorldTransform(transform);
        ultimaPosicao = transform.getOrigin();

        btVector3 vel = rigidBody->getLinearVelocity();
        float velocidadeTotal = vel.length();
        if (emVoo) {
            tempoVoo += deltaTime;
            if (tempoVoo >= tempoVooMaximo) { ativo = false; emVoo = false; limparFisica(); }
        }
        if (emVoo && velocidadeTotal < 0.1f) {
            tempoVida += deltaTime;
            if (tempoVida >= tempoVidaMaximo) {
                ativo = false; emVoo = false; limparFisica();
                g_audioManager.playPassaro(SomTipo::MORTE_PASSARO, 65);
            }
        }
    }
}

void Passaro::resetar(float posX, float posY, float posZ) {
    if (!rigidBody) return;
    btTransform transform; transform.setIdentity(); transform.setOrigin(btVector3(posX, posY, posZ));
    rigidBody->setWorldTransform(transform); rigidBody->getMotionState()->setWorldTransform(transform);
    rigidBody->setLinearVelocity(btVector3(0, 0, 0)); rigidBody->setAngularVelocity(btVector3(0, 0, 0));
    rigidBody->clearForces(); rigidBody->setActivationState(ISLAND_SLEEPING);
    emVoo = false; ativo = true; naFila = false; 
}

btVector3 Passaro::getPosicao() const {
    if (naFila) return posFila; 
    if (!rigidBody) return ultimaPosicao; // Retorna a última posição conhecida se não houver corpo
    btTransform transform; rigidBody->getMotionState()->getWorldTransform(transform);
    return transform.getOrigin();
}

void Passaro::setPosicao(float posX, float posY, float posZ) {
    if (naFila) { posFila = btVector3(posX, posY, posZ); return; }
    if (!rigidBody) return;
    btTransform transform; rigidBody->getMotionState()->getWorldTransform(transform);
    transform.setOrigin(btVector3(posX, posY, posZ));
    rigidBody->setWorldTransform(transform); rigidBody->getMotionState()->setWorldTransform(transform);
}

btVector3 Passaro::getVelocidade() const { if (!rigidBody) return btVector3(0, 0, 0); return rigidBody->getLinearVelocity(); }
void Passaro::setVelocidade(float velX, float velY, float velZ) { if (!rigidBody) return; rigidBody->setLinearVelocity(btVector3(velX, velY, velZ)); }
btQuaternion Passaro::getRotacao() const { if (!rigidBody) return btQuaternion(0, 0, 0, 1); btTransform transform; rigidBody->getMotionState()->getWorldTransform(transform); return transform.getRotation(); }
void Passaro::setRotacao(float eixoX, float eixoY, float eixoZ, float anguloRad) { if (!rigidBody) return; btTransform transform; rigidBody->getMotionState()->getWorldTransform(transform); btQuaternion quat; quat.setRotation(btVector3(eixoX, eixoY, eixoZ), anguloRad); transform.setRotation(quat); rigidBody->setWorldTransform(transform); rigidBody->getMotionState()->setWorldTransform(transform); }
void Passaro::setMassa(float valor) { massa = valor; if (rigidBody) { btVector3 inerciaLocal(0, 0, 0); if (massa > 0.0f && colisaoShape) { colisaoShape->calculateLocalInertia(massa, inerciaLocal); } rigidBody->setMassProps(massa, inerciaLocal); } }
void Passaro::setRigidBodyInfo(const btRigidBody::btRigidBodyConstructionInfo& rbInfo) { if (rigidBody) { if (mundoFisica) { mundoFisica->removeRigidBody(rigidBody); } delete rigidBody; } rigidBody = new btRigidBody(rbInfo); if (mundoFisica) { mundoFisica->addRigidBody(rigidBody); } }
void Passaro::setRestituicao(float valor) { restituicao = valor; if (rigidBody) rigidBody->setRestitution(valor); }
void Passaro::setMotionState(btDefaultMotionState* ms) { motionState = ms; if (rigidBody) rigidBody->setMotionState(ms); }
void Passaro::setColisaoShape(btCollisionShape* shape) { colisaoShape = shape; if (rigidBody) rigidBody->setCollisionShape(shape); }
void Passaro::setFriccao(float valor) { friccao = valor; if (rigidBody) rigidBody->setFriction(valor); }
void Passaro::setAmortecimento(float linear, float angular) { amortecimentoLinear = linear; amortecimentoAngular = angular; if (rigidBody) rigidBody->setDamping(linear, angular); }

void Passaro::setRotacaoVisual(float eixoX, float eixoY, float eixoZ, float anguloRad) { 
    rotacaoVisualX = eixoX; 
    rotacaoVisualY = eixoY; 
    rotacaoVisualZ = eixoZ; 
    anguloVisual = anguloRad; 
}

Passaro::~Passaro() { 
    limparFisica(); 
}
