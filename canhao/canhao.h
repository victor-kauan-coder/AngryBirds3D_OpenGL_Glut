#ifndef CANHAO_H
#define CANHAO_H

#include "../porcos/porco.h"
#include <vector>

class Cannon : public Porco {
private:
    float timeSinceLastShot;
    float shootInterval;
    btVector3 targetPosition;
    std::vector<Porco*> projectiles;
    Porco* projectile;
    btDiscreteDynamicsWorld* worldRef;
    btVector3 lastVelocity;

public:
    Cannon(float posX, float posY, float posZ, btDiscreteDynamicsWorld* world, btVector3 targetPos);
    virtual ~Cannon();

    void atualizar(float deltaTime) override;
    void desenhar() override;
    void shoot();
    void setTarget(btVector3 target);
    Porco* getProjectile();
    
    // Override to use box shape
    void inicializarFisica(btDiscreteDynamicsWorld* mundo, float posX, float posY, float posZ);
};

#endif
