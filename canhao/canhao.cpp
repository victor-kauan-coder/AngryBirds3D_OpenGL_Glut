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
    : Porco(posX, posY, posZ, 0.5f, 1.4f), // Scale 3.0f similar to pigs
      timeSinceLastShot(0.0f),
      shootInterval(15.0f), // Intervalo reduzido para 3 segundos para teste
      targetPosition(targetPos),
      worldRef(world),
      lastVelocity(0,0,0),
      projectile(nullptr)
{
    this->vidaMaxima = 50.0f;
    this->vida = 50.0f;
    this->massa = 10.0f; // Heavier than a pig
    this->escala = 5.0f;
    // Load cannon model
    // Note: The path is relative to the executable or working directory.
    // Assuming "Objetos/" is in the working directory.
    if (!carregarModelo("Objetos/conhao.obj")) {
        std::cout << "Erro ao carregar modelo do canhao (Objetos/conhao.obj)" << std::endl;
    }
    if (!carregarMTL("Objetos/conhao.mtl")) {
        std::cout << "Erro ao carregar textura do canhao (Objetos/conhao.mtl)" << std::endl;
    }
    
    // Initialize physics with a box shape
    inicializarFisica(world, posX, posY, posZ);

    // --- Rotação para mirar no estilingue ---
    if (rigidBody) {
        btVector3 cannonPos(posX, posY, posZ);
        btVector3 dir = targetPos - cannonPos;
        
        // Calcula o ângulo Y (yaw) para apontar para o alvo
        float yaw = atan2(dir.x(), dir.z());
        
        // Ajuste de rotação:
        // Adicionamos M_PI/2 (90 graus) para rotacionar o modelo (que aponta para -Z ou +Z)
        // para a direção correta.
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

    // Use a BoxShape for the cannon. 
    // Assuming the normalized model fits in a box.
    // Scale is 3.0, model is ~0.6. So size is ~1.8.
    // Half extents should be around 0.9.
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
    rbInfo.m_restitution = 0.1f; // Low restitution
    rbInfo.m_friction = 0.8f;    // High friction

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
    
    // Create projectile (Porco)
    // Use a smaller scale for the projectile
    projectile = new Porco(spawnPos.x(), spawnPos.y(), spawnPos.z(), 0.5f, 2.0f);    
    // Load pig model for the projectile
    
    projectile->inicializarFisica(worldRef, spawnPos.x(), spawnPos.y(), spawnPos.z());
    projectile->setVida(1.0f); // 1 HP as requested
    
    // Apply impulse
    float force = 100.0f; // Adjust power as needed
    // Add some upward arc
    btVector3 impulse = direction * force + btVector3(0, 8.0f, 0);
    projectile->getRigidBody()->applyCentralImpulse(impulse);
    
    projectiles.push_back(projectile);
    std::cout << "Canhao atirou!" << std::endl;

    g_audioManager.playPassaro(SomTipo::SOM_CANHAO, 100);
    // Efeito de explosão ao atirar
    g_particleManager.createFireExplosion(spawnPos, 20, 1.0f);
}

void Cannon::atualizar(float deltaTime) {
    if (!ativo) return;

    // 1. Check if standing
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
        
        // 2. Check for high impact (sudden velocity change)
        btVector3 currentVel = rigidBody->getLinearVelocity();
        btVector3 deltaV = currentVel - lastVelocity;
        if (deltaV.length() > 10.0f) { // Threshold for "certain power"
             std::cout << "Canhao sofreu impacto forte!" << std::endl;
             tomarDano(20.0f); // Take damage
        }
        lastVelocity = currentVel;
    }

    // 3. Shooting timer
    timeSinceLastShot += deltaTime;
    if (timeSinceLastShot >= shootInterval) {
        shoot();
        timeSinceLastShot = 0.0f;
    }

    // 4. Update projectiles
    for (auto it = projectiles.begin(); it != projectiles.end(); ) {
        (*it)->atualizar(deltaTime);
        
        // Remove inactive projectiles
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
    
    // --- Desenho do Canhão (Override de Porco::desenhar) ---
    if (rigidBody) {
        btTransform transform;
        rigidBody->getMotionState()->getWorldTransform(transform);

        float matriz[16];
        transform.getOpenGLMatrix(matriz);

        glPushMatrix();
        glMultMatrixf(matriz);
        glScalef(escala, escala, escala);
        
        // REMOVIDO: glRotatef(180.0f, 0.0f, 1.0f, 0.0f); 
        // A rotação agora é controlada puramente pela física (quaternion no construtor)

        // Cor de fallback (caso a textura falhe)
        // Cinza escuro metálico em vez de verde/vermelho de vida
        glColor3f(0.4f, 0.4f, 0.4f); 

        if (!modelo.vertices.empty()) {
            modelo.draw();
        } else {
            // Fallback visual se o modelo não carregar
            glutSolidCube(1.0); 
        }

        glPopMatrix();
    }
    
    // Draw projectiles
    for (auto p : projectiles) {
        p->desenhar();
    }
}
