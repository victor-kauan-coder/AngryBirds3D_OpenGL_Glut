#pragma once

#include <vector>
#include <btBulletDynamicsCommon.h>
#include <GL/glut.h>

// Para rand() e RAND_MAX
#include <cstdlib> 
#include <cmath>

enum ParticleType {
    PT_POINT,
    PT_TRIANGLE
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

    Particle() : life(0.0f), active(false), type(PT_POINT) {}
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
    void spawn(const btVector3& pos, const btVector3& vel, const btVector3& color, float life, ParticleType type = PT_POINT) {
        for (int i = 0; i < maxParticles; i++) {
            if (!particles[i].active) {
                particles[i].active = true;
                particles[i].pos = pos;
                particles[i].vel = vel;
                particles[i].color = color;
                particles[i].life = life;
                particles[i].type = type;
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
     * @brief Atualiza a física de todas as partículas ativas.
     */
    void update(float deltaTime) {
        for (int i = 0; i < maxParticles; i++) {
            if (particles[i].active) {
                // Aplica gravidade
                particles[i].vel.setY(particles[i].vel.y() - gravity * deltaTime);
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

        glEnable(GL_LIGHTING);
    }
};