#include <GL/glut.h>
#include <GL/glu.h>
// Agora, use o caminho padrão do Bullet
#include <bullet/btBulletDynamicsCommon.h>
// #include <bullet/btVector3.h> // Exemplo de outro header
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <map>
#include <windows.h>
// --- ESTRUTURAS DE DADOS ---
using namespace std;

struct Color {
    GLfloat r, g, b;
};

struct Material {
    std::string name;
    Color diffuse_color = {0.8f, 0.8f, 0.8f}; // Kd (Cor Difusa - Padrão Cinza)
};

struct FaceGroup {
    std::string material_name;
    std::vector<GLint> vertex_indices; // Índices globais de Vértices
};

// --- VARIÁVEIS GLOBAIS ---

std::vector<GLfloat> vertices;      // Coordenadas Vértices (v x y z)
std::map<std::string, Material> materials; // Mapeamento do nome do material
std::vector<FaceGroup> face_groups;        // Grupos de faces, organizados por material

float rotation_angle = 0.0f; 
bool is_rotating = false;      
const int TIMER_MS = 25;       

// --- FUNÇÕES DE CARREGAMENTO DE MATERIAIS ---

// Função para ler o arquivo MTL e extrair a Cor Difusa (Kd)
void loadMTL(const std::string& mtl_path) {
    std::ifstream file(mtl_path);
    if (!file.is_open()) {
        std::cerr << "ERRO MTL: Nao foi possivel abrir o arquivo MTL: " << mtl_path << std::endl;
        return;
    }

    std::string line;
    std::string current_mtl_name;
    
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "newmtl") {
            // Inicia um novo material
            ss >> current_mtl_name;
            materials[current_mtl_name].name = current_mtl_name;
            
        } else if (type == "Kd") {
            // Linha que define a Cor Difusa (Cor Sólida)
            GLfloat r, g, b;
            if (ss >> r >> g >> b && !current_mtl_name.empty()) {
                materials[current_mtl_name].diffuse_color = {r, g, b};
                std::cout << "DEBUG MTL: Cor Kd do material '" << current_mtl_name << "' carregada." << std::endl;
            }
        }
    }
    file.close();
}


// --- FUNÇÕES DE PARSING OBJ ---

int get_vertex_index(const std::string& face_part) {
    std::stringstream ss(face_part);
    std::string segment;
    std::getline(ss, segment, '/'); 
    
    try {
        if (segment.empty()) return -1;
        int index = std::stoi(segment);
        return index > 0 ? index - 1 : -1;
    } catch (const std::exception& e) {
        return -1;
    }
}

// Função para extrair o caminho (path) de um nome de arquivo (para achar o MTL)
std::string get_path(const std::string& full_path) {
    size_t last_slash = full_path.find_last_of("/\\");
    if (last_slash == std::string::npos) {
        return ""; // Se não houver caminho, retorna vazio
    }
    return full_path.substr(0, last_slash + 1);
}

