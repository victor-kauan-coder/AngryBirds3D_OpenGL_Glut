#pragma once

#include <GL/glut.h> 
#include <vector>       
#include <string>       
#include <fstream>      
#include <sstream>      
#include <cfloat>       
#include <cstdio>       
#include <algorithm>    
#include <map>          

#include "../stb/stb_image.h"


#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
struct Material {
    std::string name; 
    float ambient[3] = { 0.2f, 0.2f, 0.2f };
    float diffuse[3] = { 0.8f, 0.8f, 0.8f }; 
    float specular[3] = { 0.0f, 0.0f, 0.0f };
    float shininess = 0.0f;                  
    std::string textureFilename; 
    GLuint textureID = 0;        

    void apply() {
        GLfloat Ka[] = { ambient[0], ambient[1], ambient[2], 1.0f };
        GLfloat Kd[] = { diffuse[0], diffuse[1], diffuse[2], 1.0f };
        GLfloat Ks[] = { specular[0], specular[1], specular[2], 1.0f };

        if (textureID != 0) {
            Kd[0] = 1.0f; Kd[1] = 1.0f; Kd[2] = 1.0f;
        }

        glMaterialfv(GL_FRONT, GL_AMBIENT, Ka);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, Kd);
        glMaterialfv(GL_FRONT, GL_SPECULAR, Ks);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess);

        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
        } else {
            glDisable(GL_TEXTURE_2D);
        }
    }
};

struct Mesh {
    Material material; 
    std::vector<unsigned int> indices; 
};

struct OBJModel {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::map<std::string, Material> materials;
    std::vector<Mesh> meshes;


    GLuint displayListID = 0;
    bool needsUpdate = true;

    //destrutor 
    ~OBJModel() {
        if (displayListID != 0) {
            glDeleteLists(displayListID, 1);
        }
    }

    //usada para quando o bloco quebra e precisa atualizar a textura
    void invalidateDisplayList() {
        needsUpdate = true;
    }

    GLuint loadTexture(const char* filename) {
        int width, height, nrChannels;
        
        //tentando carregar 
        stbi_set_flip_vertically_on_load(true); 
        unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
        
        if (!data) {
            //caso falhe
            return 0; 
        }

        GLenum format = GL_RGB;
        if (nrChannels == 4) format = GL_RGBA;

        GLuint textureID;
        glGenTextures(1, &textureID); 
        glBindTexture(GL_TEXTURE_2D, textureID); 

        // GL_NEAREST = Mantém os pixels pixelados
        // GL_CLAMP_TO_EDGE = Impede bordas misturadas
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        
        stbi_image_free(data);
        return textureID; 
    }

