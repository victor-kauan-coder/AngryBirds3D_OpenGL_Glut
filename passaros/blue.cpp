#include "blue.h"
#include <cstdio>
#include <vector>
#include "../blocos/ParticleManager.h"

// Declaração externa do vetor de pássaros extras (definido em estilingue.cpp)
extern std::vector<Passaro*> extraBirds;
extern ParticleManager g_particleManager;

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
        g_particleManager.createSmokeEffect(pos, 15); // Efeito ao usar habilidade

        btVector3 vel = getVelocidade();
        
        // 1. Cálculo vetorial para achar a "lateral" (Esquerda/Direita do pássaro)
        btVector3 up(0, 1, 0);
        btVector3 forward = vel.normalized();
        btVector3 right = forward.cross(up).normalized();
        
        // Segurança caso o pássaro esteja caindo reto pra baixo (evita erro de vetor nulo)
        if (right.length2() < 0.01f) {
            right = btVector3(1, 0, 0); // Define eixo X como lateral padrão
        }
        btVector3 spreadDir = right.cross(forward).normalized();
            

        
        // --- CONFIGURAÇÃO DO EFEITO ---
        float offsetInicial = 0.2f; 
        float forcaSeparacao = 8.0f; // Força que empurra eles para cima/baixo
        
        // Cria 2 clones (i = -1 para baixo, i = 1 para cima)
        for (int i = -1; i <= 1; i += 2) { 
            
            PassaroBlue* clone = new PassaroBlue(*this);
            
            clone->habilidadeUsada = true; 
            
            // --- MUDANÇA 1: POSIÇÃO (Usando spreadDir) ---
            // Cria o clone deslocado verticalmente em relação à trajetória
            btVector3 clonePos = pos + spreadDir * (offsetInicial * i);
            
            clone->inicializarFisica(mundoFisica, clonePos.x(), clonePos.y(), clonePos.z());
            
            // --- MUDANÇA 2: VELOCIDADE (Usando spreadDir) ---
            if(clone->getRigidBody()){
                clone->getRigidBody()->setActivationState(ACTIVE_TAG);
                
                // Aplica a força de separação na direção vertical calculada
                btVector3 velSeparacao = vel + (spreadDir * (forcaSeparacao * i));
                
                // Removi o "empurrãozinho fixo (0,2,0)" para garantir que a abertura seja simétrica
                
                clone->getRigidBody()->setLinearVelocity(velSeparacao); 
            }

            extraBirds.push_back(clone);
        }
}
