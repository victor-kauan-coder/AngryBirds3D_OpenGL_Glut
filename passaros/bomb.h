#ifndef BOMB_H
#define BOMB_H

#include "passaro.h"
#include "../blocos/ParticleManager.h"
#include "../controle_audio/audio_manager.h"
#include <vector>

extern ParticleManager g_particleManager;

extern AudioManager g_audioManager;

/**
 * Classe derivada: Bomb (Bomba)
 * Este é o pássaro bomba do Angry Birds
 */
class PassaroBomb : public Passaro {
public:
    // Parâmetros da explosão (fáceis de manipular)
    float raioExplosao = 10.0f;
    float forcaExplosao = 250.0f;

    PassaroBomb(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f);
    
    void usarHabilidade() override;
};
#endif // BOMB_H