# Компилятор и флаги
CC = gcc
CFLAGS = -W -Wall -Wextra -std=c11 -pedantic -Wno-unused-parameter -Wno-unused-variable

# Режим сборки (по умолчанию DEBUG)
MODE ?= DEBUG

# Директории
SRC_DIR = src
BUILD_DIR = build
DEBUG_DIR = $(BUILD_DIR)/debug
RELEASE_DIR = $(BUILD_DIR)/release

# Исходные файлы
PARENT_SRC = $(SRC_DIR)/parent.c
CHILD_SRC = $(SRC_DIR)/child.c

# Имена исполняемых файлов
PARENT_EXE = parent
CHILD_EXE = child

# Флаги для режимов
ifeq ($(MODE), DEBUG)
    CFLAGS += -g -O0  # Отладочные флаги
    OUTPUT_DIR = $(DEBUG_DIR)
else ifeq ($(MODE), RELEASE)
    CFLAGS += -O2  # Оптимизация для релиза
    OUTPUT_DIR = $(RELEASE_DIR)
else
    $(error Invalid MODE. Use DEBUG or RELEASE)
endif

# Цель по умолчанию
all: $(OUTPUT_DIR)/$(PARENT_EXE) $(OUTPUT_DIR)/$(CHILD_EXE)

# Сборка parent
$(OUTPUT_DIR)/$(PARENT_EXE): $(PARENT_SRC)
	@mkdir -p $(OUTPUT_DIR)  # Создаем папку, если её нет
	$(CC) $(CFLAGS) -o $@ $<

# Сборка child
$(OUTPUT_DIR)/$(CHILD_EXE): $(CHILD_SRC)
	@mkdir -p $(OUTPUT_DIR)  # Создаем папку, если её нет
	$(CC) $(CFLAGS) -o $@ $<

# Очистка
clean:
	rm -rf $(BUILD_DIR)  # Удаляем папку build и всё её содержимое

# Псевдоцели
.PHONY: all clean
