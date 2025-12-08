#pragma once

#include <string>
#include <map>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <GL/glut.h>
#include <btBulletDynamicsCommon.h>

#include "../util/loads.h"
#include "ParticleManager.h"
#include "../controle_audio/audio_manager.h"
#include "../stb/stb_image.h"
#include "../util/enums.h"

enum class EstadoDano {
    INTEIRO,
    DANIFICADO,
    MORRENDO,
    DESTRUIDO
};

extern ParticleManager g_particleManager;
extern AudioManager g_audioManager;

class BlocoDestrutivel : public OBJModel {
private:
    // --- MUDANÇA 1: Escala separada por eixo ---
    float scaleX, scaleY, scaleZ; 
    
    bool estaAnimandoDano;
    float animDanoTimer;
    float animDanoDuracao;
    bool estaAnimandoDestruicao;
    float animDestruicaoTimer;
    float animDestruicaoDuracao;
    
    float saudeTotal;
    float saudeAtual;
    float corR, corG, corB;
    int pontuacaoValor;
    EstadoDano estado;
    MaterialTipo tipoMaterial;

    bool isContactActive;
    btRigidBody* corpoRigido;
    btVector3 dimensoes;
    float massa;
    float atrito;
    float restituicao;

    std::string texturaNomeInteiro;
    std::string texturaNomeDanificado;
    GLuint texturaIDInteiro = 0;
    GLuint texturaIDDanificado = 0;

    static std::map<std::string, GLuint>& getTextureCache() {
        static std::map<std::string, GLuint> cache;
        return cache;
    }

    GLuint loadTexture(const char* filename) {
        std::string strFilename(filename);
        auto& cache = getTextureCache();
        if (cache.find(strFilename) != cache.end()) return cache[strFilename];

        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
        if (!data) return 0;

        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        cache[strFilename] = textureID;
        return textureID;
    }

    static std::map<std::string, OBJModel*>& getModelCache() {
        static std::map<std::string, OBJModel*> cache;
        return cache;
    }

    void carregarModeloCacheado(const char* path) {
        std::string key = path;
        auto& cache = getModelCache();

        if (cache.find(key) == cache.end()) {
            OBJModel* novoModelo = new OBJModel();
            if (novoModelo->loadFromFile(path)) {
                cache[key] = novoModelo;
            } else {
                printf("ERRO FATAL: Nao foi possivel carregar modelo: %s\n", path);
                delete novoModelo;
                return;
            }
        }

        OBJModel* cached = cache[key];
        if (cached) {
            this->vertices = cached->vertices;
            this->normals = cached->normals;
            this->texCoords = cached->texCoords;
            this->meshes = cached->meshes;
            this->materials = cached->materials;
            this->displayListID = 0; 
            this->needsUpdate = true;
        }
    }

public:
    BlocoDestrutivel(MaterialTipo tipo, const char* modeloPath, float w, float h, float d)
        : OBJModel(),
        corpoRigido(nullptr), estado(EstadoDano::INTEIRO), isContactActive(false),
        estaAnimandoDano(false), animDanoTimer(0.0f), animDanoDuracao(0.3f),
        estaAnimandoDestruicao(false), animDestruicaoTimer(0.0f), animDestruicaoDuracao(0.5f)
    {
        dimensoes = btVector3(w * 0.5f, h*0.5f, d * 0.5f);
        tipoMaterial = tipo;
        std::string prefixoTextura;

        switch (tipoMaterial) {
            case MaterialTipo::GELO:
                prefixoTextura = "gelo";
                massa = 4.0f; saudeTotal = 10.0f; atrito = 0.1f; restituicao = 0.2f; pontuacaoValor = 150;
                corR = 0.7f; corG = 0.8f; corB = 1.0f; break;
            case MaterialTipo::PEDRA:
                prefixoTextura = "pedra";
                massa = 12.0f; saudeTotal = 30.0f; atrito = 10.0f; restituicao = 0.05f; pontuacaoValor = 300;
                corR = 0.4f; corG = 0.4f; corB = 0.4f; break;
            case MaterialTipo::MADEIRA:
            default:
                prefixoTextura = "madeira";
                massa = 8.0f; saudeTotal = 10.0f; atrito = 0.6f; restituicao = 0.1f; pontuacaoValor = 200;
                corR = 0.6f; corG = 0.4f; corB = 0.2f; break;
        }
        saudeAtual = saudeTotal;

        std::string forma = "barra"; 
        texturaNomeInteiro = "Objetos/texturas/" + prefixoTextura + "_" + forma + ".png";
        texturaNomeDanificado = "Objetos/texturas/" + prefixoTextura + "_" + forma + "_danificada.png";

        carregarModeloCacheado(modeloPath);

        // --- MUDANÇA 2: Cálculo Robusto de Escala ---
        // Calcula o tamanho real do modelo carregado (Bounding Box)
        float minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9, minZ = 1e9, maxZ = -1e9;
        
        if (vertices.empty()) {
            // Fallback se o modelo falhar: assume tamanho 1.0
            scaleX = w; scaleY = h; scaleZ = d;
        } else {
            for (size_t i = 0; i < vertices.size(); i += 3) {
                float vx = vertices[i];
                float vy = vertices[i+1];
                float vz = vertices[i+2];
                if (vx < minX) minX = vx; if (vx > maxX) maxX = vx;
                if (vy < minY) minY = vy; if (vy > maxY) maxY = vy;
                if (vz < minZ) minZ = vz; if (vz > maxZ) maxZ = vz;
            }
            float modelW = maxX - minX;
            float modelH = maxY - minY;
            float modelD = maxZ - minZ;

            // Evita divisão por zero
            if (modelW < 0.001f) modelW = 1.0f;
            if (modelH < 0.001f) modelH = 1.0f;
            if (modelD < 0.001f) modelD = 1.0f;

            // Define a escala para que o modelo preencha EXATAMENTE a caixa de física (w, h, d)
            scaleX = w / modelW;
            scaleY = h / modelH;
            scaleZ = d / modelD;
        }
    }

