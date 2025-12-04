#include "blue.h"
#include <cstdio>
#include <vector>

// Declaração externa do vetor de pássaros extras (definido em estilingue.cpp)
extern std::vector<Passaro*> extraBirds;

PassaroBlue::PassaroBlue(float posX, float posY, float posZ)
    : Passaro(posX, posY, posZ, 0.8f, 0.8f), habilidadeUsada(false) {
    setCor(0.0f, 0.0f, 1.0f); // Azul
    setMassa(1.5f);
    setTipo("Blue");
    // Se tiver modelo, carregar. Se não, usa esfera azul.
    carregarModelo("Objetos/blue.obj");
    carregarMTL("Objetos/blue.mtl");
    
    setRaioColisao(0.8f);
}

void PassaroBlue::usarHabilidade() {
    if (habilidadeUsada || !isEmVoo() || !mundoFisica) return;
    
    printf("Habilidade Blue ativada: Multiplicar!\n");
    habilidadeUsada = true;
    
    btVector3 pos = getPosicao();
    btVector3 vel = getVelocidade();
    
    // Vetor "Right" (perpendicular à velocidade e ao UP)
    btVector3 up(0, 1, 0);
    btVector3 forward = vel.normalized();
    btVector3 right = forward.cross(up).normalized();
    
    // Se a velocidade for muito vertical, o cross product falha.
    if (right.length2() < 0.01f) {
        right = btVector3(1, 0, 0);
    }
    
    float offset = 1.5f; // Distância lateral
    float spreadImpulse = 3.0f; // Força lateral
    
    // Cria 2 clones
    for (int i = -1; i <= 1; i += 2) { // -1 e 1
        PassaroBlue* clone = new PassaroBlue(pos.x(), pos.y(), pos.z());
        
        // Inicializa física deslocada
        btVector3 clonePos = pos + right * (offset * i);
        clone->inicializarFisica(mundoFisica, clonePos.x(), clonePos.y(), clonePos.z());
        
        // CRÍTICO: Força o corpo a acordar, pois inicializarFisica coloca em SLEEPING
        clone->getRigidBody()->setActivationState(ACTIVE_TAG);
        clone->getRigidBody()->activate(true);

        // Define mesma velocidade
        clone->setVelocidade(vel.x(), vel.y(), vel.z());
        
        // Aplica impulso lateral
        clone->getRigidBody()->applyCentralImpulse(right * (spreadImpulse * i));
        
        clone->setEmVoo(true);
        clone->habilidadeUsada = true; // Clones não podem multiplicar de novo
        
        // Adiciona à lista global
        extraBirds.push_back(clone);
    }
}
