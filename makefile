# --- Variáveis do Compilador e Executável ---
CXX = C:/msys64/mingw64/bin/g++.exe
CXXFLAGS = -std=c++11 -Wall -O2 -I. -I"C:/msys64/mingw64/include/bullet" -I"C:/msys64/mingw64/include"
TARGET = output/estilingue.exe

# --- Arquivos do Projeto ---
# CORRIGIDO: Adicionado passaros/Red.cpp
SOURCES = estilingue.cpp SlingshotManager.cpp stb_impl.cpp 
OBJECTS = $(SOURCES:.cpp=.o)

# --- Caminhos e Bibliotecas ---
LIB_PATH = -L"C:/msys64/mingw64/lib"
LIBS = -lfreeglut -lglu32 -lopengl32 -lBulletDynamics -lBulletCollision -lLinearMath

# --- Regras do Make ---
all: $(TARGET)

# CORRIGIDO: Adicionada uma regra para criar o diretório 'output/'
$(TARGET): $(OBJECTS)
	@echo "==> Criando diretório de saída..."
	@mkdir -p output
	@echo "==> Linkando o executável: $(TARGET)"
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIB_PATH) $(LIBS)
	@echo "==> Build concluído!"

%.o: %.cpp
	@echo "==> Compilando $< ..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# CORRIGIDO: Regra 'clean' para usar 'rm -f' (mais compatível com msys/make)
# e para limpar corretamente os objetos (incluindo Red.o)
clean:
	@echo "==> Limpando arquivos compilados..."
	@rm -f $(OBJECTS) $(TARGET)

run: all
	@echo "==> Executando $(TARGET)..."
	$(TARGET)