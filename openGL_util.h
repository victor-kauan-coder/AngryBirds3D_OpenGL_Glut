
//Limpa a tela com a cor branca;
void init(void){
    glClearColor(1.0, 1.0, 1.0, 1.0); 
}

void setupCamera(float cameraDistance = 5.0f, float cameraAngleX = 20.0f, float cameraAngleY = 30.0f) {
    // Configura a projeção em perspectiva
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 800.0f/600.0f, 0.1f, 100.0f);
    
    // Configura a câmera
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Posiciona a câmera
    float camX = cameraDistance * cos(cameraAngleY * M_PI / 180.0f) * cos(cameraAngleX * M_PI / 180.0f);
    float camY = cameraDistance * sin(cameraAngleX * M_PI / 180.0f);
    float camZ = cameraDistance * sin(cameraAngleY * M_PI / 180.0f) * cos(cameraAngleX * M_PI / 180.0f);
    
    gluLookAt(camX, camY, camZ,  // Posição da câmera
              0.0f, 0.0f, 0.0f,  // Olhando para a origem
              0.0f, 1.0f, 0.0f); // Vetor up
}

void setupLighting() {
    // Habilita iluminação
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    // Configuração da luz
    GLfloat light_position[] = { 5.0f, 5.0f, 5.0f, 1.0f };
    GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
}