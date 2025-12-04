#ifndef BOMB_H
#define BOMB_H

#include "passaro.h"
#include <vector>

/**
 * Classe derivada: Bomb (Bomba)
 * Este é o pássaro bomba do Angry Birds
 */
class PassaroBomb : public Passaro {
public:
    // Parâmetros da explosão (fáceis de manipular)
    float raioExplosao = 7.5f;
    float forcaExplosao = 100.0f;

    PassaroBomb(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f)
        : Passaro(posX, posY, posZ, 1.2f, 1.2f) {
        setCor(0.1f, 0.1f, 0.1f); // Preto
        setMassa(5.0f); // Mais pesado
        setTipo("Bomb");
        carregarModelo("Objetos/Bomb.obj");
        carregarMTL("Objetos/Bomb.mtl");

        setRaioColisao(1.2f);
    }

    void desenhar() override {
        Passaro::desenhar(); // Desenha o pássaro normal

        if (!rigidBody) return;

        // Desenha o raio de explosão para debug
        btTransform transform;
        rigidBody->getMotionState()->getWorldTransform(transform);
        btVector3 pos = transform.getOrigin();

        glPushMatrix();
        glTranslatef(pos.x(), pos.y(), pos.z());
        
        // Cor vermelha para o raio de explosão
        glColor3f(1.0f, 0.0f, 0.0f); 
        glDisable(GL_LIGHTING); // Para ver melhor as linhas
        glutWireSphere(raioExplosao, 20, 20);
        glEnable(GL_LIGHTING);
        
        glPopMatrix();
    }
    
    void usarHabilidade() override {
        if (!isEmVoo()) return;
        tempoVoo = tempoVooMaximo; // Marca para remoção imediata
        
        
        printf("Habilidade Bomb ativada: EXPLOSAO! (Raio: %.1f, Forca: %.1f)\n", raioExplosao, forcaExplosao);
        
        btVector3 centroExplosao = getPosicao();
        
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
                
                // Opcional: Adicionar torque aleatório para girar os objetos
                btVector3 torque(
                    (rand() % 100 - 50) * 0.1f,
                    (rand() % 100 - 50) * 0.1f,
                    (rand() % 100 - 50) * 0.1f
                );
                body->applyTorqueImpulse(torque * forcaExplosao * 0.1f);

            }
        }
        
        // Opcional: Efeito visual ou sonoro aqui
        // g_audioManager.playExplosao();
    }
};
#endif // BOMB_H