#pragma once
#include "passaro.h"
/**
 * Exemplo de classe derivada: Red (Pássaro vermelho - padrão)
 * Este é o pássaro básico do Angry Birds
 */
class PassaroRed : public Passaro {
public:
    PassaroRed(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f)
        : Passaro(posX, posY, posZ, 0.5f, 1.0f) {
        setCor(1.0f, 0.0f, 0.0f); // Vermelho
        setMassa(1.0f);
        setTipo("Red");
        carregarModelo("Objetos/yellow.obj");
        carregarMTL("Objetos/yellow.mtl");
    }
    
    void usarHabilidade() override {
        // Red não tem habilidade especial, apenas é forte e pesado
        // Poderia emitir um grito ou algo assim
    }
};