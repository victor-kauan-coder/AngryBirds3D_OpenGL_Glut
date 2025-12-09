#ifndef RED_H
#define RED_H

#include "passaro.h"
/**
 * Exemplo de classe derivada: Red (Pássaro vermelho - padrão)
 * Este é o pássaro básico do Angry Birds
 */
class PassaroRed : public Passaro {
public:
    PassaroRed(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f);
    
    void usarHabilidade() override;
};
#endif // RED_H