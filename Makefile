# Snake Notcurses Makefile
SNAKE_VERSION = 1.00
MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

SRC_DIR = src
BUILD_DIR = build
SOUNDS_SRC_DIR = $(SRC_DIR)/snake_sounds
HEADER_FILE = $(SRC_DIR)/notcurses-snake.h

LIB_NAME = notcurses-snake
STATIC_LIB = lib$(LIB_NAME).a
DYNAMIC_LIB = lib$(LIB_NAME).so.$(MAJOR_VERSION).$(MINOR_VERSION).$(PATCH_VERSION)

PACKAGE_MANAGER := $(shell which dnf 2>/dev/null || which apt-get 2>/dev/null || echo "unknown")

ifeq ($(findstring apt-get,$(PACKAGE_MANAGER)),apt-get)
  PREFIX = /usr
  LIB_INSTALL_PATH = $(PREFIX)/lib/x86_64-linux-gnu
else
  PREFIX = /usr/local
  LIB_INSTALL_PATH = $(PREFIX)/lib64
endif

HEADER_INSTALL_PATH = $(PREFIX)/include
SOUNDS_INSTALL_PATH = $(PREFIX)/share/notcurses-snake-sounds
LD_CONF_DIR = /etc/ld.so.conf.d
LD_CONF_FILE = $(LD_CONF_DIR)/notcurses-snake.conf

CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -Wpedantic -g \
         -D_XOPEN_SOURCE=600 -D_GNU_SOURCE \
         -O3 -ffast-math -finline-functions -funroll-loops \
         -mtune=native -ftree-vectorize -fPIC \
         -fopenmp -flto -fvisibility=default \
         -DSNAKE_VERSION=$(SNAKE_VERSION)f

LDFLAGS_BASE = -lnotcurses -lnotcurses-core -ldeflate -lunistring \
               -lgpm -lpthread -lm -lavcodec -lavformat -lavutil \
               -lswscale -lavdevice -lSDL2 -lSDL2_mixer

ifeq ($(findstring apt-get,$(PACKAGE_MANAGER)),apt-get)
  LDFLAGS = $(LDFLAGS_BASE) -lncursesw -ltinfo
else
  LDFLAGS = $(LDFLAGS_BASE) -lncursesw
endif

