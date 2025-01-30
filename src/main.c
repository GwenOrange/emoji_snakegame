#include "notcurses-snake.h"
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <notcurses/notcurses.h>
#include <locale.h>
#include <pthread.h>
#include <stdatomic.h>
static sigset_t original_mask; // 保存原始信号掩码 Store original signal mask

// 阻塞终端信号 | Block terminal signals
void block_terminal_signals()
{
    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGWINCH);
    sigaddset(&sigmask, SIGINT);  // Ctrl+C
    sigaddset(&sigmask, SIGQUIT); // Ctrl+\'
    sigaddset(&sigmask, SIGTSTP); // Ctrl+Z
    // 使用线程安全函数设置信号掩码 | Use thread-safe function
    pthread_sigmask(SIG_BLOCK, &sigmask, &original_mask);
}

// 恢复信号处理 | Restore signal handling
void restore_signal_handling()
{
    pthread_sigmask(SIG_SETMASK, &original_mask, NULL);
}

int main(int argc, char *argv[])
{
    // Block signals first | 在初始化任何库之前阻塞信号
    block_terminal_signals();

    unsigned level = 1;
    if (argc > 1)
    {
        if (strstr(argv[1], "v"))
        {
            printf("Version %.2lf\n", SNAKE_VERSION);
            return 0;
        }
        level = atoi(argv[1]);
    }

    // Set locale | 设置本地化支持
    setlocale(LC_ALL, "en_US.UTF-8");

    // Initialize Notcurses | 初始化Notcurses
    struct notcurses *base_nc;
    struct notcurses_options opts = {0};
    opts.flags = NCOPTION_SUPPRESS_BANNERS | NCOPTION_NO_ALTERNATE_SCREEN;
    
    // Keep original terminal contents | 重要：禁用备用屏幕保留原终端内容
    if (!(base_nc = notcurses_init(&opts, NULL)))
    {
        fprintf(stderr, "Notcurses init failed\n");
        return EXIT_FAILURE;
    }

    // Create game plane | 创建游戏平面
    struct ncplane *std_plane = notcurses_stdplane(base_nc);
    ncplane_set_base(std_plane, "", 0, 0); // Transparent background //透明背景

    // Configure game // 配置游戏参数
    struct SnakeOpt snakeOpt = {
        .snakeNcplane = std_plane,
        .width = 60,
        .height = 30,
        .fade = true,
        .level = level,
        .isMusicOn = true};

    // First game initialization // 第一个游戏初始化
    struct SnakeState *game = SnakeGameInit(base_nc, &snakeOpt);
    if (!game)
    {
        notcurses_stop(base_nc);
        return EXIT_FAILURE;
    }

    // First game main loop // 第一个游戏主循环
    ncinput ni;
    long long last_key_timestamp = 0;
    bool key_timestamp=false;
    while (snakeGameIsRunning(game))
    {
        // Non-blocking input | 非阻塞获取输入
        if (notcurses_get_nblock(base_nc, &ni))
        {
            key_timestamp=SnakeInputDebounceControl(&last_key_timestamp,500000LL);
            if ((ni.id == NCKEY_ENTER || ni.id == NCKEY_TAB)&&key_timestamp)
            {
                SnakeGameTryPause(game);
            }else if((ni.id==NCKEY_ESC||(ni.modifiers&NCKEY_MOD_CTRL&&(ni.utf8[0]=='c'||ni.utf8[0]=='C')))&&key_timestamp){
                snakeGameExit(game);
            }
            else
            {
                SnakeGameInput(game, ni);
            }
        }

        // Rendering logic | 渲染逻辑
        if (SnakeGameShouldRender(game))
        {
            notcurses_render(base_nc);
            SnakeGameUnlockRender(game);
        }
        usleep(1000 * 50); // 50ms refresh rate | 50ms刷新率
    }

    SnakeGameAllClean();



    // Stop Notcurses // 停止Notcurses
    notcurses_stop(base_nc);


    // Restore signal handling // 恢复信号处理
    restore_signal_handling();

    return EXIT_SUCCESS;
}


