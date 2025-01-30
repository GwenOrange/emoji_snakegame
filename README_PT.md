# Emoji SnakeGame

[![Apache License 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![C23 Ready](https://img.shields.io/badge/C-23-green)](https://en.cppreference.com/w/c/23)

![游戏演示](demo.gif)

`emoji_snakegame` é uma biblioteca de extensão de jogo TUI baseada em [Notcurses](https://github.com/dankamongmen/notcurses). Ela permite que você integre rapidamente um plano de jogo de cobra interativo em qualquer programa Notcurses executando em modo de tela cheia.

🇬🇧 [English](README.md) | 🇨🇳 [中文](README_ZH.md) | 🇨🇳 [简中](README_ZH_CN.md) 
|------|------|------|
🇵🇹 [Português](README_PT.md) | 🇷🇺 [Русский](README_RU.md) | 🇪🇸 [Español](README_ES.md)

## 🌟 Recursos Principais

- 🎮 Multi-threading, permitindo que você inicie quantos planos de cobra desejar.
- 🎵 Suporte a efeitos sonoros em tempo real (usando SDL2_mixer).
- 🕹️ Jogabilidade aprimorada com mais recursos em comparação com o jogo de cobra original. A cobra possui itens e habilidades.
- 🎨 Gráficos de Emoji (requer suporte a fontes no terminal).

## 🎮 Visão Geral do Jogo
![alt text](image-1.png)

Você se encontra em um mundo colorido de Emojis, como a adorável cobra Emoji 😋, embarcando em uma emocionante aventura! Seu objetivo é coletar suficientes laranjas 🍊 deliciosas dentro de um tempo especificado e superar vários desafios ao longo do caminho. Este labirinto mágico está cheio de surpresas e diversão, então venha jogar, e você com certeza terá sucesso nesta aventura.

Pontue coletando o suficiente de laranjas 🍊 dentro do tempo especificado 🕛. Quando você atingir o objetivo 🎯, você vence o jogo. Caso contrário, se seu personagem morrer por qualquer motivo (incluindo ficar sem tempo), você perde o jogo.

## 💥 Sistema de Itens Rico

### 🔪 Destruidor: Sua Arma Aterrorizante!
Este poderoso item pode destruir paredes que bloqueiam seu caminho, abrindo novas rotas. Cada vez que você destrói 5 paredes com sucesso, receberá pontos extras! Se você adquirir uma habilidade Debuff e depois imediatamente pegar o '🔪', isso resetará seu personagem para o normal. Mas tenha cuidado, pois o chato tomate queimado 🍅 pode limitar sua habilidade de destruição. O tomate queimado 🍅 suprimirá a habilidade do destruidor e lhe dará uma velocidade extraordinária 🤯. Você consegue escolher entre velocidade e destruição para alcançar seu objetivo?

### ⌛ Ampulheta: A Mágica Ferramenta de Controle do Tempo
Ao pressionar a tecla `k` ou `espaço`, você pode pausar o tempo e até retroceder o passado! Isso ajudará você a lidar facilmente com várias crises. No entanto, aqueles cogumelos chatos 🍄 podem atrapalhar sua habilidade de controle do tempo. O '🍄' diminuirá a velocidade do seu personagem, mas não muito, enquanto embaralha a direção do seu movimento 😱. A barra de status será desativada, e após um certo tempo, seu personagem recuperará de repente a velocidade e a direção originais. Você consegue reagir a tempo?

### 🧲 Ímã: O Acelerador Mágico de Pontos
Sua maior característica é que não só aumenta significativamente sua pontuação, mas também ajuda a renovar itens! Não importa qual item você pegue, ele lhe dará pontos bônus. O único problema é que quando o inseto chato 🐛 aparecer, ele pode te incomodar. O '🐛' fará você se sentir enjoado 🤮 ao pegar comida, e se você estiver no estado 🧲, pegar qualquer item ou comida resultará em perda de pontos.

## 🎯 Objetivo do Jogo
Coletar com sucesso o suficiente de laranjas 🍊 dentro do tempo especificado; caso contrário, o jogo falhará.

## 🕹️ Controles
- Use `WASD` ou as teclas de seta para mover.
- Pressione `k` ou `espaço` para ativar a habilidade de parar o tempo.
- Clique em `Tab` ou `Enter` para pausar o jogo.
- Pressione `Esc` para sair do jogo.

## 🧩 Obstáculos e Desafios
Aquelas aterradoras esculturas de pedra 🗿 são um grande problema. No entanto, se você obtiver o poder do destruidor, elas se transformarão em batatas facilmente destrutíveis 🥔! Apresse-se para destruí-las dentro do tempo especificado! Caso contrário, você será preso por paredes, resultando em um game over.

## 🚀 Como Iniciar
Para começar a jogar, digite o seguinte comando em um terminal que suporte Emoji:
```bash
emoji-snakegame n
```
onde `n` é o número do nível.

## 📦 Guia de Instalação
#### Recomendações de Compilação

- **GCC 9+**
- **std=c11+**
- **Sistemas Linux baseados em Red Hat**
- **Sistemas Linux baseados em Debian**

| Conteúdo da Instalação | Caminho da Instalação                          |
| ---------------------- | ---------------------------------------------- |
| Arquivo Executável     | /usr/bin/emoji_snakegame                      |
| Biblioteca Dinâmica/Estática | /usr/local/lib/libnotcurses-snake**   |
| Arquivo de Cabeçalho   | /usr/local/include/notcurses-snake.h         |
| Recursos Sonoros       | /usr/local/share/notcurses-snake-sounds      |

### Compilar a Partir do Código Fonte
```bash
# Extrair e entrar no diretório do projeto
tar -xvf emoji-snake.tar.xz
cd emoji-snake

# Inicializar o ambiente de compilação e instalar pacotes (detecção automática de APT/DNF)
bash setup-dev-env.sh

# Compilar e instalar
sudo make && sudo make install

# Verificar a instalação
emoji_snakegame v
```
### Desinstalar
```bash
sudo make uninstall
```

## Compilar e Linkar Snake ao Seu Programa TUI
O seguinte conteúdo vai te ensinar a usar a biblioteca libnotcurses-snake para embutir o jogo no seu programa TUI.

## 🛠️ Integração Rápida

### Exemplo
```c
#include <notcurses-snake.h>
#include <notcurses/notcurses.h>

int main()
{
    // Inicializa o Notcurses
    struct notcurses *nc = notcurses_init(NULL, stdout);
    struct ncplane *plane = notcurses_stdplane(nc);

    // Configura parâmetros do jogo
    struct SnakeOpt snakeOpt = {
        .snakeNcplane = plane,
        .width = 60,      // Largura do mapa; da mesma forma, uma string larga faz a largura real ser 60*2=160
        .height = 30,     // Altura do mapa; certifique-se de que o plano reserva pelo menos 3 linhas para a barra de status. A altura real renderizada é 30+3+1=34.
        .fade = true,     // Habilita o efeito de gradiente de cor do mapa
        .level = 1,       // Nível inicial
        .isMusicOn = true // Habilita música de fundo; apenas uma cobra pode habilitar música.
    };

    // Inicializa o motor do jogo
    struct SnakeState *game = SnakeGameInit(nc, &snakeOpt);
    // Variável de timestamp para controle de debounce
    long long last_key_timestamp = 0;
    // Loop principal do jogo
    while (SnakeGameIsRunning(game))
    {
        struct ncinput ni;
        if (notcurses_get_nblock(nc, &ni))
        {
            // Controle de debounce de entrada para evitar entradas repetidas em opções de menu; você também pode usar isso em outros lugares
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
                SnakeGameInput(game); // Nota: SnakeGameInput já possui debounce embutido, não é necessário um mecanismo de atraso adicional.
            }
        }

        // Lógica de renderização
        if (SnakeGameShouldRender(game))
        {
            notcurses_render(nc);
            SnakeGameUnlockRender(game);
        }

        usleep(50000); // Ciclo de atualização de 50ms
    }

    // Limpa recursos
    SnakeGameAllClean();
    notcurses_stop(nc);
    return 0;
}
```

## 🎮 Funcionalidades Personalizadas

### Sons Personalizados
```c
    // Ao configurar parâmetros do jogo
    struct SnakeOpt snakeOpt = {
        .snakeNcplane = plane,
        .width = 60,      // Largura do mapa; da mesma forma, uma string larga faz a largura real ser 60*2=160
        .height = 30,     // Altura do mapa; certifique-se de que o plano reserva pelo menos 3 linhas para a barra de status. A altura real renderizada é 30+3+1=34.
        .fade = true,     // Habilita o efeito de gradiente de cor do mapa
        .level = 1,       // Nível inicial
        .isMusicOn = true // Habilita música de fundo; apenas uma cobra pode habilitar música.
    };
    strcpy(opts.musicPath, "/path/to/custom_sounds"); // Você pode personalizar o diretório de sons; a música interna subsequente será proveniente deste diretório. Se você especificar um diretório relativo, ele será relativo ao diretório do programa.
```

### Mais:

[Criar Múltiplos Planos de Jogo](TEST/TestDynamic/test.c)

[Definições de Arquivo de Cabeçalho](src/notcurses-snake.h)

## 🔧 Sugestões Básicas de Comandos de Compilação

```makefile
# Usando bibliotecas estáticas, precisa vincular SDL2; especifique a biblioteca estática com -Wl,--whole-archive -l:libnotcurses-snake.a, a biblioteca dinâmica com -Wl,--no-whole-archive; deve incluir -lnotcurses -lnotcurses-core -lSDL2 -lSDL2_mixer
gcc -std=c2x -D_XOPEN_SOURCE=600 -D_GNU_SOURCE \
test.c \
-Wl,--whole-archive -l:libnotcurses-snake.a \
-Wl,--no-whole-archive -lnotcurses -lnotcurses-core -lSDL2 -lSDL2_mixer \
-o test

# Usando bibliotecas dinâmicas, tamanho de software menor
gcc test.c -o test \
    -lnotcurses-snake \
    -lnotcurses \
    -lnotcurses-core \
    -D_XOPEN_SOURCE=600 \
    -D_GNU_SOURCE \
    -std=c23
```

## ⚠️ Problemas Conhecidos de Compatibilidade com Terminais

| Ambiente de Terminal     | Status de Suporte | Observação Especial                                                       |
| ------------------------ | ----------------- | ------------------------------------------------------------------------ |
| **Kitty**                | ✅ Melhor          | Terminal com aceleração gráfica; você deve habilitar debounce manualmente, como usando a função `SnakeInputDebounceControl`. |
| Terminal Integrado do VSCode | ❌ Incompatível  | Versões 1.85.3 e superiores, notcurses é incompatível|

[Mais](https://github.com/dankamongmen/notcurses/blob/master/TERMINALS.md)

## 📜 Informações de Licença

Este projeto é licenciado sob a [Licença Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0).