    ~BlocoDestrutivel() {
        limparFisica(nullptr);
    }

    void clearContactFlag() { isContactActive = false; }
    
    bool registerContact() {
        if (isContactActive) return false;
        isContactActive = true;
        return true;
    }

    void update(float deltaTime) {
        if (estaAnimandoDano) {
            animDanoTimer += deltaTime;
            if (animDanoTimer >= animDanoDuracao) estaAnimandoDano = false;
        }
        if (estaAnimandoDestruicao) {
            animDestruicaoTimer += deltaTime;
            if (animDestruicaoTimer >= animDestruicaoDuracao) {
                estaAnimandoDestruicao = false;
                estado = EstadoDano::DESTRUIDO;
            }
        }
    }

    void carregarTexturas() {
        texturaIDInteiro = loadTexture(texturaNomeInteiro.c_str());
        texturaIDDanificado = loadTexture(texturaNomeDanificado.c_str());
        if (!meshes.empty() && texturaIDInteiro != 0) {
            meshes[0].material.textureID = texturaIDInteiro;
        }
    }

    void inicializarFisica(btDiscreteDynamicsWorld* mundoFisica, const btVector3& position, const btQuaternion& rotation = btQuaternion(0, 0, 0, 1)) {
        if (corpoRigido) limparFisica(mundoFisica);
        btCollisionShape* blockShape = new btBoxShape(dimensoes);
        btTransform startTransform; startTransform.setIdentity(); startTransform.setOrigin(position); startTransform.setRotation(rotation);
        btVector3 localInertia(0, 0, 0);
        if (massa != 0.f) blockShape->calculateLocalInertia(massa, localInertia);
        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(massa, myMotionState, blockShape, localInertia);
        corpoRigido = new btRigidBody(rbInfo);
        corpoRigido->setFriction(atrito);
        corpoRigido->setRestitution(restituicao);
        corpoRigido->setDamping(0.3f, 0.3f);
        corpoRigido->setCcdMotionThreshold(dimensoes.x());
        corpoRigido->setCcdSweptSphereRadius(dimensoes.x());
        mundoFisica->addRigidBody(corpoRigido);
    }

    void limparFisica(btDiscreteDynamicsWorld* mundoFisica) {
        if (corpoRigido) {
            if (mundoFisica) mundoFisica->removeRigidBody(corpoRigido);
            delete corpoRigido->getMotionState();
            delete corpoRigido->getCollisionShape();
            delete corpoRigido;
            corpoRigido = nullptr;
        }
    }

    void aplicarDano(float dano) {
        if (estado == EstadoDano::MORRENDO || estado == EstadoDano::DESTRUIDO) return;
        saudeAtual -= dano;
        if(dano >= 0.5f) g_audioManager.playColisao(tipoMaterial, 30);
        
        if (saudeAtual <= saudeTotal * 0.5f && estado == EstadoDano::INTEIRO) {
            estado = EstadoDano::DANIFICADO;
            if (!meshes.empty() && texturaIDDanificado != 0) {
                meshes[0].material.textureID = texturaIDDanificado;
                invalidateDisplayList(); 
            }
        }
        estaAnimandoDano = true; animDanoTimer = 0.0f;
        if (saudeAtual <= 0) {
            printf("Bloco destruido! +%d pontos!\n", pontuacaoValor);
            estado = EstadoDano::MORRENDO; estaAnimandoDestruicao = true; animDestruicaoTimer = 0.0f;
            if (corpoRigido) {
                btVector3 pos = corpoRigido->getCenterOfMassPosition();
                g_particleManager.createExplosion(pos, btVector3(corR, corG, corB));
                g_audioManager.playDestruction(tipoMaterial);
            }
        }
    }

    void desenhar() {
        if (estado == EstadoDano::DESTRUIDO) return; 
        if (!corpoRigido) return;
        btTransform trans; corpoRigido->getMotionState()->getWorldTransform(trans);
        btScalar m[16]; trans.getOpenGLMatrix(m);
        glPushMatrix();
        glMultMatrixf(m);
        if (estaAnimandoDano) glTranslatef(sin(animDanoTimer * 100.0f) * 0.05f, 0, 0);
        if (estaAnimandoDestruicao) { float t = (animDestruicaoTimer / animDestruicaoDuracao); float s = 1.0f - t; glScalef(s, s, s); }
        
        // --- MUDANÇA 3: Aplica a escala não-uniforme ---
        // Isso garante que o modelo (independente de ser cubo, barra ou placa)
        // seja esticado para preencher exatamente a caixa de colisão física.
        glScalef(scaleX, scaleY, scaleZ);
        
        OBJModel::draw();
        glPopMatrix();
    }

    MaterialTipo getTipo(){ return this->tipoMaterial; }
    btRigidBody* getRigidBody() { return corpoRigido; }
    bool isDestruido() const { return estado == EstadoDano::DESTRUIDO; }
    int getPontuacao() const { return pontuacaoValor; }
};