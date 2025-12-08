#ifndef PASSARO_H
#define PASSARO_H

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <string>
#include <btBulletDynamicsCommon.h>
#include "../util/loads.h"
#include "../controle_audio/audio_manager.h"
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern AudioManager g_audioManager;

class Passaro {
protected:
    OBJModel modelo;
    btRigidBody* rigidBody;
    btCollisionShape* colisaoShape;
    btDefaultMotionState* motionState;
    btDiscreteDynamicsWorld* mundoFisica;
    
    float escala;
    float raioColisao;
    bool ativo;
    bool emVoo;
    float corR, corG, corB;
    float massa;
    std::string tipo;
    float restituicao;
    float friccao;
    float amortecimentoLinear;
    float amortecimentoAngular;
    float rotacaoVisualX, rotacaoVisualY, rotacaoVisualZ;
    float anguloVisual;
    float tempoVida;
    float tempoVoo;
    float tempoVidaMaximo;
    float tempoVooMaximo;

    // --- VARIÁVEIS DA FILA ---
    bool naFila;
    btVector3 posFila;          
    float timerPuloFila;        
    float intervaloPuloFila;    
    bool pulandoFila;           
    float progressoPulo;        

    // --- NOVO: Variáveis para "Andar na Fila" ---
    bool andandoNaFila;
    btVector3 posOrigemAndar;
    btVector3 posDestinoAndar;
    float progressoAndar;

public:
    Passaro(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f, 
            float raio = 0.5f, float escalaInicial = 1.0f)
        : rigidBody(nullptr), colisaoShape(nullptr), motionState(nullptr), mundoFisica(nullptr),
          escala(escalaInicial), raioColisao(raio), ativo(true), emVoo(false),
          corR(1.0f), corG(0.0f), corB(0.0f), massa(1.0f), tipo("Passaro Base"),
          rotacaoVisualX(0.0f), rotacaoVisualY(1.0f), rotacaoVisualZ(0.0f), anguloVisual(M_PI),
          restituicao(0.6f), friccao(0.5f), amortecimentoLinear(0.1f), amortecimentoAngular(0.1f),
          tempoVida(0.0f), tempoVoo(0.0f), tempoVidaMaximo(1.0f), tempoVooMaximo(5.0f),
          
          naFila(false), posFila(0,0,0), timerPuloFila(0.0f), 
          intervaloPuloFila(0.0f), pulandoFila(false), progressoPulo(0.0f),
          
          // Inicializa variáveis novas
          andandoNaFila(false), posOrigemAndar(0,0,0), posDestinoAndar(0,0,0), progressoAndar(0.0f)
          {
              intervaloPuloFila = 1.0f + (static_cast<float>(rand()) / RAND_MAX) * 3.0f; 
    }

    virtual ~Passaro() { limparFisica(); }
    
    void setNaFila(bool status, btVector3 pos) {
        naFila = status;
        posFila = pos;
        if (status) limparFisica();
    }

    btVector3 getPosFila() const { return posFila; }
    bool isNaFila() const { return naFila; }

    // --- NOVO: Função para iniciar o movimento na fila ---
    void caminharNaFilaPara(btVector3 destino) {
        if (!naFila) return;
        posOrigemAndar = posFila;
        posDestinoAndar = destino;
        andandoNaFila = true;
        progressoAndar = 0.0f;
        // Reseta o pulo de tédio para não conflitar
        pulandoFila = false; 
        progressoPulo = 0.0f;
    }

    // Animação unificada da fila
    void atualizarFila(float deltaTime) {
        if (!naFila) return;

        // 1. Prioridade: Andar para a frente (quando a fila anda)
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
            return; // Se está andando, não faz o pulo de tédio
        }

        // 2. Animação de Tédio (Pulo no lugar)
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

    void desenharNaFila() {
        if (!naFila) return;

        float yOffset = 0.0f;
        // Se estiver pulando de tédio (no lugar)
        if (pulandoFila && !andandoNaFila) {
            yOffset = sin(progressoPulo * M_PI) * 1.5f;
        }
        // Nota: Se estiver 'andandoNaFila', o yOffset já está embutido em 'posFila' pelo atualizarFila

        desenharEmPosicao(posFila.x(), posFila.y() + yOffset, posFila.z());
    }

