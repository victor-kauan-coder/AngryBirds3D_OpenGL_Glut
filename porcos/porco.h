#ifndef PORCO_H
#define PORCO_H

#include <GL/glut.h>
#include <btBulletDynamicsCommon.h>
#include "../util/loads.h"
#include <string>

class Porco {
protected:
    OBJModel modelo;
    btRigidBody* rigidBody;
    btCollisionShape* colisaoShape;
    btDefaultMotionState* motionState;
    btDiscreteDynamicsWorld* mundoFisica;

    float escala;
    float raioColisao;
    
    bool ativo;
    float vida;
    float vidaMaxima;

    float massa;
    std::string tipo;
    float restituicao;
    float friccao;
    float amortecimentoLinear;
    float amortecimentoAngular;

    float timerPulo;
    float intervaloPulo;
    
    // --- NOVO: Controle de Dano Recente ---
    float tempoSemDano; // Conta quanto tempo faz desde o último hit
    int pontuacaoValor; // Valor de pontos ao destruir

public:
    Porco(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f, 
        float raio = 0.2f, float escalaInicial = 2.3f, int pontos = 5000);
    
    virtual ~Porco();
    
    void inicializarFisica(btDiscreteDynamicsWorld* mundo, float posX, float posY, float posZ);
    void limparFisica();
    
    bool carregarModelo(const char* caminhoOBJ);
    bool carregarMTL(const char* caminhoMTL);
    
    virtual void desenhar();
    virtual void atualizar(float deltaTime);
    
    void tomarDano(float dano);
    
    virtual void resetar(float posX, float posY, float posZ);
    
    // Getters e Setters
    btVector3 getPosicao() const;
    btVector3 getVelocidade() const;
    bool isAtivo() const { return ativo; }
    void setAtivo(bool valor) { ativo = valor; }
    float getVida() const { return vida; }
    void setVida(float valor) { vida = valor; }
    btRigidBody* getRigidBody() { return rigidBody; }
    const btRigidBody* getRigidBody() const { return rigidBody; }
};

#endif // PORCO_H
