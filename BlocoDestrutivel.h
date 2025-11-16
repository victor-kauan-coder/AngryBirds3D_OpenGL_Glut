#pragma once

#include <string>
#include <map>
#include <cmath>
#include <cstdio> // Para printf
#include <GL/glut.h>
#include <btBulletDynamicsCommon.h>

// Inclui o carregador de OBJ (de onde vem OBJModel) e o STB (para texturas)
// Exatamente como em passaro.h
#include "loads.h"
#include "ParticleManager.h"
#include "audio_manager.h"
#include "stb_image.h"
#include "enums.h"
// --- ENUMS para facilitar a configuração ---

/**
 * @enum MaterialTipo
 * @brief Define as propriedades físicas e de jogo do bloco.
 */


/**
 * @enum EstadoDano
 * @brief Controla o estado atual do bloco (saúde e textura).
 */
enum class EstadoDano {
    INTEIRO,
    DANIFICADO,
    MORRENDO,
    DESTRUIDO
};

extern ParticleManager g_particleManager;
extern AudioManager g_audioManager;


/**
 * @class BlocoDestrutivel
 * @brief Herda de OBJModel para carregar a malha e gerencia
 * a física, saúde e troca de texturas.
 */
class BlocoDestrutivel : public OBJModel {
private:
    // --- ADICIONE ESTAS VARIÁVEIS PRIVADAS ---
    // Timers para as animações
    bool estaAnimandoDano;
    float animDanoTimer;
    float animDanoDuracao;
    bool estaAnimandoDestruicao;
    float animDestruicaoTimer;
    float animDestruicaoDuracao;
    // --- Propriedades de Jogo ---
    float saudeTotal;
    float saudeAtual;
    float corR, corG, corB;
    int pontuacaoValor;
    EstadoDano estado;
    MaterialTipo tipoMaterial;

    // --- Propriedades de Física ---
    btRigidBody* corpoRigido;
    btVector3 dimensoes; // (w, h, d) - Meia-extensão (Half-Extents)
    float massa;
    float atrito;
    float restituicao; // Quão "saltitante" é

    // --- Propriedades Visuais (Texturas) ---
    // Nomes dos arquivos de textura
    std::string texturaNomeInteiro;
    std::string texturaNomeDanificado;
    // IDs das texturas carregadas na GPU
    GLuint texturaIDInteiro = 0;
    GLuint texturaIDDanificado = 0;

    /**
     * @brief Função utilitária para carregar uma textura da memória.
     * (Adaptado de passaro.h)
     */
    GLuint loadTexture(const char* filename) {
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
        if (!data) {
            printf("Aviso: Nao foi possivel carregar a textura %s\n", filename);
            return 0;
        }

        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        return textureID;
    }

public:
    /**
     * @brief Construtor do Bloco.
     * @param tipo O material (MADEIRA, PEDRA, GELO).
     * @param modeloPath O caminho para o arquivo .obj (ex: "Objetos/bloco_barra.obj").
     * @param w Largura total do bloco.
     * @param h Altura total do bloco.
     * @param d Profundidade total (comprimento) do bloco.
     */
    BlocoDestrutivel(MaterialTipo tipo, const char* modeloPath, float w, float h, float d)
        : corpoRigido(nullptr), estado(EstadoDano::INTEIRO),
        estaAnimandoDano(false), animDanoTimer(0.0f), animDanoDuracao(0.3f), // 0.3 segundos de tremor
        estaAnimandoDestruicao(false), animDestruicaoTimer(0.0f), animDestruicaoDuracao(0.5f) // 0.5 segundos encolhendo
    {
        // 1. Define as dimensões da FÍSICA (btBoxShape usa "meias-extensões")
        dimensoes = btVector3(w * 0.5f, h*0.5f, d * 0.5f);
        
        // 2. Define as propriedades com base no material
        tipoMaterial = tipo;
        std::string prefixoTextura;

        switch (tipoMaterial) {
            case MaterialTipo::GELO:
                prefixoTextura = "gelo";
                massa = 4.0f;
                saudeTotal = 2.0f;
                atrito = 0.1f;
                restituicao = 0.2f;
                pontuacaoValor = 150;
                corR = 0.7f; corG = 0.8f; corB = 1.0f;
                break;
            case MaterialTipo::PEDRA:
                prefixoTextura = "pedra";
                massa = 12.0f;
                saudeTotal = 10.0f;
                atrito = 0.8f;
                restituicao = 0.05f;
                pontuacaoValor = 300;
                corR = 0.4f; corG = 0.4f; corB = 0.4f;
                break;
            case MaterialTipo::MADEIRA:
            default:
                prefixoTextura = "madeira";
                massa = 8.0f;
                saudeTotal = 4.0f;
                atrito = 0.6f;
                restituicao = 0.1f;
                pontuacaoValor = 200;
                corR = 0.6f; corG = 0.4f; corB = 0.2f;
                break;
        }
        saudeAtual = saudeTotal;

        // 3. Define os nomes das texturas com base no tipo e forma
        // (Isso assume que seu modelo .obj é uma "barra")
        // Ex: "texturas/madeira_barra.png"
        // Ex: "texturas/madeira_barra_danificada.png"
        std::string forma = "barra";
        texturaNomeInteiro = "Objetos/texturas/" + prefixoTextura + "_" + forma + ".png";
        texturaNomeDanificado = "Objetos/texturas/" + prefixoTextura + "_" + forma + "_danificada.png";

        // 4. Carrega o modelo .obj (ex: "Objetos/bloco_barra.obj")
        // (O 'loads.h' vai normalizar o tamanho, mas vamos corrigir isso no 'desenhar')
        if (!loadFromFile(modeloPath)) {
            printf("ERRO: Falha ao carregar modelo de bloco: %s\n", modeloPath);
        }
    }

