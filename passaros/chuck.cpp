#include "chuck.h"

PassaroChuck::PassaroChuck(float posX, float posY, float posZ)
    : Passaro(posX, posY, posZ, 1.2f, 1.2f) {
    setCor(1.0f, 1.0f, 0.0f); // Amarelo
    setMassa(3.0f);
    setTipo("Chuck");
    carregarModelo("Objetos/yellow.obj");
    carregarMTL("Objetos/yellow.mtl");
    
    setRaioColisao(1.2f);
}

void PassaroChuck::usarHabilidade() {
    if (!isEmVoo()) return;

    g_particleManager.createSmokeEffect(getPosicao(), 20); 
    btVector3 velAtual = getVelocidade();
    btVector3 direcao = velAtual.normalized();
    
    btVector3 novaVelocidade = velAtual + direcao * (velAtual.length() * 1.4f);
    setVelocidade(novaVelocidade.x(), novaVelocidade.y(), novaVelocidade.z());
    
    // g_audioManager.loadSound(SomTipo::CHUCK_SPEED_BOOST, "chuck_speed_boost.wav");
}
