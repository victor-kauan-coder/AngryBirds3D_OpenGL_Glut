#pragma once

// Includes do OpenGL/GLUT
// (Necessários para tipos como GLuint, GLfloat, gl... , glu...)
#include <GL/glut.h> 

// Includes da Biblioteca Padrão do C++
#include <vector>       // Para std::vector (listas dinâmicas)
#include <string>       // Para std::string (manipulação de texto)
#include <fstream>      // Para std::ifstream (leitura de arquivos)
#include <sstream>      // Para std::istringstream (processamento de strings)
#include <cfloat>       // Para FLT_MAX (usado na normalização)
#include <cstdio>       // Para sscanf (usado no parser de faces "f v/vt/vn")
#include <algorithm>    // Para std::max (usado na normalização)
#include <map>          // Para std::map (crucial para o re-mapeamento de vértices)

// --- Carregador de Imagem STB ---
// Define a implementação do STB *antes* de incluir o header.
// Isso faz com que este header inclua também o código-fonte (.cpp)
// Lembre-se de baixar "stb_image.h" e colocá-lo na mesma pasta do projeto.
#include "stb_image.h"

/**
 * @struct Material
 * @brief Armazena as propriedades de um material (cor, brilho, textura)
 * lido de um arquivo .MTL
 */
struct Material {
    // --- Propriedades do Material ---
    std::string name; // Nome do material (ex: "CorpoVermelho")
    float ambient[3] = { 0.2f, 0.2f, 0.2f };  // Cor ambiente (Ka)
    float diffuse[3] = { 0.8f, 0.8f, 0.8f };  // Cor difusa (Kd) - a cor "principal"
    float specular[3] = { 0.0f, 0.0f, 0.0f }; // Cor especular (Ks) - cor do brilho
    float shininess = 0.0f;                  // Brilho (Ns)

    // --- Propriedades da Textura ---
    std::string textureFilename; // Nome do arquivo de textura (map_Kd)
    GLuint textureID = 0;        // ID da textura no OpenGL (0 = sem textura)

    /**
     * @brief Aplica este material ao pipeline do OpenGL
     */
    void apply() {
        // Converte os arrays de float para o formato GLfloat
        GLfloat Ka[] = { ambient[0], ambient[1], ambient[2], 1.0f };
        GLfloat Kd[] = { diffuse[0], diffuse[1], diffuse[2], 1.0f };
        GLfloat Ks[] = { specular[0], specular[1], specular[2], 1.0f };

        // --- CORREÇÃO DA COR DA TEXTURA ---
        // Se este material tiver uma textura...
        if (textureID != 0) {
            // ...forçamos a cor difusa (Kd) para BRANCO PURO (1,1,1).
            // Isso impede que a cor difusa do material (ex: cinza 0.8)
            // "manche" ou "escureça" a textura.
            // A conta fica: (Cor da Textura) * (1,1,1) = (Cor da Textura Original)
            Kd[0] = 1.0f;
            Kd[1] = 1.0f;
            Kd[2] = 1.0f;
        }
        // Se não tiver textura, ele usará a cor 'diffuse' lida do arquivo.
        // --- FIM DA CORREÇÃO ---

        // Envia as propriedades do material para o OpenGL
        glMaterialfv(GL_FRONT, GL_AMBIENT, Ka);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, Kd);
        glMaterialfv(GL_FRONT, GL_SPECULAR, Ks);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess);

        // Se tivermos uma textura, ativa e vincula (bind) ela
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
        } else {
            // Garante que a texturização esteja desligada se este material não a usar
            glDisable(GL_TEXTURE_2D);
        }
    }
};

/**
 * @struct Mesh
 * @brief Representa um sub-grupo de um OBJModel.
 * Um modelo OBJ (como o pássaro) é composto de várias "meshes",
 * cada uma com seu próprio material (ex: corpo, bico, sombrancelhas).
 */
struct Mesh {
    Material material; // O material que esta mesh usa
    std::vector<unsigned int> indices; // A lista de índices (triângulos) SÓ DESTA MESH
};

/**
 * @struct OBJModel
 * @brief Armazena a geometria completa de um modelo .obj, incluindo
 * múltiplos materiais e meshes.
 */