    ~BlocoDestrutivel() {
        limparFisica(nullptr);
    }

    void update(float deltaTime) {
        // 1. Atualiza a animação de "tremor"
        if (estaAnimandoDano) {
            animDanoTimer += deltaTime;
            if (animDanoTimer >= animDanoDuracao) {
                estaAnimandoDano = false;
                animDanoTimer = 0.0f;
            }
        }

        // 2. Atualiza a animação de "encolhimento"
        if (estaAnimandoDestruicao) {
            animDestruicaoTimer += deltaTime;
            if (animDestruicaoTimer >= animDestruicaoDuracao) {
                estaAnimandoDestruicao = false;
                estado = EstadoDano::DESTRUIDO; // <-- Só agora ele é marcado para limpeza
            }
        }
    }

    /**
     * @brief Carrega as texturas (Inteira e Danificada) para a GPU.
     * Deve ser chamado DEPOIS que o modelo .obj foi carregado (no construtor).
     */
    void carregarTexturas() {
        texturaIDInteiro = loadTexture(texturaNomeInteiro.c_str());
        texturaIDDanificado = loadTexture(texturaNomeDanificado.c_str());

        // Define a textura inicial no material carregado pelo loads.h
        if (!meshes.empty() && texturaIDInteiro != 0) {
            meshes[0].material.textureID = texturaIDInteiro;
        } else {
             printf("Aviso: Bloco nao tem malhas (meshes) ou textura inteira falhou.\n");
        }
    }

    /**
     * @brief Cria o corpo rígido (btRigidBody) e o adiciona ao mundo.
     * (Adaptado de passaro.h)
     */
    void inicializarFisica(btDiscreteDynamicsWorld* mundoFisica, const btVector3& position, const btQuaternion& rotation = btQuaternion(0, 0, 0, 1)) {
        if (corpoRigido) {
            limparFisica(mundoFisica);
        }

        // 1. Cria a forma de colisão (uma caixa com as dimensões corretas)
        btCollisionShape* blockShape = new btBoxShape(dimensoes);
        
        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(position);
        startTransform.setRotation(rotation);
        
        btVector3 localInertia(0, 0, 0);
        if (massa != 0.f)
            blockShape->calculateLocalInertia(massa, localInertia);

        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(massa, myMotionState, blockShape, localInertia);
        
        corpoRigido = new btRigidBody(rbInfo);
        corpoRigido->setFriction(atrito);
        corpoRigido->setRestitution(restituicao);
        corpoRigido->setDamping(0.3f, 0.3f); // Adiciona "peso" ao movimento

        // Ativa o CCD para evitar tunelamento (como fizemos no estilingue.cpp)
        corpoRigido->setCcdMotionThreshold(dimensoes.x()); // Usa a menor dimensão

        corpoRigido->setCcdSweptSphereRadius(dimensoes.x());

        mundoFisica->addRigidBody(corpoRigido);
    }