LIB_SOURCES = $(filter-out $(SRC_DIR)/main.c, $(wildcard $(SRC_DIR)/*.c))
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/obj/%.o,$(LIB_SOURCES))

.PHONY: all
all: directories static dynamic snakegame

.PHONY: directories
directories:
	@mkdir -p $(BUILD_DIR)/obj $(BUILD_DIR)/lib $(BUILD_DIR)/bin

$(BUILD_DIR)/obj/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

.PHONY: static
static: $(BUILD_DIR)/lib/$(STATIC_LIB)

$(BUILD_DIR)/lib/$(STATIC_LIB): $(OBJECTS)
	ar rcs --plugin=$(shell gcc --print-file-name=liblto_plugin.so) $@ $^
	ranlib $@

.PHONY: dynamic
dynamic: $(BUILD_DIR)/lib/$(DYNAMIC_LIB)

$(BUILD_DIR)/lib/$(DYNAMIC_LIB): $(OBJECTS)
	$(CC) -shared $(CFLAGS) -Wl,-soname,lib$(LIB_NAME).so.$(MAJOR_VERSION) \
	-o $@ $^ $(LDFLAGS) -Wl,--no-undefined -Wl,--as-needed -Wl,-rpath=$(LIB_INSTALL_PATH)
	ln -sf $(DYNAMIC_LIB) $(BUILD_DIR)/lib/lib$(LIB_NAME).so.$(MAJOR_VERSION)
	ln -sf lib$(LIB_NAME).so.$(MAJOR_VERSION) $(BUILD_DIR)/lib/lib$(LIB_NAME).so

# 关键修复：调整链接顺序和参数
snakegame: $(SRC_DIR)/main.c static
	$(CC) $(CFLAGS) -I$(SRC_DIR) -L$(BUILD_DIR)/lib \
	$< \
	-Wl,--whole-archive -l:$(STATIC_LIB) -Wl,--no-whole-archive \
	$(LDFLAGS) \
	-o $(BUILD_DIR)/bin/emoji_snakegame

.PHONY: check-ld-config
check-ld-config:
	@if [ ! -f $(LD_CONF_FILE) ]; then \
		echo "$(LIB_INSTALL_PATH)" > $(LD_CONF_FILE); \
		ldconfig; \
	else \
		grep -q "^$(LIB_INSTALL_PATH)$$" $(LD_CONF_FILE) || { \
			echo "$(LIB_INSTALL_PATH)" >> $(LD_CONF_FILE); \
			ldconfig; \
		}; \
	fi

.PHONY: install
install: static dynamic snakegame
	install -d $(LIB_INSTALL_PATH) $(HEADER_INSTALL_PATH) $(SOUNDS_INSTALL_PATH)
	install -m 644 $(BUILD_DIR)/lib/$(STATIC_LIB) $(LIB_INSTALL_PATH)/$(STATIC_LIB)
	install -m 755 $(BUILD_DIR)/lib/$(DYNAMIC_LIB) $(LIB_INSTALL_PATH)
	cd $(LIB_INSTALL_PATH) && ln -sf $(DYNAMIC_LIB) lib$(LIB_NAME).so.$(MAJOR_VERSION) \
	&& ln -sf lib$(LIB_NAME).so.$(MAJOR_VERSION) lib$(LIB_NAME).so
	install -m 644 $(HEADER_FILE) $(HEADER_INSTALL_PATH)
	install -m 644 $(SOUNDS_SRC_DIR)/* $(SOUNDS_INSTALL_PATH)
	install -m 755 $(BUILD_DIR)/bin/emoji_snakegame /usr/bin/emoji_snakegame
	@if [ $$(id -u) -eq 0 ]; then $(MAKE) check-ld-config; fi

.PHONY: remove-ld-config
remove-ld-config:
	@rm -f $(LD_CONF_FILE) 2>/dev/null && ldconfig || true

.PHONY: uninstall
uninstall:
	rm -f $(LIB_INSTALL_PATH)/$(STATIC_LIB) \
	$(LIB_INSTALL_PATH)/lib$(LIB_NAME).so* \
	$(LIB_INSTALL_PATH)/$(DYNAMIC_LIB)
	rm -f $(HEADER_INSTALL_PATH)/$(notdir $(HEADER_FILE))
	rm -rf $(SOUNDS_INSTALL_PATH)
	rm -f /usr/bin/emoji_snakegame
	@if [ $$(id -u) -eq 0 ]; then $(MAKE) remove-ld-config; fi

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	find . -name "*.o" -delete

.PHONY: debug
debug: CFLAGS += -DDEBUG -g
debug:
	@# 获取系统语言
	@LANG=$$(locale | grep "LANG=" | cut -d= -f2 | cut -d. -f1); \
	case $$LANG in \
		ru_RU) \
			echo "Проверка системы..."; \
			LANG_MSG="Русский язык обнаружен"; \
			;; \
		zh_CN) \
			echo "正在检查系统..."; \
			LANG_MSG="检测到简体中文"; \
			;; \
		zh_TW|zh_HK) \
			echo "正在檢查系統..."; \
			LANG_MSG="檢測到繁體中文"; \
			;; \
		es_*) \
			echo "Verificando el sistema..."; \
			LANG_MSG="Español detectado"; \
			;; \
		pt_*) \
			echo "Verificando o sistema..."; \
			LANG_MSG="Português detectado"; \
			;; \
		*) \
			echo "Checking system..."; \
			LANG_MSG="English detected (default)"; \
			;; \
	esac; \
	echo "$$LANG_MSG"

	@# 检查 build/bin 目录是否存在
	@if [ ! -d "$(BUILD_DIR)/bin" ]; then \
		LANG=$$(locale | grep "LANG=" | cut -d= -f2 | cut -d. -f1); \
		case $$LANG in \
			ru_RU) echo "Ошибка: Каталог $(BUILD_DIR)/bin не существует, создаём...";; \
			zh_CN) echo "错误: 目录 $(BUILD_DIR)/bin 不存在，自动创建该目录";; \
			zh_TW|zh_HK) echo "錯誤: 目錄 $(BUILD_DIR)/bin 不存在，自動創建該目錄";; \
			es_*) echo "Error: El directorio $(BUILD_DIR)/bin no existe, creándolo...";; \
			pt_*) echo "Erro: O diretório $(BUILD_DIR)/bin não existe, criando...";; \
			*) echo "Error: Directory $(BUILD_DIR)/bin does not exist, creating...";; \
		esac; \
		mkdir -p $(BUILD_DIR)/bin; \
	fi

	@# 检查目录权限
	@if [ ! -w "$(BUILD_DIR)/bin" ]; then \
		LANG=$$(locale | grep "LANG=" | cut -d= -f2 | cut -d. -f1); \
		case $$LANG in \
			ru_RU) \
				echo "Ошибка: Нет прав на запись в каталог $(BUILD_DIR)/bin"; \
				echo "Выполните: chmod 755 $(BUILD_DIR)/bin";; \
			zh_CN) \
				echo "错误: 目录 $(BUILD_DIR)/bin 没有写入权限"; \
				echo "请执行: chmod 755 $(BUILD_DIR)/bin";; \
			zh_TW|zh_HK) \
				echo "錯誤: 目錄 $(BUILD_DIR)/bin 沒有寫入權限"; \
				echo "請執行: chmod 755 $(BUILD_DIR)/bin";; \
			es_*) \
				echo "Error: No hay permisos de escritura en $(BUILD_DIR)/bin"; \
				echo "Ejecute: chmod 755 $(BUILD_DIR)/bin";; \
			pt_*) \
				echo "Erro: Sem permissão de escrita em $(BUILD_DIR)/bin"; \
				echo "Execute: chmod 755 $(BUILD_DIR)/bin";; \
			*) \
				echo "Error: No write permission for directory $(BUILD_DIR)/bin"; \
				echo "Please run: chmod 755 $(BUILD_DIR)/bin";; \
		esac; \
		exit 1; \
	fi

	@# 检查目录所有者
	@CURRENT_USER=$$(whoami); \
	DIR_OWNER=$$(ls -ld "$(BUILD_DIR)/bin" | awk '{print $$3}'); \
	if [ "$$CURRENT_USER" != "$$DIR_OWNER" ]; then \
		LANG=$$(locale | grep "LANG=" | cut -d= -f2 | cut -d. -f1); \
		case $$LANG in \
			ru_RU) \
				echo "Ошибка: Каталог $(BUILD_DIR)/bin не принадлежит текущему пользователю $$CURRENT_USER"; \
				echo "Выполните: sudo chown $$CURRENT_USER:$$CURRENT_USER $(BUILD_DIR)/bin";; \
			zh_CN) \
				echo "错误: 目录 $(BUILD_DIR)/bin 不属于当前用户 $$CURRENT_USER"; \
				echo "请执行: sudo chown $$CURRENT_USER:$$CURRENT_USER $(BUILD_DIR)/bin";; \
			zh_TW|zh_HK) \
				echo "錯誤: 目錄 $(BUILD_DIR)/bin 不屬於當前用戶 $$CURRENT_USER"; \
				echo "請執行: sudo chown $$CURRENT_USER:$$CURRENT_USER $(BUILD_DIR)/bin";; \
			es_*) \
				echo "Error: El directorio $(BUILD_DIR)/bin no pertenece al usuario actual $$CURRENT_USER"; \
				echo "Ejecute: sudo chown $$CURRENT_USER:$$CURRENT_USER $(BUILD_DIR)/bin";; \
			pt_*) \
				echo "Erro: O diretório $(BUILD_DIR)/bin não pertence ao usuário atual $$CURRENT_USER"; \
				echo "Execute: sudo chown $$CURRENT_USER:$$CURRENT_USER $(BUILD_DIR)/bin";; \
			*) \
				echo "Error: Directory $(BUILD_DIR)/bin is not owned by current user $$CURRENT_USER"; \
				echo "Please run: sudo chown $$CURRENT_USER:$$CURRENT_USER $(BUILD_DIR)/bin";; \
		esac; \
		exit 1; \
	fi

	$(CC) $(CFLAGS) $(SRC_DIR)/*.c -o $(BUILD_DIR)/bin/emoji_snakegame $(LDFLAGS_BASE)


.PHONY: check-install
check-install:
	@ls -l $(LIB_INSTALL_PATH)/lib$(LIB_NAME).* || true
	@ldconfig -p | grep $(LIB_NAME) || true
	@[ -f $(LD_CONF_FILE) ] && cat $(LD_CONF_FILE) || echo "No linker config"