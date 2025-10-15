CC = gcc # переменная, определяющая компилятор
CFLAGS = -Wall -g # флаги компиляции для компилятора

TARGETS = parallel_min_max process_memory #исполняемые файлы
#пути
PARALLEL_SRC = lab3/src/parallel_min_max.c lab3/src/utils.c lab3/src/find_min_max.c
MEMORY_SRC   = lab4/src/process_memory.c
#сборка программ
all: $(TARGETS)

parallel_min_max: $(PARALLEL_SRC)
	$(CC) $(CFLAGS) -o $@ $^

process_memory: $(MEMORY_SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean: #удаление скомпилированных файлов
	rm -f $(TARGETS)

.PHONY: all clean
