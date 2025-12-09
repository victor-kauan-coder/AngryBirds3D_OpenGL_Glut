#include "red.h"

PassaroRed::PassaroRed(float posX, float posY, float posZ)
    : Passaro(posX, posY, posZ, 1.2f, 1.2f) {
    setCor(1.0f, 0.0f, 0.0f); // Vermelho
    setMassa(3.0f);
    setTipo("Red");
    carregarModelo("Objetos/redV2.obj");
    carregarMTL("Objetos/redV2.mtl");

    setRaioColisao(1.2f);
}

void PassaroRed::usarHabilidade() {

}
