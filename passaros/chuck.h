#ifndef CHUCK_H
#define CHUCK_H

#include "passaro.h"
/**
 * Exemplo de classe derivada: Chuck (Pássaro amarelo - rápido)
 * Este é o pássaro rápido do Angry Birds
 */
class PassaroChuck : public Passaro {
public:
    PassaroChuck(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f)
        : Passaro(posX, posY, posZ, 1.2f, 1.2f) {
        setCor(1.0f, 1.0f, 0.0f); // Amarelo
        setMassa(3.0f);
        setTipo("Chuck");
        carregarModelo("Objetos/yellow.obj");
        carregarMTL("Objetos/yellow.mtl");

        setRaioColisao(1.2f);
    }
    
    void usarHabilidade() override {
        if (!isEmVoo()) return;
        printf("Habilidade Chuck ativada: Acelerar!\n");
        
        btVector3 velAtual = getVelocidade();
        btVector3 direcao = velAtual.normalized();
        
        btVector3 novaVelocidade = velAtual + direcao * (velAtual.length() * 1.4f);
        setVelocidade(novaVelocidade.x(), novaVelocidade.y(), novaVelocidade.z());
        
        // g_audioManager.loadSound(SomTipo::CHUCK_SPEED_BOOST, "chuck_speed_boost.wav");
    }
};
#endif // CHUCK_H