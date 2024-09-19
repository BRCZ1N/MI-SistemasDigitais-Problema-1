# Nome do compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall -O2 -std=c11

# Nome do arquivo de saída
TARGET = accel

<<<<<<< Updated upstream
# Lista de arquivos fonte
SRCS = accel.c

# Gera o arquivo objeto a partir do arquivo fonte
OBJS = $(SRCS:.c=.o)
=======
# Source files
SRCS = acelerometro.c

# Output executable
TARGET = acelerometro
>>>>>>> Stashed changes

# Regra padrão para criar o executável
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Regra para criar arquivos objeto
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza dos arquivos gerados (executável e objetos)
clean:
	rm -f $(TARGET) $(OBJS)

# Regra para executar o programa (opcional)
run: $(TARGET)
	./$(TARGET)
