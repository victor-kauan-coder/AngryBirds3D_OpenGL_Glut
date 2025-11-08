#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cfloat>



struct Material {
    std::string name;
    float ambient[3] = { 0.2f, 0.2f, 0.2f };
    float diffuse[3] = { 0.8f, 0.8f, 0.8f };
    float specular[3] = { 0.0f, 0.0f, 0.0f };
    float shininess = 0.0f;

    void apply() {
        GLfloat Ka[] = { ambient[0], ambient[1], ambient[2], 1.0f };
        GLfloat Kd[] = { diffuse[0], diffuse[1], diffuse[2], 1.0f };
        GLfloat Ks[] = { specular[0], specular[1], specular[2], 1.0f };

        glMaterialfv(GL_FRONT, GL_AMBIENT, Ka);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, Kd);
        glMaterialfv(GL_FRONT, GL_SPECULAR, Ks);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess);
    }
};

struct OBJModel {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned int> indices;
    std::vector<Material> materials;
    Material currentMaterial;
    bool hasMaterial = false;

    bool loadMTL(const char* filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            printf("Aviso: Arquivo MTL %s nao encontrado\n", filename);
            return false;
        }

        Material* currentMat = nullptr;
        std::string line;

        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "newmtl") {
                materials.push_back(Material());
                currentMat = &materials.back();
                iss >> currentMat->name;
                printf("  Material: %s\n", currentMat->name.c_str());
            }
            else if (currentMat) {
                if (prefix == "Ka") {
                    iss >> currentMat->ambient[0] >> currentMat->ambient[1] >> currentMat->ambient[2];
                }
                else if (prefix == "Kd") {
                    iss >> currentMat->diffuse[0] >> currentMat->diffuse[1] >> currentMat->diffuse[2];
                }
                else if (prefix == "Ks") {
                    iss >> currentMat->specular[0] >> currentMat->specular[1] >> currentMat->specular[2];
                }
                else if (prefix == "Ns") {
                    iss >> currentMat->shininess;
                }
            }
        }

        file.close();

        if (!materials.empty()) {
            currentMaterial = materials[0];
            hasMaterial = true;
            printf("  Carregados %zu materiais do MTL\n", materials.size());
        }

        return true;
    }

    bool loadFromFile(const char* filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            printf("Erro: Nao foi possivel abrir arquivo %s\n", filename);
            return false;
        }

        std::vector<float> temp_vertices;
        std::vector<float> temp_normals;
        std::vector<unsigned int> vertex_indices, normal_indices;

        std::string mtlFilename;

        float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
        float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "mtllib") {
                iss >> mtlFilename;
                printf("Arquivo MTL referenciado: %s\n", mtlFilename.c_str());

                const char* mtlPaths[] = {
                    (std::string("Objetos/") + mtlFilename).c_str(),
                    (std::string("./Objetos/") + mtlFilename).c_str(),
                    (std::string("../Objetos/") + mtlFilename).c_str(),
                    mtlFilename.c_str()
                };

                bool mtlLoaded = false;
                for (int i = 0; i < 4; i++) {
                    if (loadMTL(mtlPaths[i])) {
                        mtlLoaded = true;
                        break;
                    }
                }

                if (!mtlLoaded) {
                    loadMTL(mtlFilename.c_str());
                }
            }
            else if (prefix == "usemtl") {
                std::string matName;
                iss >> matName;
                for (auto& mat : materials) {
                    if (mat.name == matName) {
                        currentMaterial = mat;
                        printf("Usando material: %s\n", matName.c_str());
                        break;
                    }
                }
            }
            else if (prefix == "v") {
                float x, y, z;
                iss >> x >> y >> z;
                temp_vertices.push_back(x);
                temp_vertices.push_back(y);
                temp_vertices.push_back(z);

                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
                if (z < minZ) minZ = z;
                if (z > maxZ) maxZ = z;
            }
            else if (prefix == "vn") {
                float nx, ny, nz;
                iss >> nx >> ny >> nz;
                temp_normals.push_back(nx);
                temp_normals.push_back(ny);
                temp_normals.push_back(nz);
            }
            else if (prefix == "f") {
                std::string vertex1, vertex2, vertex3;
                iss >> vertex1 >> vertex2 >> vertex3;

                auto parseFaceVertex = [](const std::string& str, unsigned int& v_idx, unsigned int& n_idx) {
                    size_t slash1 = str.find('/');
                    if (slash1 == std::string::npos) {
                        v_idx = std::stoi(str);
                        n_idx = 0;
                    }
                    else {
                        v_idx = std::stoi(str.substr(0, slash1));
                        size_t slash2 = str.find('/', slash1 + 1);
                        if (slash2 != std::string::npos && slash2 > slash1 + 1) {
                            n_idx = std::stoi(str.substr(slash2 + 1));
                        }
                        else if (slash2 != std::string::npos) {
                            n_idx = std::stoi(str.substr(slash2 + 1));
                        }
                        else {
                            n_idx = 0;
                        }
                    }
                    };

                unsigned int v1, v2, v3, n1, n2, n3;
                parseFaceVertex(vertex1, v1, n1);
                parseFaceVertex(vertex2, v2, n2);
                parseFaceVertex(vertex3, v3, n3);

                vertex_indices.push_back(v1 - 1);
                vertex_indices.push_back(v2 - 1);
                vertex_indices.push_back(v3 - 1);

                if (n1 > 0) normal_indices.push_back(n1 - 1);
                if (n2 > 0) normal_indices.push_back(n2 - 1);
                if (n3 > 0) normal_indices.push_back(n3 - 1);
            }
        }

        file.close();

        float sizeX = maxX - minX;
        float sizeY = maxY - minY;
        float sizeZ = maxZ - minZ;
        float maxSize = std::max(std::max(sizeX, sizeY), sizeZ);

        float normalizeScale = 0.6f / maxSize;
        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        float centerZ = (minZ + maxZ) / 2.0f;

        printf("Modelo original: tamanho %.2f x %.2f x %.2f\n", sizeX, sizeY, sizeZ);
        printf("Normalizando com escala: %.4f\n", normalizeScale);

        for (size_t i = 0; i < temp_vertices.size(); i += 3) {
            temp_vertices[i] = (temp_vertices[i] - centerX) * normalizeScale;
            temp_vertices[i + 1] = (temp_vertices[i + 1] - centerY) * normalizeScale;
            temp_vertices[i + 2] = (temp_vertices[i + 2] - centerZ) * normalizeScale;
        }

        for (size_t i = 0; i < vertex_indices.size(); i++) {
            unsigned int v_idx = vertex_indices[i];
            vertices.push_back(temp_vertices[v_idx * 3]);
            vertices.push_back(temp_vertices[v_idx * 3 + 1]);
            vertices.push_back(temp_vertices[v_idx * 3 + 2]);

            if (i < normal_indices.size()) {
                unsigned int n_idx = normal_indices[i];
                normals.push_back(temp_normals[n_idx * 3]);
                normals.push_back(temp_normals[n_idx * 3 + 1]);
                normals.push_back(temp_normals[n_idx * 3 + 2]);
            }

            indices.push_back(i);
        }

        printf("Modelo OBJ carregado: %zu vertices, %zu triangulos\n",
            vertices.size() / 3, indices.size() / 3);
        return true;
    }

    void draw() {
        if (vertices.empty()) return;

        if (hasMaterial) {
            currentMaterial.apply();
        }

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, vertices.data());

        if (!normals.empty()) {
            glEnableClientState(GL_NORMAL_ARRAY);
            glNormalPointer(GL_FLOAT, 0, normals.data());
        }

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());

        glDisableClientState(GL_VERTEX_ARRAY);
        if (!normals.empty()) {
            glDisableClientState(GL_NORMAL_ARRAY);
        }
    }
};