    /**
     * @brief Remove o corpo rígido do mundo.
     * (Adaptado de passaro.h)
     */
    void limparFisica(btDiscreteDynamicsWorld* mundoFisica) {
        if (corpoRigido) {
            if (mundoFisica) {
                mundoFisica->removeRigidBody(corpoRigido);
            }
            delete corpoRigido->getMotionState();
            // NÃO delete o collision shape se ele for compartilhado
            // (mas aqui estamos criando um novo, então podemos deletar)
            delete corpoRigido->getCollisionShape();
            delete corpoRigido;
            corpoRigido = nullptr;
        }
    }

    /**
     * @brief Aplica dano ao bloco.
     * Esta função deve ser chamada pelo loop principal do jogo
     * ao detectar uma colisão.
     */
void aplicarDano(float dano) {
        if (estado == EstadoDano::MORRENDO || estado == EstadoDano::DESTRUIDO) return;

        saudeAtual -= dano;

        // --- Lógica de Troca de Textura (Dano) ---
        if (saudeAtual <= saudeTotal * 0.5f && estado == EstadoDano::INTEIRO) {
            printf("Bloco danificado!\n");
            estado = EstadoDano::DANIFICADO;
            if (!meshes.empty() && texturaIDDanificado != 0) {
                meshes[0].material.textureID = texturaIDDanificado;
            }
        }

        // --- ADICIONADO: Inicia a animação de tremor ---
        estaAnimandoDano = true;
        animDanoTimer = 0.0f;

        // --- Lógica de Destruição ---
        if (saudeAtual <= 0) {
            printf("Bloco destruido! +%d pontos!\n", pontuacaoValor);
            estado = EstadoDano::MORRENDO; // <-- MUDADO (Inicia a animação)
            estaAnimandoDestruicao = true; // <-- ADICIONADO
            animDestruicaoTimer = 0.0f;    // <-- ADICIONADO
            btVector3 pos = corpoRigido->getCenterOfMassPosition();
            // Cria a explosão de partículas com a cor do material
            g_particleManager.createExplosion(pos, btVector3(corR, corG, corB));
            g_audioManager.playDestruction(tipoMaterial);
            // Não limpe a física aqui, a animação precisa tocar primeiro!
        }
    }

    /**
     * @brief Desenha o bloco (Override de OBJModel::draw).
     * (Adaptado de passaro.h)
     */
    // Em BlocoDestrutivel.h

    void desenhar() {
        // Agora, 'DESTRUIDO' significa "limpeza", então não desenhe
        if (estado == EstadoDano::DESTRUIDO) return; 
        if (!corpoRigido) return;

        btTransform trans;
        corpoRigido->getMotionState()->getWorldTransform(trans);
        
        btScalar m[16];
        trans.getOpenGLMatrix(m);
        
        glPushMatrix();
        glMultMatrixf(m);

        // --- ADICIONADO: Lógica da Animação de Tremor (Dano) ---
        if (estaAnimandoDano) {
            // Calcula um tremor rápido (seno) no eixo X
            float shakeOffset = sin(animDanoTimer * 100.0f) * 0.05f; // (Frequência * Amplitude)
            glTranslatef(shakeOffset, 0, 0);
        }

        // --- ADICIONADO: Lógica da Animação de Encolhimento (Destruição) ---
        if (estaAnimandoDestruicao) {
            // Calcula a escala de 1.0 (início) para 0.0 (fim)
            float t = (animDestruicaoTimer / animDestruicaoDuracao); // 0.0 -> 1.0
            float escala = 1.0f - t; // 1.0 -> 0.0
            
            // Aplica a escala de encolhimento
            glScalef(escala, escala, escala);
        }
        // --- Correção da Escala da Normalização (código original) ---
        float maxDimOriginal = std::max({dimensoes.x()*2, dimensoes.y()*2, dimensoes.z()*2});
        float escalaNecessaria = maxDimOriginal / 0.6f;
        glScalef(escalaNecessaria, escalaNecessaria, escalaNecessaria);

        OBJModel::draw(); 
        glPopMatrix();
    }

    // --- Getters Úteis ---
    btRigidBody* getRigidBody() { return corpoRigido; }
    bool isDestruido() const { return estado == EstadoDano::DESTRUIDO; }
    int getPontuacao() const { return pontuacaoValor; }
};