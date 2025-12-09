#include "bomb.h"
#include <iostream>

PassaroBomb::PassaroBomb(float posX, float posY, float posZ)
    : Passaro(posX, posY, posZ, 1.2f, 1.2f) {
    setCor(0.1f, 0.1f, 0.1f); // Preto
    setMassa(3.5f); // Mais pesado
    setTipo("Bomb");
    carregarModelo("Objetos/Bomb.obj");
    carregarMTL("Objetos/Bomb.mtl");

    setRaioColisao(1.2f);
}

void PassaroBomb::usarHabilidade() {
    if (!isEmVoo()) return;
    tempoVoo = tempoVooMaximo; // Marca para remoção imediata
    
    
    printf("Habilidade Bomb ativada: EXPLOSAO! (Raio: %.1f, Forca: %.1f)\n", raioExplosao, forcaExplosao);
    
    btVector3 centroExplosao = getPosicao();

    // Efeito visual da explosão
    g_particleManager.createFireExplosion(centroExplosao, 100, raioExplosao / 2.0f);
    
    // Itera sobre todos os objetos de colisão no mundo
    int numObjects = mundoFisica->getNumCollisionObjects();
    for (int i = 0; i < numObjects; i++) {
        btCollisionObject* obj = mundoFisica->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        
        // Verifica se é um corpo rígido válido e não é o próprio pássaro
        if (!body || body == rigidBody) continue;
        
        // Ignora objetos estáticos (massa infinita/invMass 0), como o chão
        if (body->getInvMass() == 0) continue;

        btVector3 posObjeto = body->getCenterOfMassPosition();
        btScalar dist = centroExplosao.distance(posObjeto);
        
        // Se o objeto estiver dentro do raio da explosão
        if (dist < raioExplosao) {
            btVector3 direcao = posObjeto - centroExplosao;
            if (direcao.length2() > 0.0001f) {
                direcao.normalize();
            } else {
                direcao = btVector3(0, 1, 0); // Se estiver exatamente no mesmo lugar, joga pra cima
            }
            
            // Cálculo do impulso com decaimento linear (mais forte perto do centro)
            float fatorDistancia = 1.0f - (dist / raioExplosao);
            btVector3 impulso = direcao * forcaExplosao * fatorDistancia;
            
            body->activate(true); // Acorda o objeto se estiver dormindo
            body->applyCentralImpulse(impulso);
            
            //adiciona torque aleatório para efeito extra
            btVector3 torque(
                (rand() % 100 - 50) * 0.1f,
                (rand() % 100 - 50) * 0.1f,
                (rand() % 100 - 50) * 0.1f
            );
            body->applyTorqueImpulse(torque * forcaExplosao * 0.1f);

        }
    }
    
    // Opcional: Efeito visual ou sonoro aqui
    g_audioManager.playPassaro(SomTipo::EXPLOSAO, 55);
}
