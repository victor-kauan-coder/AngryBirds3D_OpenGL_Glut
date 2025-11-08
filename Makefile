# --- Variáveis do Compilador e Executável ---
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2 # Flags de compilação: C++11, avisa sobre warnings, otimizado
TARGET = estilingue.exe

# --- Arquivos do Projeto ---
# Lista de todos os seus arquivos .cpp
SOURCES = estilingue.cpp SlingshotManager.cpp
# Converte a lista de .cpp para .o (arquivos objeto)
OBJECTS = $(SOURCES:.cpp=.o)

# --- Caminhos e Bibliotecas (Configuração do MinGW) ---
# Caminhos de Inclusão (Headers .h) - IMPORTANTE: Use o formato de caminho do MINGW
INC_PATHS = -I/c/msys64/mingw64/include/bullet -I/c/msys64/mingw64/include

# Caminho das Bibliotecas (Libs .a)
LIB_PATH = -L/c/msys64/mingw64/lib

# Nomes das Bibliotecas (Linkagem)
# Use -lfreeglut, -lglu32, -lopengl32 como discutimos
LIBS = -lfreeglut -lglu32 -lopengl32 -lBulletDynamics -lBulletCollision -lLinearMath

# --- Regras do Make ---

# A regra "all" é a padrão. Ela será executada quando você digitar "make".
# Ela diz que o objetivo final é construir o nosso $(TARGET)
all: $(TARGET)

# Regra para "linkar" o executável final
# Depende de todos os arquivos objeto (.o)
$(TARGET): $(OBJECTS)
	@echo "==> Linkando o executável: $(TARGET)"
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIB_PATH) $(LIBS)
	@echo "==> Build concluído!"

# Regra padrão para compilar um .cpp em um .o
# "%.o: %.cpp" é uma regra que diz: "Para fazer qualquer arquivo .o,
# encontre o .cpp correspondente e execute este comando."
%.o: %.cpp
	@echo "==> Compilando $<..."
	$(CXX) $(CXXFLAGS) $(INC_PATHS) -c $< -o $@

# Regra "clean" - para limpar os arquivos compilados
clean:
	@echo "==> Limpando arquivos compilados..."
	rm -f $(TARGET) $(OBJECTS)

# Regra "run" - para compilar E executar
run: all
	@echo "==> Executando $(TARGET)..."
	./$(TARGET)