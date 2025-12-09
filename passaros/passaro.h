#ifndef PASSARO_H
#define PASSARO_H

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <string>
#include <btBulletDynamicsCommon.h>
#include "../util/loads.h"
#include "../controle_audio/audio_manager.h"
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern AudioManager g_audioManager;

class Passaro {
protected:
    OBJModel modelo;
    btRigidBody* rigidBody;
    btCollisionShape* colisaoShape;
    btDefaultMotionState* motionState;
    btDiscreteDynamicsWorld* mundoFisica;
    
    float escala;
    float raioColisao;
    bool ativo;
    bool emVoo;
    float corR, corG, corB;
    float massa;
    std::string tipo;
    float restituicao;
    float friccao;
    float amortecimentoLinear;
    float amortecimentoAngular;
    float rotacaoVisualX, rotacaoVisualY, rotacaoVisualZ;
    float anguloVisual;
    float tempoVida;
    float tempoVoo;
    float tempoVidaMaximo;
    float tempoVooMaximo;
    
    // --- NOVO: Última posição conhecida (para quando o corpo físico é removido) ---
    btVector3 ultimaPosicao;

    // --- VARIÁVEIS DA FILA ---
    bool naFila;
    btVector3 posFila;          
    float timerPuloFila;        
    float intervaloPuloFila;    
    bool pulandoFila;           
    float progressoPulo;        

    // --- NOVO: Variáveis para "Andar na Fila" ---
    bool andandoNaFila;
    btVector3 posOrigemAndar;
    btVector3 posDestinoAndar;
    float progressoAndar;

public:
    Passaro(float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f, 
            float raio = 0.5f, float escalaInicial = 1.0f);

    virtual ~Passaro();
    
    void setNaFila(bool status, btVector3 pos);

    btVector3 getPosFila() const { return posFila; }
    bool isNaFila() const { return naFila; }

    // --- NOVO: Função para iniciar o movimento na fila ---
    void caminharNaFilaPara(btVector3 destino);

    // Animação unificada da fila
    void atualizarFila(float deltaTime);

    void desenharNaFila();

    void inicializarFisica(btDiscreteDynamicsWorld* mundo, float posX, float posY, float posZ);
    
    void limparFisica();
    
    bool carregarModelo(const char* caminhoOBJ);
    bool carregarMTL(const char* caminhoMTL);
    
    virtual void desenhar();
    
    virtual void desenharEmPosicao(float x, float y, float z);

    void desenharEsferaColisao();
    
    virtual void atualizar(float deltaTime);
    
    virtual void usarHabilidade() {}
    
    virtual void resetar(float posX, float posY, float posZ);
    
    btVector3 getPosicao() const;
    
    float getX() const { return getPosicao().x(); }
    float getY() const { return getPosicao().y(); }
    float getZ() const { return getPosicao().z(); }
    
    void setPosicao(float posX, float posY, float posZ);
    
    btVector3 getVelocidade() const;
    float getVelocidadeX() const { return getVelocidade().x(); }
    float getVelocidadeY() const { return getVelocidade().y(); }
    float getVelocidadeZ() const { return getVelocidade().z(); }
    void setVelocidade(float velX, float velY, float velZ);
    btQuaternion getRotacao() const;
    void setRotacao(float eixoX, float eixoY, float eixoZ, float anguloRad);
    void setRotacaoVisual(float eixoX, float eixoY, float eixoZ, float anguloRad);
    float getEscala() const { return escala; }
    void setEscala(float valor) { escala = valor; }
    float getRaioColisao() const { return raioColisao; }
    void setRaioColisao(float raio) { raioColisao = raio; }
    bool isAtivo() const { return ativo; }
    void setAtivo(bool valor) { ativo = valor; }
    bool isEmVoo() const { return emVoo; }
    void setEmVoo(bool valor) { emVoo = valor; }
    void setCor(float r, float g, float b) { corR = r; corG = g; corB = b; }
    float getMassa() const { return massa; }
    void setMassa(float valor);
    std::string getTipo() const { return tipo; }
    void setTipo(const std::string& novoTipo) { tipo = novoTipo; }
    OBJModel& getModelo() { return modelo; }
    btRigidBody* getRigidBody() { return rigidBody; }
    const btRigidBody* getRigidBody() const { return rigidBody; }
    void setRigidBodyInfo(const btRigidBody::btRigidBodyConstructionInfo& rbInfo);
    float getRestituicao() const { return restituicao; }
    void setRestituicao(float valor);
    btDefaultMotionState* getMotionState() { return motionState; }
    const btDefaultMotionState* getMotionState() const { return motionState; }    
    void setMotionState(btDefaultMotionState* ms);
    btCollisionShape* getColisaoShape() { return colisaoShape; }
    const btCollisionShape* getColisaoShape() const { return colisaoShape; }
    void setColisaoShape(btCollisionShape* shape);
    float getFriccao() const { return friccao; }
    void setFriccao(float valor);
    void setAmortecimento(float linear, float angular);
};

#endif // PASSARO_H