// Função para carregar o arquivo OBJ, ler MTL e agrupar faces
void loadOBJ(const char* obj_path) {
    std::ifstream file(obj_path);
    if (!file.is_open()) {
        cout << "Caminho do arquivo OBJ: " << obj_path << endl;
        // C:\Users\davi5\UFPI\4_periodo\AngryBirds3D_OpenGL_Glut\Objetos\estilingue.obj
        std::cerr << "ERRO OBJ: Nao foi possivel abrir o arquivo OBJ: " << obj_path << std::endl;
        return;
    }

    std::string path_prefix = get_path(obj_path); // Extrai o prefixo do caminho
    std::string line;
    std::vector<GLfloat> temp_vertices;
    
    FaceGroup current_group = {"default_material", {}};
    face_groups.push_back(current_group);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "mtllib") {
            std::string mtl_filename;
            ss >> mtl_filename;
            
            // CONCATENA O CAMINHO: path/estilingue.mtl
            loadMTL(path_prefix + mtl_filename); 
            
        } else if (type == "v") {
            GLfloat x, y, z;
            if (ss >> x >> y >> z) {
                temp_vertices.push_back(x);
                temp_vertices.push_back(y);
                temp_vertices.push_back(z);
            }
        } else if (type == "usemtl") {
            std::string material_name;
            ss >> material_name;
            
            if (!face_groups.back().vertex_indices.empty()) {
                 face_groups.push_back({material_name, {}});
            } else {
                 face_groups.back().material_name = material_name;
            }
            
        } else if (type == "f") {
            std::string part;
            std::vector<int> face_v_indices;

            while (ss >> part) {
                int v_index = get_vertex_index(part); 
                if (v_index != -1) face_v_indices.push_back(v_index);
            }

            if (face_v_indices.size() >= 3) {
                for (size_t i = 1; i < face_v_indices.size() - 1; ++i) {
                    face_groups.back().vertex_indices.push_back(face_v_indices[0]);
                    face_groups.back().vertex_indices.push_back(face_v_indices[i]);
                    face_groups.back().vertex_indices.push_back(face_v_indices[i+1]);
                }
            }
        }
    }
    file.close();

    vertices = temp_vertices;
    
    std::cout << "DEBUG: Carregamento OBJ finalizado. " << face_groups.size() << " grupos de faces." << std::endl;
}


// --- FUNÇÕES DE CALLBACK DO GLUT (MANTIDAS) ---

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 0.1, 100.0); 
    glMatrixMode(GL_MODELVIEW);
}

void timer(int value) {
    if (is_rotating) {
        rotation_angle += 2.0f; 
        if (rotation_angle > 360.0f) {
            rotation_angle -= 360.0f;
        }
        glutPostRedisplay(); 
    }
    glutTimerFunc(TIMER_MS, timer, 0);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 'R' || key == 'r') {
        is_rotating = !is_rotating; 
        std::cout << "Rotacao: " << (is_rotating ? "Ligada" : "Desligada") << std::endl;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    // --- TRANSFORMACÕES ---
    glTranslatef(0.0f, -5.0f, -30.0f); 
    glRotatef(rotation_angle, 0.0f, 1.0f, 0.0f); 
    glScalef(30.0f, 30.0f, 30.0f); 

    glDisable(GL_TEXTURE_2D); 

    // Desenha cada grupo de faces
    for (const auto& group : face_groups) {
        auto it = materials.find(group.material_name);
        
        // 1. Aplica a cor do material
        if (it != materials.end()) {
            glColor3f(it->second.diffuse_color.r, 
                      it->second.diffuse_color.g, 
                      it->second.diffuse_color.b);
        } else {
            // Fallback (Cor cinza) - Se o material não for encontrado
            glColor3f(0.3f, 0.3f, 0.3f); 
        }

        // 2. Desenha os triângulos do grupo
        glBegin(GL_TRIANGLES);
        for (GLint v_index : group.vertex_indices) {
            glVertex3f(vertices[v_index * 3], vertices[v_index * 3 + 1], vertices[v_index * 3 + 2]);
        }
        glEnd();
    }
    
    glutSwapBuffers();
}

// --- FUNÇÃO PRINCIPAL ---

int main(int argc, char** argv) {

    char buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);

    string dir = buffer;

    if (dir.back() != '\\' && dir.back() != '/')
        dir += '/';

    dir += "\\..\\Objetos\\";;
    string arquivo = "estilinguee.obj";
    string caminho_completo = dir + arquivo;

    cout << "Caminho completo: " << caminho_completo << endl;

    loadOBJ(caminho_completo.c_str());

    if (vertices.empty() || face_groups.empty()) {
        std::cerr << "Falha ao carregar o modelo. Encerrando." << std::endl;
        return 1;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("OBJ com Cores do MTL e Rotacao - Pressione 'R'");
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
    glEnable(GL_DEPTH_TEST);              
    glDisable(GL_CULL_FACE);              
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);         
    glutKeyboardFunc(keyboard);       
    glutTimerFunc(TIMER_MS, timer, 0);

    glutMainLoop();
    return 0;
}