#pragma once

#include <vector>
#include <btBulletDynamicsCommon.h>
#include <GL/glut.h>

// Para rand() e RAND_MAX
#include <cstdlib> 
#include <cmath>

#include <string> // Para std::to_string

enum ParticleType {
    PT_POINT,
    PT_TRIANGLE,
    PT_SMOKE, // Novo tipo para fumaça estilo Minecraft
    PT_TEXT   // Novo tipo para texto flutuante
};

/**
 * @struct Particle
 * @brief Armazena o estado de uma única partícula.
 */
struct Particle {
    btVector3 pos;      // Posição
    btVector3 vel;      // Velocidade
    btVector3 color;    // Cor
    float life;         // Tempo de vida restante (em segundos)
    bool active;        // Está ativa?
    ParticleType type;
    int scoreValue;     // Valor dos pontos (para PT_TEXT)

    Particle() : life(0.0f), active(false), type(PT_POINT), scoreValue(0) {}
};

/**
 * @class ParticleManager
 * @brief Gerencia um pool de partículas para explosões.
 */
class ParticleManager {
private:
    std::vector<Particle> particles; // Nosso pool de partículas
    int maxParticles;
    float gravity;

    // Retorna um float aleatório entre -1.0 e 1.0
    float randFloat() {
        return (float(rand()) / (float(RAND_MAX) / 2.0f)) - 1.0f;
    }

public:
    ParticleManager(int maxP = 1000) : maxParticles(maxP), gravity(9.81f) {
        // Pré-aloca o pool de partículas
        particles.resize(maxParticles);
    }

    /**
     * @brief Encontra uma partícula inativa e a "acorda".
     */
    void spawn(const btVector3& pos, const btVector3& vel, const btVector3& color, float life, ParticleType type = PT_POINT, int score = 0) {
        for (int i = 0; i < maxParticles; i++) {
            if (!particles[i].active) {
                particles[i].active = true;
                particles[i].pos = pos;
                particles[i].vel = vel;
                particles[i].color = color;
                particles[i].life = life;
                particles[i].type = type;
                particles[i].scoreValue = score;
                return; // Sai após encontrar um slot
            }
        }
    }

    /**
     * @brief Cria uma explosão de N partículas em uma posição.
     */
    void createExplosion(const btVector3& pos, const btVector3& color, int count = 30) {
        for (int i = 0; i < count; i++) {
            // Velocidade aleatória para cima e para fora
            btVector3 vel(
                randFloat() * 5.0f,     // Velocidade X aleatória
                (randFloat() + 1.0f) * 4.0f, // Velocidade Y (sempre para cima)
                randFloat() * 5.0f      // Velocidade Z aleatória
            );
            
            // Tempo de vida aleatório (entre 0.5 e 1.5 segundos)
            float life = 0.5f + (float(rand()) / float(RAND_MAX));
            
            spawn(pos, vel, color, life);
        }
    }

    /**
     * @brief Cria uma explosão de fogo (triângulos vermelhos/amarelos) em todas as direções.
     * @param pos Posição da explosão
     * @param count Quantidade de partículas
     * @param radius Raio da explosão (influencia a velocidade inicial)
     */
    void createFireExplosion(const btVector3& pos, int count, float radius) {
        for (int i = 0; i < count; i++) {
            // Direção aleatória esférica
            float x = randFloat();
            float y = randFloat();
            float z = randFloat();
            btVector3 dir(x, y, z);
            if (dir.length2() > 0.0001f) {
                dir.normalize();
            } else {
                dir = btVector3(0, 1, 0);
            }
            
            // Velocidade baseada no raio
            float speed = (randFloat() + 1.5f) * radius; 
            btVector3 vel = dir * speed;

            // Cor: Vermelho (1,0,0) a Amarelo (1,1,0)
            // Green varia de 0.0 a 1.0
            float green = (randFloat() + 1.0f) / 2.0f; 
            btVector3 color(1.0f, green, 0.0f);

            // Vida
            float life = 0.5f + (float(rand()) / float(RAND_MAX));

            spawn(pos, vel, color, life, PT_TRIANGLE);
        }
    }

    /**
     * @brief Cria um efeito de fumaça (cubos brancos que sobem).
     */
    void createSmokeEffect(const btVector3& pos, int count = 20) {
        for (int i = 0; i < count; i++) {
            // Velocidade: Sobe (Y positivo) e espalha pouco em X/Z
            btVector3 vel(
                randFloat() * 1.0f,     // Espalha pouco em X
                (randFloat() + 2.0f) * 1.5f, // Sobe com velocidade constante
                randFloat() * 1.0f      // Espalha pouco em Z
            );
            
            // Cor: Branco ou Cinza muito claro
            float shade = 0.8f + (float(rand()) / float(RAND_MAX)) * 0.2f;
            btVector3 color(shade, shade, shade);

            // Vida mais longa para subir bastante
            float life = 0.5f + (float(rand()) / float(RAND_MAX));
            
            spawn(pos, vel, color, life, PT_SMOKE);
        }
    }

    /**
     * @brief Cria um texto flutuante com pontuação.
     */
    void createScorePopup(const btVector3& pos, int points) {
        // Velocidade: Sobe suavemente
        btVector3 vel(0.0f, 2.0f, 0.0f);
        
        // Cor base (será sobrescrita pelo arco-íris no draw)
        btVector3 color(1.0f, 1.0f, 1.0f);

        // Vida de 2 segundos
        float life = 2.0f;
        
        spawn(pos, vel, color, life, PT_TEXT, points);
    }

