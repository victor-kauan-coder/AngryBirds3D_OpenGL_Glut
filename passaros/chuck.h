#ifndef CHUCK_H
#define CHUCK_H

#include "passaro.h"
#include "../blocos/ParticleManager.h"
extern ParticleManager g_particleManager;
/**
 * Exemplo de classe derivada: Chuck (Pássaro amarelo - rápido)
 * Este é o pássaro rápido do Angry Birds
 */
class PassaroChuck : public Passaro {
public:
    PassaroChuck(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f);
    
    void usarHabilidade() override;
};
#endif // CHUCK_H