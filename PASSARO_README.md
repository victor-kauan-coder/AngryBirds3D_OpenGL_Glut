# Sistema de Pássaros - Angry Birds 3D

## Descrição

Sistema de classes para implementar os pássaros do Angry Birds em OpenGL 3D.

## Estrutura de Classes

### Classe Base: `Passaro`

Classe abstrata que serve como base para todos os pássaros do jogo.

**Características principais:**
- ✅ Herda de `OBJModel` para carregar modelos 3D
- ✅ Colisão esférica
- ✅ Sistema de física básico (gravidade, velocidade, posição)
- ✅ Rotação em 3 eixos
- ✅ Sistema de cores RGB
- ✅ Método virtual `usarHabilidade()` para habilidades especiais
- ✅ Sistema de debug para visualizar esfera de colisão

**Propriedades:**
```cpp
- float x, y, z                    // Posição
- float velocidadeX, Y, Z          // Velocidade
- float rotacaoX, Y, Z             // Rotação
- float escala                     // Tamanho
- float raioColisao                // Raio da esfera de colisão
- bool ativo, emVoo                // Estados
- float corR, G, B                 // Cor RGB
- float massa                      // Massa (física)
- OBJModel modelo                  // Modelo 3D
```

## Pássaros Implementados (Exemplos)

### 1. PassaroRed (Vermelho)
- **Cor:** Vermelho
- **Habilidade:** Nenhuma (pássaro básico)
- **Características:** Forte e balanceado

### 2. PassaroChuck (Amarelo)
- **Cor:** Amarelo
- **Habilidade:** Acelera drasticamente quando ativado
- **Características:** Rápido e ágil

### 3. PassaroBomb (Preto)
- **Cor:** Preto
- **Habilidade:** Explosão em área
- **Características:** Pesado e explosivo

### 4. PassaroBlues (Azuis)
- **Cor:** Azul claro
- **Habilidade:** Se divide em 3 pássaros
- **Características:** Pequenos e múltiplos

### 5. PassaroMatilda (Branco)
- **Cor:** Branco
- **Habilidade:** Solta um ovo explosivo e sobe
- **Características:** Mãe protetora

## Como Criar um Novo Tipo de Pássaro

```cpp
class MeuPassaro : public Passaro {
public:
    MeuPassaro(float x, float y, float z) 
        : Passaro(x, y, z, raio, escala) {
        setCor(r, g, b);        // Define a cor
        setMassa(massa);        // Define a massa
        setTipo("MeuPassaro");  // Define o tipo
    }
    
    // Implementa a habilidade especial
    void usarHabilidade() override {
        // Seu código aqui
    }
    
    // Opcional: customizar desenho
    void desenhar() override {
        // Efeitos especiais
        Passaro::desenhar(); // Chama o desenho base
    }
};
```

## Exemplo de Uso

```cpp
#include "passaro.h"

// Criar um pássaro
PassaroRed* red = new PassaroRed(-5.0f, 0.0f, 0.0f);

// Carregar modelo 3D (opcional)
red->carregarModelo("Objetos/red.obj");

// Lançar o pássaro
red->lancar(velocidadeX, velocidadeY, velocidadeZ);

// Atualizar física (em loop)
red->atualizar(deltaTime);

// Desenhar (em loop de renderização)
red->desenhar();

// Usar habilidade
red->usarHabilidade();

// Verificar colisão com outro objeto
if (red->verificarColisaoEsferica(x, y, z, raio)) {
    // Tratar colisão
}

// Resetar posição
red->resetar(x, y, z);

// Limpeza
delete red;
```

## Métodos Principais

### Construtor
```cpp
Passaro(float posX, float posY, float posZ, 
        float raio, float escalaInicial)
```

### Métodos de Controle
- `void lancar(float velX, float velY, float velZ)` - Lança o pássaro
- `void atualizar(float deltaTime)` - Atualiza física
- `void desenhar()` - Renderiza o pássaro
- `void usarHabilidade()` - Ativa habilidade especial
- `void resetar(float x, float y, float z)` - Reseta posição

### Métodos de Colisão
- `bool verificarColisaoEsferica(float x, float y, float z, float raio)`
- `bool verificarColisao(const Passaro& outro)`
- `void desenharEsferaColisao()` - Debug visual

### Getters/Setters
Todos os atributos possuem getters e setters:
- `getPosicao()`, `setPosicao()`
- `getVelocidade()`, `setVelocidade()`
- `getRotacao()`, `setRotacao()`
- `getCor()`, `setCor()`
- `getMassa()`, `setMassa()`
- `isAtivo()`, `setAtivo()`
- `isEmVoo()`, `setEmVoo()`
- etc.

## Compilação

### Com Makefile
```bash
# Compilar passaro.cpp
g++ -g -I. -IC:/msys64/mingw64/include/bullet passaro.cpp \
    -o output/passaro.exe \
    -lfreeglut -lopengl32 -lglu32 \
    -lBulletDynamics -lBulletCollision -lLinearMath
```

### Flags importantes
- `-I.` - Inclui headers locais (loads.h, passaro.h)
- `-IC:/msys64/mingw64/include/bullet` - Headers do Bullet Physics
- `-lfreeglut -lopengl32 -lglu32` - Bibliotecas OpenGL
- `-lBulletDynamics -lBulletCollision -lLinearMath` - Bullet Physics (opcional)

## Debug

Para visualizar as esferas de colisão, adicione antes de incluir `passaro.h`:

```cpp
#define DEBUG_COLISAO
#include "passaro.h"
```

## Controles (Exemplo no passaro.cpp)

- **1** - Lançar Red (vermelho)
- **2** - Lançar Chuck (amarelo)
- **3** - Lançar Bomb (preto)
- **ESPAÇO** - Usar habilidade especial
- **R** - Resetar pássaros
- **ESC** - Sair

## Próximos Passos

1. Integrar com sistema de física Bullet
2. Adicionar sistema de partículas para efeitos
3. Implementar sons para cada pássaro
4. Criar sistema de trajetória prevista
5. Adicionar mais tipos de pássaros (Hal, Terence, Stella, etc.)

## Dependências

- OpenGL
- GLUT/FreeGLUT
- loads.h (carregamento de OBJ)
- openGL_util.h (utilitários OpenGL)

## Licença

Este código é parte do projeto Angry Birds 3D - OpenGL.