    // ... (RESTANTE DA CLASSE IGUAL: inicializarFisica, limparFisica, carregarModelo, etc...) ...
    // COPIE O RESTANTE DO ARQUIVO ANTERIOR AQUI, NADA MUDOU DAQUI PARA BAIXO
    void inicializarFisica(btDiscreteDynamicsWorld* mundo, float posX, float posY, float posZ) {
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
    
    void limparFisica() {
        if (mundoFisica && rigidBody) mundoFisica->removeRigidBody(rigidBody);
        if (rigidBody) { delete rigidBody; rigidBody = nullptr; }
        if (motionState) { delete motionState; motionState = nullptr; }
        if (colisaoShape) { delete colisaoShape; colisaoShape = nullptr; }
    }
    
    bool carregarModelo(const char* caminhoOBJ) { return modelo.loadFromFile(caminhoOBJ); }
    bool carregarMTL(const char* caminhoMTL) { return modelo.loadMTL(caminhoMTL); }
    
    virtual void desenhar() {
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
    
    virtual void desenharEmPosicao(float x, float y, float z) {
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

    void desenharEsferaColisao() {
        if (!rigidBody) return;
        btTransform transform; rigidBody->getMotionState()->getWorldTransform(transform);
        btVector3 pos = transform.getOrigin();
        glPushMatrix(); glTranslatef(pos.x(), pos.y(), pos.z());
        glColor3f(0.0f, 1.0f, 0.0f); glDisable(GL_LIGHTING);
        glutWireSphere(raioColisao * escala, 12, 12); glEnable(GL_LIGHTING);
        glPopMatrix();
    }
    
    virtual void atualizar(float deltaTime) {
        if (rigidBody) {
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
    
    void lancar(float velX, float velY, float velZ) {
        if (!rigidBody) return;
        rigidBody->activate(true); rigidBody->setActivationState(ACTIVE_TAG);
        rigidBody->setLinearVelocity(btVector3(velX, velY, velZ));
        rigidBody->setAngularVelocity(btVector3(0, 0, velX * 0.5f));
        emVoo = true;
        // g_audioManager.playPassaro(SomTipo::LANCAMENTO_PASSARO);
    }
    
    void aplicarImpulso(float impulsoX, float impulsoY, float impulsoZ) {
        if (!rigidBody) return;
        rigidBody->activate(true);
        rigidBody->applyCentralImpulse(btVector3(impulsoX, impulsoY, impulsoZ));
    }
    
    void aplicarForca(float forcaX, float forcaY, float forcaZ) {
        if (!rigidBody) return;
        rigidBody->activate(true);
        rigidBody->applyCentralForce(btVector3(forcaX, forcaY, forcaZ));
    }
    
    bool verificarColisao() {
        if (!rigidBody || !mundoFisica) return false;
        int numManifolds = mundoFisica->getDispatcher()->getNumManifolds();
        for (int i = 0; i < numManifolds; i++) {
            btPersistentManifold* contactManifold = mundoFisica->getDispatcher()->getManifoldByIndexInternal(i);
            const btCollisionObject* obA = contactManifold->getBody0();
            const btCollisionObject* obB = contactManifold->getBody1();
            if (obA == rigidBody || obB == rigidBody) {
                if (contactManifold->getNumContacts() > 0) return true;
            }
        }
        return false;
    }
    
    bool verificarColisaoCom(const Passaro& outro) {
        if (!rigidBody || !outro.rigidBody || !mundoFisica) return false;
        int numManifolds = mundoFisica->getDispatcher()->getNumManifolds();
        for (int i = 0; i < numManifolds; i++) {
            btPersistentManifold* contactManifold = mundoFisica->getDispatcher()->getManifoldByIndexInternal(i);
            const btCollisionObject* obA = contactManifold->getBody0();
            const btCollisionObject* obB = contactManifold->getBody1();
            bool temEstePassaro = (obA == rigidBody || obB == rigidBody);
            bool temOutroPassaro = (obA == outro.rigidBody || obB == outro.rigidBody);
            if (temEstePassaro && temOutroPassaro) {
                if (contactManifold->getNumContacts() > 0) return true;
            }
        }
        return false;
    }
    
    virtual void usarHabilidade() {}
    
    virtual void resetar(float posX, float posY, float posZ) {
        if (!rigidBody) return;
        btTransform transform; transform.setIdentity(); transform.setOrigin(btVector3(posX, posY, posZ));
        rigidBody->setWorldTransform(transform); rigidBody->getMotionState()->setWorldTransform(transform);
        rigidBody->setLinearVelocity(btVector3(0, 0, 0)); rigidBody->setAngularVelocity(btVector3(0, 0, 0));
        rigidBody->clearForces(); rigidBody->setActivationState(ISLAND_SLEEPING);
        emVoo = false; ativo = true; naFila = false; 
    }
    
    btVector3 getPosicao() const {
        if (naFila) return posFila; 
        if (!rigidBody) return btVector3(0, 0, 0);
        btTransform transform; rigidBody->getMotionState()->getWorldTransform(transform);
        return transform.getOrigin();
    }
    
    float getX() const { return getPosicao().x(); }
    float getY() const { return getPosicao().y(); }
    float getZ() const { return getPosicao().z(); }
    
    void setPosicao(float posX, float posY, float posZ) {
        if (naFila) { posFila = btVector3(posX, posY, posZ); return; }
        if (!rigidBody) return;
        btTransform transform; rigidBody->getMotionState()->getWorldTransform(transform);
        transform.setOrigin(btVector3(posX, posY, posZ));
        rigidBody->setWorldTransform(transform); rigidBody->getMotionState()->setWorldTransform(transform);
    }
    
    btVector3 getVelocidade() const { if (!rigidBody) return btVector3(0, 0, 0); return rigidBody->getLinearVelocity(); }
    float getVelocidadeX() const { return getVelocidade().x(); }
    float getVelocidadeY() const { return getVelocidade().y(); }
    float getVelocidadeZ() const { return getVelocidade().z(); }
    void setVelocidade(float velX, float velY, float velZ) { if (!rigidBody) return; rigidBody->setLinearVelocity(btVector3(velX, velY, velZ)); }
    btQuaternion getRotacao() const { if (!rigidBody) return btQuaternion(0, 0, 0, 1); btTransform transform; rigidBody->getMotionState()->getWorldTransform(transform); return transform.getRotation(); }
    void setRotacao(float eixoX, float eixoY, float eixoZ, float anguloRad) { if (!rigidBody) return; btTransform transform; rigidBody->getMotionState()->getWorldTransform(transform); btQuaternion quat; quat.setRotation(btVector3(eixoX, eixoY, eixoZ), anguloRad); transform.setRotation(quat); rigidBody->setWorldTransform(transform); rigidBody->getMotionState()->setWorldTransform(transform); }
    void setRotacaoVisual(float eixoX, float eixoY, float eixoZ, float anguloRad) { rotacaoVisualX = eixoX; rotacaoVisualY = eixoY; rotacaoVisualZ = eixoZ; anguloVisual = anguloRad; }
    float getEscala() const { return escala; }
    void setEscala(float valor) { escala = valor; }
    float getRaioColisao() const { return raioColisao; }
    void setRaioColisao(float raio) { raioColisao = raio; }
    bool isAtivo() const { return ativo; }
    void setAtivo(bool valor) { ativo = valor; }
    bool isEmVoo() const { return emVoo; }
    void setEmVoo(bool valor) { emVoo = valor; }
    void setCor(float r, float g, float b) { corR = r; corG = g; corB = b; }
    float getMassa() const { return massa; }
    void setMassa(float valor) { massa = valor; if (rigidBody) { btVector3 inerciaLocal(0, 0, 0); if (massa > 0.0f && colisaoShape) { colisaoShape->calculateLocalInertia(massa, inerciaLocal); } rigidBody->setMassProps(massa, inerciaLocal); } }
    std::string getTipo() const { return tipo; }
    void setTipo(const std::string& novoTipo) { tipo = novoTipo; }
    OBJModel& getModelo() { return modelo; }
    btRigidBody* getRigidBody() { return rigidBody; }
    const btRigidBody* getRigidBody() const { return rigidBody; }
    void setRigidBodyInfo(const btRigidBody::btRigidBodyConstructionInfo& rbInfo) { if (rigidBody) { if (mundoFisica) { mundoFisica->removeRigidBody(rigidBody); } delete rigidBody; } rigidBody = new btRigidBody(rbInfo); if (mundoFisica) { mundoFisica->addRigidBody(rigidBody); } }
    float getRestituicao() const { return restituicao; }
    void setRestituicao(float valor) { restituicao = valor; if (rigidBody) rigidBody->setRestitution(valor); }
    btDefaultMotionState* getMotionState() { return motionState; }
    const btDefaultMotionState* getMotionState() const { return motionState; }    
    void setMotionState(btDefaultMotionState* ms) { motionState = ms; if (rigidBody) rigidBody->setMotionState(ms); }
    btCollisionShape* getColisaoShape() { return colisaoShape; }
    const btCollisionShape* getColisaoShape() const { return colisaoShape; }
    void setColisaoShape(btCollisionShape* shape) { colisaoShape = shape; if (rigidBody) rigidBody->setCollisionShape(shape); }
    float getFriccao() const { return friccao; }
    void setFriccao(float valor) { friccao = valor; if (rigidBody) rigidBody->setFriction(valor); }
    void setAmortecimento(float linear, float angular) { amortecimentoLinear = linear; amortecimentoAngular = angular; if (rigidBody) rigidBody->setDamping(linear, angular); }
};

#endif // PASSARO_H