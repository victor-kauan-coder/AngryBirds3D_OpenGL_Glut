#include "canhao.h"
#include <iostream>

Cannon::Cannon(float posX, float posY, float posZ, btDiscreteDynamicsWorld* world, btVector3 targetPos)
    : Porco(posX, posY, posZ, 0.5f, 3.0f), // Scale 3.0f similar to pigs
      timeSinceLastShot(0.0f),
      shootInterval(15.0f),
      targetPosition(targetPos),
      worldRef(world),
      lastVelocity(0,0,0)
{
    this->vidaMaxima = 50.0f;
    this->vida = 50.0f;
    this->massa = 10.0f; // Heavier than a pig
    
    // Load cannon model
    // Note: The path is relative to the executable or working directory.
    // Assuming "Objetos/" is in the working directory.
    if (!carregarModelo("Objetos/conhao.obj")) {
        std::cout << "Erro ao carregar modelo do canhao (Objetos/conhao.obj)" << std::endl;
    }
    
    // Initialize physics with a box shape
    inicializarFisica(world, posX, posY, posZ);
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
    Porco* projectile = new Porco(spawnPos.x(), spawnPos.y(), spawnPos.z(), 0.2f, 1.5f);
    
    // Load pig model for the projectile
    if (!projectile->carregarModelo("Objetos/porco.obj")) {
         std::cout << "Erro ao carregar projetil" << std::endl;
    }
    
    projectile->inicializarFisica(worldRef, spawnPos.x(), spawnPos.y(), spawnPos.z());
    projectile->setVida(1.0f); // 1 HP as requested
    
    // Apply impulse
    float force = 30.0f; // Adjust power as needed
    // Add some upward arc
    btVector3 impulse = direction * force + btVector3(0, 5.0f, 0);
    projectile->getRigidBody()->applyCentralImpulse(impulse);
    
    projectiles.push_back(projectile);
    std::cout << "Canhao atirou!" << std::endl;
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
    
    // Draw the cannon itself
    Porco::desenhar(); 
    
    // Draw projectiles
    for (auto p : projectiles) {
        p->desenhar();
    }
}