struct OBJModel {
    // --- Arrays Unificados de Geometria ---
    // Todos os vértices, normais e coordenadas de textura
    // de todas as meshes são armazenados juntos aqui.
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    
    // --- Estrutura de Materiais e Meshes ---
    // Um "mapa" para encontrar um Material pelo seu nome
    std::map<std::string, Material> materials;
    // A lista de sub-meshes que compõem este modelo
    std::vector<Mesh> meshes;

    /**
     * @brief Carrega um arquivo de imagem (PNG, JPG, etc.) e o envia para a GPU,
     * retornando o ID da textura no OpenGL.
     */
    GLuint loadTexture(const char* filename) {
        int width, height, nrChannels;
        // Inverte a imagem no eixo Y, pois o OpenGL (0,0) é no canto inferior esquerdo
        // e as imagens (0,0) são no canto superior esquerdo.
        stbi_set_flip_vertically_on_load(true); 
        
        // Carrega os dados da imagem do arquivo

        unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
        if (!data) {
            printf("Aviso: Nao foi possivel carregar a textura %s\n", filename);
            return 0; // Retorna 0 (ID de textura inválido)
        }

        // Determina o formato (RGB ou RGBA)
        GLenum format = GL_RGB;
        if (nrChannels == 4) {
            format = GL_RGBA;
        }

        GLuint textureID;
        glGenTextures(1, &textureID); // 1. Gera um ID
        glBindTexture(GL_TEXTURE_2D, textureID); // 2. "Ativa" o ID

        // 3. Define os parâmetros da textura
        // GL_REPEAT faz a textura se repetir se o modelo for maior que ela
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // GL_LINEAR faz a filtragem (suavização) da textura
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // 4. Envia os dados da imagem (data) para a GPU
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        // 5. Libera a memória da CPU, pois os dados já estão na GPU
        stbi_image_free(data);

        return textureID; // Retorna o ID da textura na GPU
    }

    /**
     * @brief Carrega e processa um arquivo de material (.mtl)
     */
    bool loadMTL(const char* filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            printf("Aviso: Arquivo MTL %s nao encontrado\n", filename);
            return false;
        }

