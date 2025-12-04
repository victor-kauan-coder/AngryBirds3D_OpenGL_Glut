#ifndef BLUE_H
#define BLUE_H

#include "passaro.h"
#include <vector>

/**
 * Classe derivada: Blue (Pássaro azul - divide em 3)
 */
class PassaroBlue : public Passaro {
private:
    bool habilidadeUsada;

public:
    PassaroBlue(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f);
    
    void usarHabilidade() override;
};

#endif // BLUE_H