    bool loadMTL(const char* filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;

        Material currentMat; 
        std::string currentMatName;
        std::string line;
        // le linha por linha do arquivo mtl
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "newmtl") {
                if (!currentMatName.empty()) materials[currentMatName] = currentMat;
                iss >> currentMatName;
                currentMat = Material(); 
                currentMat.name = currentMatName;
            }
            else if (prefix == "Ka") iss >> currentMat.ambient[0] >> currentMat.ambient[1] >> currentMat.ambient[2];
            else if (prefix == "Kd") iss >> currentMat.diffuse[0] >> currentMat.diffuse[1] >> currentMat.diffuse[2];
            else if (prefix == "Ks") iss >> currentMat.specular[0] >> currentMat.specular[1] >> currentMat.specular[2];
            else if (prefix == "Ns") iss >> currentMat.shininess;
            else if (prefix == "map_Kd") iss >> currentMat.textureFilename;
        }
        if (!currentMatName.empty()) materials[currentMatName] = currentMat;
        file.close();

        for (auto& pair : materials) {
            Material& mat = pair.second;
            if (!mat.textureFilename.empty()) {
                std::string path1 = std::string("../") + mat.textureFilename;
                std::string path2 = mat.textureFilename;
                std::string path3 = std::string("Objetos/") + mat.textureFilename;
                const char* texPaths[] = { path1.c_str(), path2.c_str(), path3.c_str() };
                for (int i = 0; i < 3; i++) {
                    mat.textureID = loadTexture(texPaths[i]); 
                    if (mat.textureID != 0) break;
                }
            }
        }
        return true;
    }

    bool loadFromFile(const char* filename) {
        // Se já carregou antes, limpa
        if (!vertices.empty()) {
            vertices.clear(); normals.clear(); texCoords.clear(); meshes.clear(); materials.clear();
        }

        std::ifstream file(filename);
        if (!file.is_open()) return false;

        std::vector<float> temp_vertices, temp_normals, temp_texCoords;
        std::map<std::string, unsigned int> vertexIndexMap;
        unsigned int nextIndex = 0; 
        
        Mesh currentMesh;
        Material defaultMaterial; 
        currentMesh.material = defaultMaterial;

        std::string mtlFilename;
        std::string objPath = filename;
        size_t lastSlash = objPath.find_last_of("/\\");
        std::string objDir = (lastSlash != std::string::npos) ? objPath.substr(0, lastSlash + 1) : "";
        
        float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
        float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix; 

            if (prefix == "mtllib") {
                iss >> mtlFilename;
                std::string fullPath1 = objDir + mtlFilename;
                std::string fullPath2 = mtlFilename;
                const char* mtlPaths[] = { fullPath1.c_str(), fullPath2.c_str() };
                for (int i = 0; i < 2; i++) {
                    if (loadMTL(mtlPaths[i])) break; 
                }
            }
            else if (prefix == "usemtl") {
                if (!currentMesh.indices.empty()) meshes.push_back(currentMesh);
                std::string matName;
                iss >> matName;
                currentMesh = Mesh(); 
                if (materials.count(matName)) currentMesh.material = materials[matName]; 
                else currentMesh.material = defaultMaterial; 
            }
            else if (prefix == "v") {
                float x, y, z; iss >> x >> y >> z;
                temp_vertices.push_back(x); temp_vertices.push_back(y); temp_vertices.push_back(z);
                if (x < minX) minX = x; if (x > maxX) maxX = x;
                if (y < minY) minY = y; if (y > maxY) maxY = y;
                if (z < minZ) minZ = z; if (z > maxZ) maxZ = z;
            }
            else if (prefix == "vn") {
                float nx, ny, nz; iss >> nx >> ny >> nz;
                temp_normals.push_back(nx); temp_normals.push_back(ny); temp_normals.push_back(nz);
            }
            else if (prefix == "vt") {
                float u, v; iss >> u >> v;
                temp_texCoords.push_back(u); temp_texCoords.push_back(v);
            }
            else if (prefix == "f") {
                for (int i = 0; i < 3; i++) {
                    std::string faceVertexStr; iss >> faceVertexStr; 
                    if (vertexIndexMap.count(faceVertexStr) == 0) {
                        vertexIndexMap[faceVertexStr] = nextIndex;
                        unsigned int v_idx = 0, vt_idx = 0, vn_idx = 0;
                        int matches = sscanf(faceVertexStr.c_str(), "%u/%u/%u", &v_idx, &vt_idx, &vn_idx);
                        if (matches != 3) {
                            matches = sscanf(faceVertexStr.c_str(), "%u//%u", &v_idx, &vn_idx);
                            if (matches != 2) {
                                matches = sscanf(faceVertexStr.c_str(), "%u/%u", &v_idx, &vt_idx);
                                if (matches != 2) sscanf(faceVertexStr.c_str(), "%u", &v_idx);
                            }
                        }
                        vertices.push_back(temp_vertices[(v_idx - 1) * 3 + 0]);
                        vertices.push_back(temp_vertices[(v_idx - 1) * 3 + 1]);
                        vertices.push_back(temp_vertices[(v_idx - 1) * 3 + 2]);

                        if (vt_idx > 0) {
                            texCoords.push_back(temp_texCoords[(vt_idx - 1) * 2 + 0]);
                            texCoords.push_back(temp_texCoords[(vt_idx - 1) * 2 + 1]);
                        } else { texCoords.push_back(0); texCoords.push_back(0); }
                        
                        if (vn_idx > 0) {
                            normals.push_back(temp_normals[(vn_idx - 1) * 3 + 0]);
                            normals.push_back(temp_normals[(vn_idx - 1) * 3 + 1]);
                            normals.push_back(temp_normals[(vn_idx - 1) * 3 + 2]);
                        } else { normals.push_back(0); normals.push_back(1); normals.push_back(0); }
                        nextIndex++; 
                    }
                    currentMesh.indices.push_back(vertexIndexMap[faceVertexStr]);
                }
            }
        }
        if (!currentMesh.indices.empty()) meshes.push_back(currentMesh);
        file.close();

        // Normalização
        float sizeX = maxX - minX; float sizeY = maxY - minY; float sizeZ = maxZ - minZ;
        float maxSize = std::max(std::max(sizeX, sizeY), sizeZ);
        float normalizeScale = 0.6f / maxSize; 
        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        float centerZ = (minZ + maxZ) / 2.0f;

        for (size_t i = 0; i < vertices.size(); i += 3) {
            vertices[i] = (vertices[i] - centerX) * normalizeScale;
            vertices[i+1] = (vertices[i+1] - centerY) * normalizeScale;
            vertices[i+2] = (vertices[i+2] - centerZ) * normalizeScale;
        }
        
        needsUpdate = true; 
        return true;
    }

    //desenhar
    void draw() {
        if (meshes.empty() || vertices.empty()) return;

        //se esta desenhando pela primeira vez ou um bloco precisa mudar de textura
        if (displayListID == 0 || needsUpdate) {
            if (displayListID == 0) displayListID = glGenLists(1);
            
            glNewList(displayListID, GL_COMPILE); 
            bool has_real_materials = !materials.empty();
            if (has_real_materials) glDisable(GL_COLOR_MATERIAL);

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

            for (auto& mesh : meshes) {
                if (has_real_materials) mesh.material.apply();
                glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, mesh.indices.data());
                if (has_real_materials && mesh.material.textureID != 0) glDisable(GL_TEXTURE_2D);
            }

            glDisableClientState(GL_VERTEX_ARRAY);
            if (!normals.empty()) glDisableClientState(GL_NORMAL_ARRAY);
            if (!texCoords.empty()) glDisableClientState(GL_TEXTURE_COORD_ARRAY);

            if (has_real_materials) glEnable(GL_COLOR_MATERIAL);

            glEndList(); 
            needsUpdate = false;
        }
        glCallList(displayListID);
    }
};