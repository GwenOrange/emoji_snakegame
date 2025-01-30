# Emoji SnakeGame

[![Apache License 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![C23 Ready](https://img.shields.io/badge/C-23-green)](https://en.cppreference.com/w/c/23)

![Game Demo](demo.gif)

`emoji_snakegame` is a TUI snake game extension library based on [Notcurses](https://github.com/dankamongmen/notcurses). It allows you to quickly integrate an interactive snake game plane into any Notcurses program running in fullscreen mode.

🇬🇧 [English](README.md) | 🇨🇳 [中文](README_ZH.md) | 🇨🇳 [简中](README_ZH_CN.md) 
|------|------|------|
🇵🇹 [Português](README_PT.md) | 🇷🇺 [Русский](README_RU.md) | 🇪🇸 [Español](README_ES.md)



## 🌟 Core Features

- 🎮 Multi-threading, allowing you to start as many snake planes as you want.
- 🎵 Real-time sound support (using SDL2_mixer).
- 🕹️ Enhanced gameplay with more features compared to the original snake game. The snake has items and skills.
- 🎨 Emoji graphics (requires terminal font support).

## 🎮 Game Overview
![alt text](image-1.png)

You find yourself in a colorful Emoji world, as the cute Emoji snake 😋, embarking on an exciting adventure! Your goal is to collect enough delicious oranges 🍊 within a specified time and overcome various challenges along the way. This magical maze is full of surprises and fun, so come and play, and you will surely succeed in this adventure.

Score points by collecting enough oranges 🍊 within the specified time 🕛. When you achieve the goal 🎯, you win the game. Otherwise, if your character dies for any reason (including running out of time), you fail the game.

## 💥 Rich Item System

### 🔪 Destroyer: Your Terrifying Weapon!
This powerful item can smash through walls blocking your path, opening up new routes. Each time you successfully destroy 5 walls, you will receive extra points! If you acquire a Debuff ability and then immediately get the '🔪', it will reset your character to normal. But be careful, because the annoying burning tomato 🍅 can limit your destructive ability. The burning tomato 🍅 will suppress the destroyer ability and grant you extraordinary speed 🤯. Can you choose between speed and destruction to achieve your goal?

### ⌛ Hourglass: The Magical Time Control Tool
By pressing the `k` key or `space`, you can pause time and even rewind the past! This will help you easily deal with various crises. However, those pesky mushrooms 🍄 can disrupt your time control ability. The '🍄' will slow down your character's speed, but not too much, while also scrambling your movement direction 😱. The status bar will be disabled, and after a certain time, your character will suddenly regain original speed and direction. Can you react quickly enough?

### 🧲 Magnet: The Magical Scoring Booster
Its greatest feature is that it not only significantly increases your score but also helps you refresh items! No matter what item you pick up, it will give you bonus points. The only trouble is that when the annoying bug 🐛 appears, it might disturb you. The '🐛' will make you feel nauseous 🤮 when you pick up food, and if you are in 🧲 mode, picking any item or food will deduct points.

## 🎯 Game Objective
Successfully collect enough oranges 🍊 within the specified time; otherwise, the game will fail.

## 🕹️ Controls
- Use `WASD` or arrow keys to move.
- Press `k` or `space` to activate time stop ability.
- Click `Tab` or `Enter` to pause the game.
- Press `Esc` to exit the game.

## 🧩 Obstacles and Challenges
Those terrifying stone sculptures 🗿 are quite a hassle. However, if you obtain the power of the destroyer, they will turn into easily destructible potatoes 🥔! Hurry up and destroy them within the specified time! Otherwise, you will be trapped by walls, leading to a game over.

## 🚀 How to Launch
To start playing, enter the following command in a terminal that supports Emoji:
```bash
emoji-snakegame n
```
where `n` is the level number.

## 📦 Installation Guide
#### Compilation Recommendations

- **GCC 9+**
- **std=c11+**
- **Red Hat-based Linux systems**
- **Debian-based Linux systems**

| Installation Content | Installation Path                           |
| ------------------- | ------------------------------------------- |
| Executable File     | /usr/bin/emoji_snakegame                   |
| Dynamic/Static Library | /usr/local/lib/libnotcurses-snake**    |
| Header File         | /usr/local/include/notcurses-snake.h      |
| Sound Resources     | /usr/local/share/notcurses-snake-sounds   |

### Build from Source
```bash
# Extract and enter the project directory
tar -xvf emoji-snake.tar.xz
cd emoji-snake

# Initialize build environment and install packages (automatic detection of APT/DNF)
bash setup-dev-env.sh

# Compile and install
sudo make && sudo make install

# Verify installation
emoji_snakegame v
```
### Uninstall
```bash
sudo make uninstall
```

## Compile and Link Snake to Your TUI Program
The following content will tell you how to use the libnotcurses-snake library to embed the game into your TUI program.

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
        .width = 60,      // Map width; similarly, a wide string causes the actual width to be 60*2=160
        .height = 30,     // Map height; make sure the plane reserves at least 3 rows for the status bar. The actual render height is 30+3+1=34.
        .fade = true,     // Enable map color gradient effect
        .level = 1,       // Initial level
        .isMusicOn = true // Enable background music; only one snake can enable music.
    };

    // Initialize game engine
    struct SnakeState *game = SnakeGameInit(nc, &snakeOpt);
    // Timestamp variable for debounce handling
    long long last_key_timestamp = 0;
    // Main game loop
    while (SnakeGameIsRunning(game))
    {
        struct ncinput ni;
        if (notcurses_get_nblock(nc, &ni))
        {
            // Input debounce handling to prevent repeated inputs for menu options; you can also use this elsewhere
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
                SnakeGameInput(game); // Note that SnakeGameInput has built-in debounce, no additional delay mechanism is needed.
            }
        }

        // Rendering logic
        if (SnakeGameShouldRender(game))
        {
            notcurses_render(nc);
            SnakeGameUnlockRender(game);
        }

        usleep(50000); // 50ms refresh cycle
    }

    // Clean up resources
    SnakeGameAllClean();
    notcurses_stop(nc);
    return 0;
}
```

## 🎮 Custom Features

### Custom Sounds
```c
    // When configuring game parameters
    struct SnakeOpt snakeOpt = {
        .snakeNcplane = plane,
        .width = 60,      // Map width; similarly, a wide string causes the actual width to be 60*2=160
        .height = 30,     // Map height; make sure the plane reserves at least 3 rows for the status bar. The actual render height is 30+3+1=34.
        .fade = true,     // Enable map color gradient effect
        .level = 1,       // Initial level
        .isMusicOn = true // Enable background music; only one snake can enable music.
    };
    strcpy(opts.musicPath, "/path/to/custom_sounds"); // You can customize the sound directory; subsequent internal music will be sourced from this directory. If you specify a relative directory, it will be relative to the program directory.
```

### More:

[Create Multiple Game Planes](TEST/TestDynamic/test.c)

[Header File Definitions](src/notcurses-snake.h)

## 🔧 Basic Compilation Command Suggestions

```makefile
# Using static libraries, need to link SDL2; specify static library with -Wl,--whole-archive -l:libnotcurses-snake.a, dynamic library with -Wl,--no-whole-archive; must include -lnotcurses -lnotcurses-core -lSDL2 -lSDL2_mixer
gcc -std=c2x -D_XOPEN_SOURCE=600 -D_GNU_SOURCE \
test.c \
-Wl,--whole-archive -l:libnotcurses-snake.a \
-Wl,--no-whole-archive -lnotcurses -lnotcurses-core -lSDL2 -lSDL2_mixer \
-o test

# Using dynamic libraries, smaller program size
gcc test.c -o test \
    -lnotcurses-snake \
    -lnotcurses \
    -lnotcurses-core \
    -D_XOPEN_SOURCE=600 \
    -D_GNU_SOURCE \
    -std=c23
```

## ⚠️ Known Terminal Compatibility Issues

| Terminal Environment    | Support Status | Special Note                                                             |
| ----------------------- | -------------- | ----------------------------------------------------------------------- |
| **Kitty**               | ✅ Best        | Graphics accelerated terminal; you must manually enable debounce, such as using `SnakeInputDebounceControl` function. |
| VSCode Built-in Terminal | ❌ Incompatible | Versions 1.85.3 and above, notcurses is incompatible; versions 1.85.2 and below, notcurses is incompatible. |

[More](https://github.com/dankamongmen/notcurses/blob/master/TERMINALS.md)

## 📜 License Information

This project is licensed under [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).