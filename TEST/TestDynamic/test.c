#include <notcurses-snake.h>
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
#define SNAKE_VERSION 0.01f
static sigset_t original_mask; // 保存原始信号掩码 Store original signal mask

// 阻塞终端信号 | Block terminal signals
void block_terminal_signals()
{
    sigset_t sigmask;
    sigemptyset(&sigmask);
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
    bool firstGameRunning = true;

    if (!game)
    {
        notcurses_stop(base_nc);
        return EXIT_FAILURE;
    }

    // First game main loop // 第一个游戏主循环
    ncinput ni;
    long long last_key_timestamp=0;
    bool key_timestamp=false;
    while (firstGameRunning)
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

        // 检查游戏是否结束
        if(snakeGameIsRunning(game)==false){
            break;
        }

        usleep(1000 * 50); // 50ms refresh rate | 50ms刷新率
    }

    SnakeGameAllClean();

    // Reset input structure // 重置输入结构
    ncplane_erase(std_plane);
    memset(&ni, 0, sizeof(ni));
    int game2_width = 20;
    int game2_height = 30;
    // Create plane configurations for multiple games //创建多个游戏平面的配置
    struct ncplane_options nopt1 = {
        .y = 0,
        .x = 0,
        .rows = game2_height + 4,
        .cols = game2_width * 2,
        .flags = 0};

    struct ncplane_options nopt2 = {
        .y = 0,
        .x = game2_width * 2,
        .rows = game2_height + 4,
        .cols = game2_width * 2,
        .flags = 0};

    // Create new game planes // 创建新的游戏平面
    struct ncplane *plane2 = ncplane_create(std_plane, &nopt1);
    struct ncplane *plane3 = ncplane_create(std_plane, &nopt2);

    // Configure options for multiple games // 配置多个游戏的选项
    struct SnakeOpt snake2Opt = {
        .snakeNcplane = plane2,
        .width = game2_width,
        .height = game2_height,
        .fade = true,
        .level = level,
        .isMusicOn = true};

    struct SnakeOpt snake3Opt = {
        .snakeNcplane = plane3,
        .width = game2_width,
        .height = game2_height,
        .fade = true,
        .level = level,
        .isMusicOn = true};

    // Initialize multiple games // 初始化多个游戏
    struct SnakeState *game1 = SnakeGameInit(base_nc, &snake2Opt);
    struct SnakeState *game2 = SnakeGameInit(base_nc, &snake3Opt);

    // Multiple games main loop // 多游戏主循环
    bool multiGameRunning = true;
    bool render1 = false, render2 = false;
    key_timestamp=SnakeInputDebounceControl(&last_key_timestamp,500000LL);
    while (multiGameRunning)
    {
        // Non-blocking input // 非阻塞获取输入
        if (notcurses_get_nblock(base_nc, &ni))
        {
            key_timestamp=SnakeInputDebounceControl(&last_key_timestamp,500000LL);
            if ((ni.id == NCKEY_ENTER || ni.id == NCKEY_TAB)&&key_timestamp)
            {
                SnakeGameTryPause(game1);
                SnakeGameTryPause(game2);
            }else if((ni.id==NCKEY_ESC||(ni.modifiers&NCKEY_MOD_CTRL&&(ni.utf8[0]=='c'||ni.utf8[0]=='C')))&&key_timestamp){
                snakeGameExit(game1);
                snakeGameExit(game2);
            }
            else
            {
                SnakeGameInput(game1, ni);
                SnakeGameInput(game2, ni);
            }
        }

        // Rendering logic // 渲染逻辑
        if (SnakeGameShouldRender(game1)||snakeGameIsRunning(game2)==false|| snakeGameIsPaused(game2)==false)
        {
            render1 = 1;
        }
        else
        {
            render1 = 0;
        }
        if (SnakeGameShouldRender(game2)||snakeGameIsRunning(game1)==false||snakeGameIsPaused(game1)==false)
        {
            render2 = 1;
        }
        else
        {
            render2 = 0;
        }

        if (render1&&render2)
        {
            notcurses_render(base_nc);
            SnakeGameUnlockRender(game1);
            SnakeGameUnlockRender(game2);
        }

        // Check whether all game are over // 检查游戏是否都结束 
        if (snakeGameIsRunning(game1)==false && snakeGameIsRunning(game2)==false)
        {
            multiGameRunning = false;
        }
        usleep(1000);
    }
    sleep(1);
    // Global cleanup // 全局清理
    SnakeGameAllClean();

    // Stop Notcurses // 停止Notcurses
    notcurses_stop(base_nc);


    // Restore signal handling // 恢复信号处理
    restore_signal_handling();

    return EXIT_SUCCESS;
}


