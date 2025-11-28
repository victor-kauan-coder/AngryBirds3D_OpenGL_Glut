# --- Estágio de Compilação (Build) ---
FROM gcc:latest AS compilador

WORKDIR /build

# 1. REMOVIDO: Não precisamos mais instalar 'cmake' via apt-get.
# O comando 'make' já vem instalado na imagem gcc:latest.

COPY . .

# 2. ALTERADO: Comando de compilação
# Em vez de criar pasta build e rodar cmake, apenas rodamos o make.
# Instala bibliotecas necessárias (Ex: Bullet Physics, OpenGL, Boost)
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
	build-essential \
	pkg-config \
	libgl1-mesa-dev \
	libglu1-mesa-dev \
	freeglut3-dev \
	libbullet-dev \
	libsdl2-dev \
	libsdl2-mixer-dev \
	&& rm -rf /var/lib/apt/lists/*

# Remove any prebuilt object files (copied from host) that may be Windows/Mingw
# to ensure the container compiles fresh Linux objects, then build.
RUN  make clean \
	 && make 

CMD ["./estilingue"]
# --- Estágio de Execução (Runtime) ---
# FROM debian:bookworm-slim

# WORKDIR /app

# RUN apt-get update && apt-get install -y \
#     freeglut3-dev \
#     libglu1-mesa-dev \
#     libgl1-mesa-glx \
#     libsdl2-2.0-0 \
#     libsdl2-mixer-2.0-0 \
#     libbullet-dev \
#     && rm -rf /var/lib/apt/lists/*
# # 3. ALTERADO: Caminho da cópia
# # O Makefile geralmente gera o executável na mesma pasta raiz (a menos que configurado diferente).
# # Antes era: /build/build/meu_executavel
# # Agora é provavelmente: /build/meu_executavel
# COPY --from=compilador /build/estilingue .

# CMD ["./estilingue"]