        Material currentMat; // Um material temporário
        std::string currentMatName;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "newmtl") {
                // Se já tínhamos um material sendo lido, salva ele no map
                if (!currentMatName.empty()) {
                    materials[currentMatName] = currentMat;
                }
                // Começa um novo material
                iss >> currentMatName;
                currentMat = Material(); // Reseta para o padrão
                currentMat.name = currentMatName;
                printf("  Material: %s\n", currentMatName.c_str());
            }
            // Lê as propriedades do material atual
            else if (prefix == "Ka") { // Ambiente
                iss >> currentMat.ambient[0] >> currentMat.ambient[1] >> currentMat.ambient[2];
            }
            else if (prefix == "Kd") { // Difusa
                iss >> currentMat.diffuse[0] >> currentMat.diffuse[1] >> currentMat.diffuse[2];
            }
            else if (prefix == "Ks") { // Especular
                iss >> currentMat.specular[0] >> currentMat.specular[1] >> currentMat.specular[2];
            }
            else if (prefix == "Ns") { // Brilho
                iss >> currentMat.shininess;
            }
            else if (prefix == "map_Kd") { // Mapa de Textura
                iss >> currentMat.textureFilename;
            }
        }
        // Salva o último material que foi lido
        if (!currentMatName.empty()) {
            materials[currentMatName] = currentMat;
        }
        file.close();

        printf("  Carregados %zu materiais do MTL\n", materials.size());

        // --- CARREGAMENTO DE TEXTURA ---
        // Agora que lemos todos os materiais, iteramos pelo 'map'
        // e carregamos as texturas de verdade.
        printf("Carregando texturas de material...\n");
        // 'auto& pair' itera por cada par (chave, valor) no 'map'
        for (auto& pair : materials) {
            Material& mat = pair.second; // Pega o material do par
            if (!mat.textureFilename.empty()) {
                // Tenta encontrar o arquivo de textura em vários caminhos
                std::string path1 = std::string("../") + mat.textureFilename; // NOVO CAMINHO CORRETO!
                std::string path2 = mat.textureFilename; // Fallback
                std::string path3 = std::string("Objetos/") + mat.textureFilename; // Legado
                std::string path4 = std::string("../Objetos/") + mat.textureFilename; // Legado

                // O array agora armazena ponteiros para strings que
                // GARANTIDAMENTE existem durante o loop 'for'.
                const char* texPaths[] = {
                    path1.c_str(), // Tenta "../texturas/arquivo.png" PRIMEIRO
                    path2.c_str(),
                    path3.c_str(),
                    path4.c_str()
                };
                bool texLoaded = false;
                for (int i = 0; i < 4; i++) {
                    mat.textureID = loadTexture(texPaths[i]); // Chama nossa função
                    if (mat.textureID != 0) { // 0 = falha
                        printf("  Textura %s carregada (ID: %u)\n", texPaths[i], mat.textureID);
                        texLoaded = true;
                        break;
                    }
                }
                if (!texLoaded) {
                     printf("  Aviso: Textura %s nao encontrada.\n", mat.textureFilename.c_str());
                }
            }
        }
        return true;
    }

    /**
     * @brief Carrega um arquivo de modelo (.obj)
     */
    bool loadFromFile(const char* filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            printf("Erro: Nao foi possivel abrir arquivo %s\n", filename);
            return false;
        }

        // --- Buffers Temporários ---
        // Armazenam os dados brutos lidos do OBJ
        std::vector<float> temp_vertices;
        std::vector<float> temp_normals;
        std::vector<float> temp_texCoords;
        
        // --- Re-indexação ---
        // Um mapa para rastrear combinações únicas de "v/vt/vn"
        // Chave: "1/1/1", Valor: 0
        // Chave: "2/1/1", Valor: 1
        std::map<std::string, unsigned int> vertexIndexMap;
        unsigned int nextIndex = 0; // O próximo índice a ser criado
        
        // A mesh (grupo) atual que estamos preenchendo
        Mesh currentMesh;
        Material defaultMaterial; // Um material cinza padrão, caso o .mtl falhe
        currentMesh.material = defaultMaterial;

        std::string mtlFilename;
        //NOVOO
        std::string objPath = filename;
        size_t lastSlash = objPath.find_last_of("/\\");
        std::string objDir = "";
        if (lastSlash != std::string::npos) {
            objDir = objPath.substr(0, lastSlash + 1); // ex: "Objetos/"
        }
        // Bounding box (para normalização)
        float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
        float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

        std::string line;
        // Lê o arquivo .obj linha por linha
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix; // Pega a primeira palavra (v, vn, vt, f, usemtl)

            if (prefix == "mtllib") {
                // Carrega o arquivo de material
                iss >> mtlFilename;
                printf("Arquivo MTL referenciado: %s\n", mtlFilename.c_str());
               // --- CORRIGIDO: Caminhos relativos ao OBJ ---
                // Substitua o bloco mtlPaths[] antigo por este
                const char* mtlPaths[] = {
                    (objDir + mtlFilename).c_str(), // Tenta carregar (ex: "Objetos/arvore2.mtl")
                    mtlFilename.c_str()             // Fallback: (ex: "arvore2.mtl")
                };
                for (int i = 0; i < 4; i++) {
                    if (loadMTL(mtlPaths[i])) break; // Carrega e para no primeiro sucesso
                }
            }
            else if (prefix == "usemtl") {
                // --- Troca de Material (Início de uma nova Mesh) ---
                // Se a mesh atual já tem triângulos (índices),
                // salva ela na nossa lista e começa uma nova.
                if (!currentMesh.indices.empty()) {
                    meshes.push_back(currentMesh);
                }
                
                // Começa a nova mesh
                std::string matName;
                iss >> matName;
                currentMesh = Mesh(); // Reseta
                
                // Tenta encontrar o material no map que carregamos
                if (materials.count(matName)) {
                    currentMesh.material = materials[matName]; // Encontrou!
                    printf("Usando material: %s\n", matName.c_str());
                } else {
                    printf("Aviso: Material %s nao encontrado. Usando padrao.\n", matName.c_str());
                    currentMesh.material = defaultMaterial; // Não encontrou, usa o cinza
                }
            }
            else if (prefix == "v") {
                // Lê um vértice (posição)
                float x, y, z;
                iss >> x >> y >> z;
                temp_vertices.push_back(x);
                temp_vertices.push_back(y);
                temp_vertices.push_back(z);
                // Atualiza o bounding box
                if (x < minX) minX = x; if (x > maxX) maxX = x;
                if (y < minY) minY = y; if (y > maxY) maxY = y;
                if (z < minZ) minZ = z; if (z > maxZ) maxZ = z;
            }
            else if (prefix == "vn") {
                // Lê uma normal
                float nx, ny, nz;
                iss >> nx >> ny >> nz;
                temp_normals.push_back(nx);
                temp_normals.push_back(ny);
                temp_normals.push_back(nz);
            }
            else if (prefix == "vt") {
                // Lê uma coordenada de textura
                float u, v;
                iss >> u >> v;
                temp_texCoords.push_back(u);
                temp_texCoords.push_back(v);
            }
            else if (prefix == "f") {
                // --- Processamento de Face (Triângulo) ---
                // Itera 3 vezes (para os 3 vértices do triângulo)
                for (int i = 0; i < 3; i++) {
                    std::string faceVertexStr;
                    iss >> faceVertexStr; // Pega a string (ex: "1/1/1" ou "2//3")

                    // --- Lógica de Re-indexação ---
                    // Verifica se já processamos esta combinação "v/vt/vn"
                    if (vertexIndexMap.count(faceVertexStr) == 0) {
                        // NÃO, é um vértice único.
                        // Precisamos adicioná-lo aos nossos arrays FINAIS
                        // e salvar seu novo índice.
                        
                        // 1. Salva o novo índice (que é o tamanho atual do array)
                        vertexIndexMap[faceVertexStr] = nextIndex;
                        
                        unsigned int v_idx = 0, vt_idx = 0, vn_idx = 0;
                        
                        // 2. Parseia os índices do OBJ (v/vt/vn)
                        // (Nota: sscanf é uma função C antiga, mas ótima para isso)
                        int matches = sscanf(faceVertexStr.c_str(), "%u/%u/%u", &v_idx, &vt_idx, &vn_idx);
                        if (matches != 3) {
                            matches = sscanf(faceVertexStr.c_str(), "%u//%u", &v_idx, &vn_idx);
                            if (matches != 2) {
                                matches = sscanf(faceVertexStr.c_str(), "%u/%u", &v_idx, &vt_idx);
                                if (matches != 2) {
                                    sscanf(faceVertexStr.c_str(), "%u", &v_idx);
                                }
                            }
                        }

                        // 3. Adiciona os dados aos arrays FINAIS
                        //    (Converte de 1-based (OBJ) para 0-based (C++))
                        vertices.push_back(temp_vertices[(v_idx - 1) * 3 + 0]);
                        vertices.push_back(temp_vertices[(v_idx - 1) * 3 + 1]);
                        vertices.push_back(temp_vertices[(v_idx - 1) * 3 + 2]);

                        if (vt_idx > 0) { // Se tiver coordenada de textura
                            texCoords.push_back(temp_texCoords[(vt_idx - 1) * 2 + 0]);
                            texCoords.push_back(temp_texCoords[(vt_idx - 1) * 2 + 1]);
                        } else {
                            texCoords.push_back(0); // Coordenada de textura padrão
                            texCoords.push_back(0);
                        }
                        
                        if (vn_idx > 0) { // Se tiver normal
                            normals.push_back(temp_normals[(vn_idx - 1) * 3 + 0]);
                            normals.push_back(temp_normals[(vn_idx - 1) * 3 + 1]);
                            normals.push_back(temp_normals[(vn_idx - 1) * 3 + 2]);
                        } else {
                            normals.push_back(0); // Normal padrão (apontando para cima)
                            normals.push_back(1);
                            normals.push_back(0);
                        }
                        
                        nextIndex++; // Incrementa o índice para o próximo novo vértice
                    }
                    
                    // 4. Adiciona o índice (seja ele novo ou antigo)
                    //    à lista de índices da MESH ATUAL.
                    currentMesh.indices.push_back(vertexIndexMap[faceVertexStr]);
                }
            }
        }
        // Salva a última mesh que estava sendo processada
        if (!currentMesh.indices.empty()) {
            meshes.push_back(currentMesh);
        }
        file.close();

        // --- Normalização (Centralizar e Redimensionar o Modelo) ---
        float sizeX = maxX - minX;
        float sizeY = maxY - minY;
        float sizeZ = maxZ - minZ;
        float maxSize = std::max(std::max(sizeX, sizeY), sizeZ);

        float normalizeScale = 0.6f / maxSize; // Escala para caber em 0.6 unidades
        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        float centerZ = (minZ + maxZ) / 2.0f;

        printf("Modelo original: tamanho %.2f x %.2f x %.2f\n", sizeX, sizeY, sizeZ);
        printf("Normalizando com escala: %.4f\n", normalizeScale);

        // Aplica a normalização iterando por todos os vértices FINAIS
        for (size_t i = 0; i < vertices.size(); i += 3) {
            vertices[i] = (vertices[i] - centerX) * normalizeScale;
            vertices[i+1] = (vertices[i+1] - minY) * normalizeScale;
            vertices[i+2] = (vertices[i+2] - centerZ) * normalizeScale;
        }

        printf("Modelo OBJ carregado: %zu meshes, %zu vertices unicos\n",
            meshes.size(), vertices.size() / 3);
        return true;
    }

    /**
     * @brief Desenha o modelo completo (todas as suas meshes).
     */
    void draw() {
        // Se não houver o que desenhar, sai
        if (meshes.empty() || vertices.empty()) return;

        // --- CORREÇÃO (Pássaro vs. Árvore) ---
        // Verificamos se este modelo realmente carregou materiais de um arquivo .mtl
        bool has_real_materials = !materials.empty();

        if (has_real_materials) {
            // Se SIM (é o pássaro), desabilitamos GL_COLOR_MATERIAL.
            // Isso força o OpenGL a usar os materiais do .mtl (com nossa
            // correção de cor branca na função apply()).
            glDisable(GL_COLOR_MATERIAL);
        }
        // Se NÃO (é a árvore), deixamos GL_COLOR_MATERIAL LIGADO.
        // Isso permite que o glColor3f(verde) da 'Tree::draw()' funcione.
        // --- FIM DA CORREÇÃO ---

        // Ativa os arrays de dados (eles são os mesmos para todas as meshes)
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, vertices.data());

        if (!normals.empty()) {
            glEnableClientState(GL_NORMAL_ARRAY);
            glNormalPointer(GL_FLOAT, 0, normals.data());
        }

        if (!texCoords.empty()) {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, 0, texCoords.data());
        }

        // --- LÓGICA DE DESENHO (Multi-Material) ---
        // Itera sobre cada sub-mesh
        for (auto& mesh : meshes) {
            
            // Se o modelo tiver materiais (é o pássaro)...
            if (has_real_materials) {
                // 1. Aplica o material específico desta mesh
                //    (A função apply() já cuida da textura e da cor branca)
                mesh.material.apply();
            }
            // Se não tiver materiais (é a árvore), pulamos o apply()
            // e o OpenGL usará o glColor3f(verde) que foi definido antes.

            // 2. Desenha apenas os triângulos (índices) desta mesh
            glDrawElements(GL_TRIANGLES, 
                           mesh.indices.size(), 
                           GL_UNSIGNED_INT, 
                           mesh.indices.data());
            
            // 3. Desliga a textura (se o material a usou)
            //    para que a próxima mesh (se não tiver textura) não a use.
            if (has_real_materials && mesh.material.textureID != 0) {
                glDisable(GL_TEXTURE_2D);
            }
        }
        // --- FIM DA LÓGICA DE DESENHO ---

        // Desativa todos os arrays
        glDisableClientState(GL_VERTEX_ARRAY);
        if (!normals.empty()) {
            glDisableClientState(GL_NORMAL_ARRAY);
        }
        if (!texCoords.empty()) {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }

        // --- CORREÇÃO (Pássaro vs. Árvore) ---
        // Se nós desligamos o GL_COLOR_MATERIAL (para o pássaro),
        // devemos ligá-lo de volta agora para o resto da cena.
        if (has_real_materials) {
            glEnable(GL_COLOR_MATERIAL);
        }
        // --- FIM DA CORREÇÃO ---
    }
};