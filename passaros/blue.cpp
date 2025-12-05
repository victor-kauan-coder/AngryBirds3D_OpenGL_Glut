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
    
    // printf("Habilidade Blue ativada: Multiplicar!\n");
    habilidadeUsada = true;
    
    btVector3 pos = getPosicao();
    btVector3 vel = getVelocidade();
    
    // Cálculo vetorial para espalhar os clones
    btVector3 up(0, 1, 0);
    btVector3 forward = vel.normalized();
    btVector3 right = forward.cross(up).normalized();
    
    if (right.length2() < 0.01f) {
        right = btVector3(1, 0, 0);
    }
    
    float offset = 1.5f; 
    
    // Cria 2 clones
    for (int i = -1; i <= 1; i += 2) { // Loop para -1 e 1
        
        // --- CORREÇÃO DE PERFORMANCE AQUI ---
        // Em vez de: new PassaroBlue(...) que carrega o modelo do HD.
        // Usamos: new PassaroBlue(*this) que copia o modelo da RAM.
        PassaroBlue* clone = new PassaroBlue(*this);
        
        // O clone já nasce com a habilidade "gasta" para não se multiplicar infinitamente
        clone->habilidadeUsada = true; 
        
        // Calcula posição deslocada
        btVector3 clonePos = pos + right * (offset * i);
        
        // --- IMPORTANTE: Reinicializar a Física ---
        // O clone copiou o ponteiro do corpo rígido do pai. 
        // Precisamos criar um NOVO corpo físico exclusivo para o clone.
        // O método inicializarFisica vai sobrescrever o ponteiro antigo, o que é o desejado.
        clone->inicializarFisica(mundoFisica, clonePos.x(), clonePos.y(), clonePos.z());
        
        // Garante que o clone acorde na simulação
        if(clone->getRigidBody()){
             clone->getRigidBody()->setActivationState(ACTIVE_TAG);
             clone->getRigidBody()->setLinearVelocity(vel); // Mantém a velocidade do pai
        }

        // Adiciona à lista global
        extraBirds.push_back(clone);
    }
}
