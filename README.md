<<<<<<< HEAD
# Emoji SnakeGame

[![Apache License 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![C23 Ready](https://img.shields.io/badge/C-23-green)](https://en.cppreference.com/w/c/23)

![Game Demo](demo.gif)

`emoji_snackgame` is a TUI Snake game extension library based on [Notcurses](https://github.com/dankamongmen/notcurses). It allows rapid integration of interactive Snake game planes into any fullscreen Notcurses program.

## 🌟 Core Features

- 🎮 Multi-threaded - Create as many snake planes as needed
- 🎵 Real-time sound effects support (using SDL2_mixer)
- 🕹️ Enhanced gameplay with power-ups and skills beyond classic Snake
- 🎨 Emoji graphics (requires terminal font support)

## 🎮 Game Overview
![alt text](image-1.png)

Embark on a colorful Emoji adventure as a cute snake 😋! Navigate through magical mazes to collect delicious oranges 🍊 within time limits. Overcome obstacles and challenges to achieve victory. The game ends when either:
- Success: Collect required oranges before timer expires 🎯
- Failure: Character dies (including timeout) ❌

## 💥 Rich Power-up System

### 🔪 Destroyer: Ultimate Wall Breaker
- Smash through walls (5 destroyed walls = bonus points)
- Resets debuffs when collected
- Warning: Burning tomatoes 🍅 will disable destruction ability but grant super speed

### ⌛ Time Turner: Temporal Manipulator
- Press `k`/`Space` to pause/rewind time
- Mushrooms 🍄 will disrupt time control:
  - Slows movement
  - Randomizes direction
  - Disables status bar
  - Sudden recovery after duration

### 🧲 Magnet: Score Multiplier
- Boost scores and refresh items
- Warning: Bugs 🐛 will invert scoring:
  - Lose points when collecting items
  - Nausea effect during consumption

## 🎯 Objective
Collect sufficient oranges 🍊 within time limit to win.

## 🕹️ Controls
- Movement: `WASD`/Arrow keys
- Time stop: `k`/`Space` 
- Pause: `Tab`/`Enter`
- Quit: `Esc`

## 🧩 Obstacles & Challenges
Stone statues 🗿 become destructible potatoes 🥔 when Destroyer is active. Destroy them quickly before they revert!

## 🚀 Launch
Run in Emoji-capable terminal:
```bash
emoji-snakegame n
```
(n = level number)

## 📦 Installation

#### Build Requirements
- **GCC 9+**
- **std=c11+**
- **RHEL-based Linux**
- **Debian-based Linux**

| Component       | Installation Path                     |
|-----------------|---------------------------------------|
| Executable      | /usr/bin/emoji_snakegame              |
| Libraries       | /usr/local/lib/libnotcurses-snake**   |
| Headers         | /usr/local/include/notcurses-snake.h  |
| Sound Resources | /usr/local/share/notcurses-snake-sounds |

### Build from Source
```bash
# Extract and enter directory
tar -xvf emoji-snake.tar.xz
cd emoji-snake

# Initialize dev environment (auto-detects APT/DNF)
bash setup-dev-env.sh

# Compile & install
sudo make && sudo make install

# Verify installation
emoji_snackgame v
```

### Uninstall
```bash
sudo make uninstall
```

# Integrating into TUI Programs
Learn how to embed the game using libnotcurses-snake.

## 🛠️ Quick Integration

### Example
```c
#include <notcurses-snake.h>
#include <notcurses/notcurses.h>

int main()
{
    // Initialize Notcurses
    struct notcurses *nc = notcurses_init(NULL, stdout);
    struct ncplane *plane = notcurses_stdplane(nc);

    // Configure game parameters
    struct SnakeOpt snakeOpt = {
        .snakeNcplane = plane,
        .width = 60,      // Map width (actual width: 60*2=160)
        .height = 30,     // Map height (reserve +3 lines for status bar)
        .fade = true,     Enable gradient effects
        .level = 1,       // Initial level
        .isMusicOn = true // Enable BGM (only one instance can use music)
    };

    // Initialize game engine
    struct SnakeState *game = SnakeGameInit(nc, &snakeOpt);
    long long last_key_timestamp = 0; // Debouncing
    
    // Main loop
    while (SnakeGameIsRunning(game))
    {
        struct ncinput ni;
        if (notcurses_get_nblock(nc, &ni))
        {
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
                SnakeGameInput(game, ni); // Built-in debouncing
            }
        }

        // Rendering
        if (SnakeGameShouldRender(game))
        {
            notcurses_render(nc);
            SnakeGameUnlockRender(game);
        }

        usleep(50000); // 50ms refresh
    }

    // Cleanup
    SnakeGameAllClean();
    notcurses_stop(nc);
    return 0;
}
```

## 🎮 Customization

### Custom Sounds
```c
struct SnakeOpt snakeOpt = {
    // ...other options...
    .isMusicOn = true
};
strcpy(opts.musicPath, "/path/to/custom_sounds"); // Absolute path recommended
```

### More Examples:
[Multi-plane Implementation](TEST/TestDynamic/test.c)

[Header Definitions](src/notcurses-snake.h)

## 🔧 Build Commands

```makefile
# Static linking (requires SDL2)
gcc test.c -o test -L/usr/local/lib \
-Wl,-Bstatic \
    -lnotcurses-snake \
-Wl,-Bdynamic \
    -lnotcurses \
    -lnotcurses-core \
    -lSDL2 \
    -lSDL2_mixer \
    -D_XOPEN_SOURCE=600 \
    -D_GNU_SOURCE \
    -std=c23

# Dynamic linking (smaller binary)
gcc test.c -o test \
    -lnotcurses-snake \
    -lnotcurses \
    -lnotcurses-core \
    -D_XOPEN_SOURCE=600 \
    -D_GNU_SOURCE \
    -std=c23
```

## ⚠️ Terminal Compatibility

| Terminal         | Support  | Notes                                  |
|------------------|----------|----------------------------------------|
| **Kitty**        | ✅ Best   | Enable debounce manually               |
| VSCode Terminal  | ❌ Broken | All versions incompatible with Notcurses |

[More Details](https://github.com/dankamongmen/notcurses/blob/master/TERMINALS.md)

## 📜 License

Apache License 2.0 - [Full Text](https://www.apache.org/licenses/LICENSE-2.0)
=======
# emoji_snakegame
`emoji_snackgame` is a TUI Snake game extension library based on [Notcurses](https://github.com/dankamongmen/notcurses). It allows rapid integration of interactive Snake game planes into any fullscreen Notcurses program
>>>>>>> 119bcac (Initial commit)
