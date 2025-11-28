#pragma once
#include "passaro.h"
/**
 * Exemplo de classe derivada: Red (Pássaro vermelho - padrão)
 * Este é o pássaro básico do Angry Birds
 */
class PassaroRed : public Passaro {
public:
    PassaroRed(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f)
        : Passaro(posX, posY, posZ, 1.2f, 1.2f) {
        setCor(1.0f, 0.0f, 0.0f); // Vermelho
        setMassa(3.0f);
        setTipo("Red");
        carregarModelo("Objetos/blue.obj");
        carregarMTL("Objetos/blue.mtl");

        setRaioColisao(1.2f);
    }
    
    void usarHabilidade() override {
        // Red não tem habilidade especial, apenas é forte e pesado
        // Poderia emitir um grito ou algo assim
    }
};