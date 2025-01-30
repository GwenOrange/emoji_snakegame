# Emoji SnakeGame

[![Apache License 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![C23 Ready](https://img.shields.io/badge/C-23-green)](https://en.cppreference.com/w/c/23)

![Game Demo](demo.gif)

`emoji_snakegame` es una biblioteca de extensión de juego TUI basada en [Notcurses](https://github.com/dankamongmen/notcurses). Permite integrar rápidamente un plano de juego interactivo de la serpiente en cualquier programa de Notcurses que se esté ejecutando en modo de pantalla completa.

🇬🇧 [English](README.md) | 🇨🇳 [中文](README_ZH.md) | 🇨🇳 [简中](README_ZH_CN.md) 
|------|------|------|
🇵🇹 [Português](README_PT.md) | 🇷🇺 [Русский](README_RU.md) | 🇪🇸 [Español](README_ES.md)

## 🌟 Características Principales

- 🎮 Multihilo, puedes iniciar tantos planos de serpiente como desees.
- 🎵 Soporte de efectos de sonido en tiempo real (usando SDL2_mixer).
- 🕹️ Mayor jugabilidad en comparación con el juego original de la serpiente. La serpiente tiene objetos y habilidades.
- 🎨 Gráficos de Emoji (requiere soporte de fuentes en el terminal).

## 🎮 Resumen del Juego
![alt text](image-1.png)

Te encuentras en un colorido mundo de Emoji, como la adorable serpiente Emoji 😋, ¡embarcándote en una emocionante aventura! Tu objetivo es recoger suficientes naranjas 🍊 deliciosas dentro de un tiempo especificado y superar varios desafíos en el camino. Este mágico laberinto está lleno de sorpresas y diversión, ¡así que ven a jugar y seguro que completarás esta aventura!

Consigue puntos recogiendo suficientes naranjas 🍊 dentro del tiempo especificado 🕛. Cuando logres el objetivo 🎯, ganarás el juego; de lo contrario, si tu personaje muere por cualquier razón (incluyendo que se acabe el tiempo), perderás el juego.

## 💥 Sistema de Objetos Rico

### 🔪 Destruidor: ¡Tu Terrible Arma!
Este poderoso objeto puede destruir las paredes que bloquean tu camino, abriendo nuevos pasajes. Cada vez que destruyas 5 paredes con éxito, recibirás puntos extra. Si adquieres una habilidad de Debuff y luego obtienes '🔪' inmediatamente, se restablecerá tu personaje a la normalidad. Pero ten cuidado, porque el molesto tomate ardiente 🍅 puede limitar tu capacidad de destrucción. El tomate ardiente 🍅 suprimirá la habilidad del destruidor y te otorgará una velocidad extraordinaria 🤯. ¿Podrás elegir entre velocidad y destrucción para alcanzar tu objetivo?

### ⌛ Reloj de Arena: El Maravilloso Artefacto de Control del Tiempo
Al presionar la tecla `k` o `espacio`, puedes pausar el tiempo e incluso retroceder al pasado. ¡Esto te ayudará a lidiar fácilmente con diversas crisis! Sin embargo, esos molestos hongos 🍄 pueden arruinar tu capacidad de controlar el tiempo; '🍄' ralentizará la velocidad de tu personaje, pero no demasiado, mientras que también desorganiza la dirección de tu movimiento 😱. La barra de estado será desactivada, y después de un tiempo, tu personaje recuperará de repente la velocidad y dirección originales. ¿Podrás reaccionar a tiempo?

### 🧲 Imán: El Mágico Acelerador de Puntos
Su mayor característica es que no solo aumenta significativamente tu puntuación, sino que también te ayuda a refrescar los objetos. Cualquier objeto que recojas te otorgará puntos. El único problema es que cuando aparezca el molesto bicho 🐛, puede interrumpirte. El '🐛' te hará sentir nauseabundo 🤮 al recoger comida, y cuando estés en estado de 🧲, recoger cualquier objeto o comida te penalizará con pérdida de puntos.

## 🎯 Objetivo del Juego
Recoger con éxito suficientes naranjas 🍊 dentro del tiempo especificado; de lo contrario, el juego fallará.

## 🕹️ Guía de Controles
- Usa `WASD` o las teclas de flecha para moverte.
- Presiona `k` o `espacio` para activar la habilidad de detener el tiempo.
- Haz clic en `Tab` o `Enter` para pausar el juego.
- Presiona `Esc` para salir del juego.

## 🧩 Obstáculos y Desafíos
Esas aterradoras esculturas de piedra 🗿 son un gran problema. Sin embargo, si obtienes el poder del destruidor, se convertirán en papas fácilmente destruidas 🥔. ¡Apresúrate a destruirlas dentro del tiempo especificado! Si no, quedarás atrapado por las paredes, lo que resultará en un game over.

## 🚀 Cómo Iniciar
Para comenzar a jugar, ingresa el siguiente comando en un terminal que soporte Emoji:
```bash
emoji-snakegame n
```
donde `n` es el número del nivel.

## 📦 Guía de Instalación
#### Recomendaciones de Compilación

- **GCC 9+**
- **std=c11+**
- **Sistemas Linux basados en Red Hat**
- **Sistemas Linux basados en Debian**

| Contenido de Instalación | Ruta de Instalación                         |
| ------------------------ | ------------------------------------------- |
| Archivo Ejecutable       | /usr/bin/emoji_snakegame                   |
| Biblioteca Dinámica/Estática | /usr/local/lib/libnotcurses-snake**   |
| Archivo de Cabecera      | /usr/local/include/notcurses-snake.h      |
| Recursos de Sonido       | /usr/local/share/notcurses-snake-sounds    |

### Construir desde el Código Fuente
```bash
# Extraer y entrar en el directorio del proyecto
tar -xvf emoji-snake.tar.xz
cd emoji-snake

# Inicializar el entorno de construcción e instalar paquetes (detección automática de APT/DNF)
bash setup-dev-env.sh

# Compilar e instalar
sudo make && sudo make install

# Verificar la instalación
emoji_snakegame v
```
### Desinstalar
```bash
sudo make uninstall
```

## Compilar y Enlazar Snake a tu Programa TUI
El siguiente contenido te indicará cómo usar la biblioteca libnotcurses-snake para incrustar el juego en tu programa TUI.

## 🛠️ Integración Rápida

### Ejemplo
```c
#include <notcurses-snake.h>
#include <notcurses/notcurses.h>

int main()
{
    // Inicializa Notcurses
    struct notcurses *nc = notcurses_init(NULL, stdout);
    struct ncplane *plane = notcurses_stdplane(nc);

    // Configura los parámetros del juego
    struct SnakeOpt snakeOpt = {
        .snakeNcplane = plane,
        .width = 60,      // Ancho del mapa; de manera similar, una cadena ancha hace que el ancho real sea 60*2=160
        .height = 30,     // Altura del mapa; asegúrate de que el plano reserve al menos 3 filas para la barra de estado. La altura real renderizada es 30+3+1=34.
        .fade = true,     // Habilita el efecto de degradado de color del mapa
        .level = 1,       // Nivel inicial
        .isMusicOn = true // Habilita la música de fondo; solo una serpiente puede habilitar música.
    };

    // Inicializa el motor del juego
    struct SnakeState *game = SnakeGameInit(nc, &snakeOpt);
    // Variable de timestamp para el manejo de debounce
    long long last_key_timestamp = 0;
    // Bucle principal del juego
    while (SnakeGameIsRunning(game))
    {
        struct ncinput ni;
        if (notcurses_get_nblock(nc, &ni))
        {
            // Manejo de debounce de entrada, para evitar entradas repetidas en las opciones del menú; también puedes usarlo en otros lugares
            bool allow_input = SnakeInputDebounceControl(
                &last_key_timestamp,
                50000LL);

            if (allow_input && ni.id == NCKEY_ENTER || ni.id == NCKEY_TAB)
            {
                SnakeGameTryPause(game);
            }
            else if (allow_input && ni.id == NCKEY_ESC)
            {
                snakeGameExit(game);
            }
            else
            {
                SnakeGameInput(game, ni); // Nota: SnakeGameInput ya tiene debounce, no se necesita un mecanismo adicional para retrasarlo.
            }
        }

        // Lógica de renderizado
        if (SnakeGameShouldRender(game))
        {
            notcurses_render(nc);
            SnakeGameUnlockRender(game);
        }

        usleep(50000); // Ciclo de actualización de 50 ms
    }

    // Limpiar recursos
    SnakeGameAllClean();
    notcurses_stop(nc);
    return 0;
}
```

## 🎮 Funciones Personalizadas

### Sonidos Personalizados
```c
    // Al configurar los parámetros del juego
    struct SnakeOpt snakeOpt = {
        .snakeNcplane = plane,
        .width = 60,      // Ancho del mapa; de manera similar, una cadena ancha hace que el ancho real sea 60*2=160
        .height = 30,     // Altura del mapa; asegúrate de que el plano reserve al menos 3 filas para la barra de estado. La altura real renderizada es 30+3+1=34.
        .fade = true,     // Habilita el efecto de degradado de color del mapa
        .level = 1,       // Nivel inicial
        .isMusicOn = true // Habilita la música de fondo; solo una serpiente puede habilitar música.
    };
    strcpy(opts.musicPath, "/path/to/custom_sounds"); // Puedes personalizar el directorio de sonidos; la música interna posterior será de este directorio. Si especificas un directorio relativo, será relativo al directorio del programa.
```

### Más:

[Crear Varios Planos de Juego](TEST/TestDynamic/test.c)

[Definiciones de Archivos de Cabecera](src/notcurses-snake.h)

## 🔧 Sugerencias Básicas de Comandos de Compilación

```makefile
# Usar bibliotecas estáticas, necesita vincular SDL2; especificar la biblioteca estática con -Wl,--whole-archive -l:libnotcurses-snake.a, la biblioteca dinámica con -Wl,--no-whole-archive; debe incluir al menos -lnotcurses -lnotcurses-core -lSDL2 -lSDL2_mixer
gcc -std=c2x -D_XOPEN_SOURCE=600 -D_GNU_SOURCE \
test.c \
-Wl,--whole-archive -l:libnotcurses-snake.a \
-Wl,--no-whole-archive -lnotcurses -lnotcurses-core -lSDL2 -lSDL2_mixer \
-o test

# Usar bibliotecas dinámicas, tamaño de software más pequeño
gcc test.c -o test \
    -lnotcurses-snake \
    -lnotcurses \
    -lnotcurses-core \
    -D_XOPEN_SOURCE=600 \
    -D_GNU_SOURCE \
    -std=c23
```

## ⚠️ Problemas de Compatibilidad Conocidos con Terminales

| Entorno de Terminal     | Estado de Soporte | Nota Especial                                                          |
| ----------------------- | ----------------- | ----------------------------------------------------------------------- |
| **Kitty**               | ✅ Mejor           | Terminal acelerado por GPU; debes habilitar el debounce manualmente, como usando la función `SnakeInputDebounceControl`. |
| Terminal Integrado de VSCode | ❌ Incompatible  | Versiones 1.85.3 y superiores, notcurses no es compatible |

[Más](https://github.com/dankamongmen/notcurses/blob/master/TERMINALS.md)

## 📜 Información de Licencia

Este proyecto está bajo la [Licencia Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0).