    /**
     * @brief Atualiza a física de todas as partículas ativas.
     */
    void update(float deltaTime) {
        for (int i = 0; i < maxParticles; i++) {
            if (particles[i].active) {
                // Aplica gravidade (EXCETO para fumaça e texto)
                if (particles[i].type != PT_SMOKE && particles[i].type != PT_TEXT) {
                    particles[i].vel.setY(particles[i].vel.y() - gravity * deltaTime);
                } else if (particles[i].type == PT_SMOKE) {
                    // Fumaça desacelera um pouco ao subir (drag), mas não cai
                    particles[i].vel *= 0.98f; 
                } else if (particles[i].type == PT_TEXT) {
                    // Texto desacelera suavemente
                    particles[i].vel *= 0.95f;
                }

                // Move a partícula
                particles[i].pos += particles[i].vel * deltaTime;
                // Reduz a vida
                particles[i].life -= deltaTime;

                // Desativa se a vida acabar ou se cair pelo chão
                if (particles[i].life <= 0.0f || particles[i].pos.y() < -1.0f) {
                    particles[i].active = false;
                }
            }
        }
    }

    /**
     * @brief Desenha todas as partículas ativas.
     */
    void draw() {
        // Desliga a iluminação para as partículas (para que a cor apareça pura)
        glDisable(GL_LIGHTING);
        
        // Desenha pontos
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        for (int i = 0; i < maxParticles; i++) {
            if (particles[i].active && particles[i].type == PT_POINT) {
                glColor3f(particles[i].color.x(), particles[i].color.y(), particles[i].color.z());
                glVertex3f(particles[i].pos.x(), particles[i].pos.y(), particles[i].pos.z());
            }
        }
        glEnd();

        // Desenha triângulos (fogo)
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < maxParticles; i++) {
            if (particles[i].active && particles[i].type == PT_TRIANGLE) {
                glColor3f(particles[i].color.x(), particles[i].color.y(), particles[i].color.z());
                
                btVector3 p = particles[i].pos;
                float s = 0.2f; // Tamanho do triângulo

                // Triângulo simples billboard-ish (não rotaciona com a câmera, mas serve para o efeito)
                glVertex3f(p.x(), p.y() + s, p.z());
                glVertex3f(p.x() - s, p.y() - s, p.z());
                glVertex3f(p.x() + s, p.y() - s, p.z());
            }
        }
        glEnd();

        // Desenha fumaça (cubos estilo Minecraft)
        for (int i = 0; i < maxParticles; i++) {
            if (particles[i].active && particles[i].type == PT_SMOKE) {
                glColor3f(particles[i].color.x(), particles[i].color.y(), particles[i].color.z());
                
                glPushMatrix();
                glTranslatef(particles[i].pos.x(), particles[i].pos.y(), particles[i].pos.z());
                
                // Aumenta o tamanho conforme a vida diminui (expansão da fumaça)
                // life começa em ~2.0. Vamos inverter para escala.
                float scale = 0.5f + (2.0f - particles[i].life) * 0.3f; // Reduzido escala inicial e fator de crescimento
                glScalef(scale, scale, scale);
                
                glutSolidCube(0.3f); // Reduzido tamanho base do cubo de 0.5 para 0.3
                glPopMatrix();
            }
        }

        // Desenha texto flutuante (Pontos)
        for (int i = 0; i < maxParticles; i++) {
            if (particles[i].active && particles[i].type == PT_TEXT) {
                glPushMatrix();
                glTranslatef(particles[i].pos.x(), particles[i].pos.y(), particles[i].pos.z());
                
                // Define a cor baseada no valor (Porco >= 5000)
                float r, g, b;
                if (particles[i].scoreValue >= 5000) {
                    r = 0.2f; g = 1.0f; b = 0.2f; // Verde
                } else {
                    r = 1.0f; g = 1.0f; b = 1.0f; // Branco
                }

                // Escala fixa
                float scale = 0.01f; 
                glScalef(scale, scale, scale);

                // Centraliza o texto
                std::string text = "+" + std::to_string(particles[i].scoreValue);
                float textWidth = 0;
                for (char c : text) {
                    textWidth += glutStrokeWidth(GLUT_STROKE_ROMAN, c);
                }
                glTranslatef(-textWidth / 2.0f, 0.0f, 0.0f);

                // Desabilita teste de profundidade para garantir que o texto fique visível e o preenchimento sobreponha o contorno
                glDisable(GL_DEPTH_TEST);

                // 1. Desenha o Contorno (Preto e Grosso)
                glColor3f(0.0f, 0.0f, 0.0f);
                glLineWidth(5.0f);
                glPushMatrix(); // Salva posição inicial do texto
                for (char c : text) {
                    glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
                }
                glPopMatrix(); // Restaura posição inicial do texto

                // 2. Desenha o Preenchimento (Cor e Fino)
                glColor3f(r, g, b);
                glLineWidth(2.0f);
                for (char c : text) {
                    glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
                }
                
                // Restaura configurações
                glLineWidth(1.0f);
                glEnable(GL_DEPTH_TEST);
                
                glPopMatrix();
            }
        }

        glEnable(GL_LIGHTING);
    }
};