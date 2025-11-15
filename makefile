# --- Variáveis do Compilador e Executável ---
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2 
TARGET = estilingue.exe

# --- Arquivos do Projeto ---
# CORRIGIDO: Adicionado stb_impl.cpp
SOURCES = estilingue.cpp SlingshotManager.cpp stb_impl.cpp

OBJECTS = $(SOURCES:.cpp=.o)

# --- Caminhos e Bibliotecas (Configuração do MinGW) ---
# CORRIGIDO: Adicionado -I.
INC_PATHS = -I. -I/c/msys64/mingw64/include/bullet -I/c/msys64/mingw64/include

LIB_PATH = -L/c/msys64/mingw64/lib
LIBS = -lfreeglut -lglu32 -lopengl32 -lBulletDynamics -lBulletCollision -lLinearMath

# --- Regras do Make ---
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "==> Linkando o executável: $(TARGET)"
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIB_PATH) $(LIBS)
	@echo "==> Build concluído!"

%.o: %.cpp
	@echo "==> Compilando $<..."
	$(CXX) $(CXXFLAGS) $(INC_PATHS) -c $< -o $@

clean:
	@echo "==> Limpando arquivos compilados..."
	rm -f $(TARGET) $(OBJECTS)

run: all
	@echo "==> Executando $(TARGET)..."
	./$(TARGET)