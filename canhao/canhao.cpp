#include "canhao.h"
#include "../blocos/ParticleManager.h"
#include "../controle_audio/audio_manager.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <iostream>

extern ParticleManager g_particleManager;

extern AudioManager g_audioManager;

Cannon::Cannon(float posX, float posY, float posZ, btDiscreteDynamicsWorld* world, btVector3 targetPos)
    : Porco(posX, posY, posZ, 0.5f, 1.4f, 500), 
      timeSinceLastShot(0.0f),
      shootInterval(6.0f), //
      targetPosition(targetPos),
      worldRef(world),
      lastVelocity(0,0,0),
      projectile(nullptr)
{
    this->vidaMaxima = 50.0f;
    this->vida = 50.0f;
    this->massa = 10.0f; 
    this->escala = 5.0f;
    
    if (!carregarModelo("Objetos/conhao.obj")) {
        std::cout << "Erro ao carregar modelo do canhao (Objetos/conhao.obj)" << std::endl;
    }
    if (!carregarMTL("Objetos/conhao.mtl")) {
        std::cout << "Erro ao carregar textura do canhao (Objetos/conhao.mtl)" << std::endl;
    }
    
    inicializarFisica(world, posX, posY, posZ);

    if (rigidBody) {
        btVector3 cannonPos(posX, posY, posZ);
        btVector3 dir = targetPos - cannonPos;
        
        btQuaternion rotation(btVector3(0, 1, 0), M_PI/2); 
        
        btTransform trans = rigidBody->getWorldTransform();
        trans.setRotation(rotation);
        
        rigidBody->setWorldTransform(trans);
        if (rigidBody->getMotionState()) {
            rigidBody->getMotionState()->setWorldTransform(trans);
        }
    }
}

Cannon::~Cannon() {
    for (auto p : projectiles) {
        delete p;
    }
    projectiles.clear();
}

void Cannon::inicializarFisica(btDiscreteDynamicsWorld* mundo, float posX, float posY, float posZ) {
    mundoFisica = mundo;

    colisaoShape = new btBoxShape(btVector3(0.9f, 0.9f, 0.9f)); 

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(posX, posY, posZ));

    motionState = new btDefaultMotionState(transform);

    btVector3 inerciaLocal(0, 0, 0);
    if (massa > 0.0f) {
        colisaoShape->calculateLocalInertia(massa, inerciaLocal);
    }

    btRigidBody::btRigidBodyConstructionInfo rbInfo(massa, motionState, colisaoShape, inerciaLocal);
    rbInfo.m_restitution = 0.1f;
    rbInfo.m_friction = 0.8f;  

    rigidBody = new btRigidBody(rbInfo);
    rigidBody->setUserPointer(this);

    mundoFisica->addRigidBody(rigidBody);
    rigidBody->activate(true);
    
    lastVelocity = btVector3(0,0,0);
}

void Cannon::setTarget(btVector3 target) {
    targetPosition = target;
}

Porco* Cannon::getProjectile() {
    return projectile;
}

void Cannon::shoot() {
    if (!ativo || !worldRef) return;

    btVector3 cannonPos = getPosicao();
    
    // Calculate direction to target
    btVector3 direction = targetPosition - cannonPos;
    direction.normalize();
    
    // Spawn projectile slightly in front and above to avoid immediate collision
    btVector3 spawnPos = cannonPos + direction * 2.0f + btVector3(0, 1.0f, 0);
    
    //cria um porco para arremessar(vale 0 pontos)
    projectile = new Porco(spawnPos.x(), spawnPos.y(), spawnPos.z(), 0.5f, 2.0f, 0);    
    
    //coloca o porco no mundo de física e da a ele 1 de vida
    projectile->inicializarFisica(worldRef, spawnPos.x(), spawnPos.y(), spawnPos.z());
    projectile->setVida(1.0f); 
    
    float force = 100.0f; 
    btVector3 impulse = direction * force + btVector3(0, 8.0f, 0); // 8y para criar um lançamento em arco
    projectile->getRigidBody()->applyCentralImpulse(impulse); //aplica o impulso para lançar o porco
    
    projectiles.push_back(projectile);
    // std::cout << "Canhao atirou!" << std::endl;

    g_audioManager.playPassaro(SomTipo::SOM_CANHAO, 45);
    // Efeito de explosão ao atirar
    g_particleManager.createFireExplosion(spawnPos, 20, 1.0f);
}

void Cannon::atualizar(float deltaTime) {
    if (!ativo) return;

    //Check if standing
    if (rigidBody) {
        btTransform trans;
        rigidBody->getMotionState()->getWorldTransform(trans);
        btVector3 up = trans.getBasis() * btVector3(0, 1, 0); // Local up in world space
        
        // If the local Y axis is not pointing roughly up (dot product < 0.5 means > 60 degrees tilt)
        if (up.dot(btVector3(0, 1, 0)) < 0.5f) {
            std::cout << "Canhao tombou!" << std::endl;
            tomarDano(1000); // Destroy
            return;
        }
        
        
        btVector3 currentVel = rigidBody->getLinearVelocity();
        btVector3 deltaV = currentVel - lastVelocity;
        if (deltaV.length() > 10.0f) { 
             std::cout << "Canhao sofreu impacto forte!" << std::endl;
             tomarDano(20.0f); 
        }
        lastVelocity = currentVel;
    }

    
    timeSinceLastShot += deltaTime;
    if (timeSinceLastShot >= shootInterval) {
        shoot();
        timeSinceLastShot = 0.0f;
    }

    //atualiza porco
    for (auto it = projectiles.begin(); it != projectiles.end(); ) {
        (*it)->atualizar(deltaTime);
        
        if (!(*it)->isAtivo()) {
            delete *it;
            it = projectiles.erase(it);
        } else {
            ++it;
        }
    }
}

void Cannon::desenhar() {
    if (!ativo) return;
    
    if (rigidBody) {
        btTransform transform;
        rigidBody->getMotionState()->getWorldTransform(transform);

        float matriz[16];
        transform.getOpenGLMatrix(matriz);

        glPushMatrix();
        glMultMatrixf(matriz);
        glScalef(escala, escala, escala);
        
        glColor3f(0.4f, 0.4f, 0.4f); 

        if (!modelo.vertices.empty()) {
            modelo.draw();
        } else {

            glutSolidCube(1.0); 
        }

        glPopMatrix();
    }
    
    for (auto p : projectiles) {
        p->desenhar();
    }
}
