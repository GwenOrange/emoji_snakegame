// APACHE LICENSE 2.0
#ifndef NOTCURSES_SNAKE_H
#define NOTCURSES_SNAKE_H

#ifdef __cplusplus
extern "C"
{
#endif
#define SNAKE_SIZEOF_BUFFER_INPUT 14
#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <linux/limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>
  struct SnakeState;

  /*
   * @brief Terminal capabilities structure // 终端能力结构体
   * @param[in] color_count Number of colors // 颜色数量
   * @param[in] height Terminal height // 终端高度
   * @param[in] width Terminal width // 终端宽度
   * @param[in] can_fade Supports fading // 是否支持渐变
   * @param[in] can_change_palette Can change palette // 是否可修改调色板
   * @param[in] supports_rgb Supports direct RGB // 是否支持直接RGB
   * @param[in] can_pixel Supports pixel rendering // 是否支持像素渲染
   * @param[in] can_openimages Supports loading images // 是否支持加载图片
   * @param[in] can_openvideos Supports loading videos // 是否支持加载视频
   * @param[in] tstyle_support Supported text styles // 支持的文本样式
   */
  typedef struct TerminalCapabilities
  {
    unsigned int color_count; // Number of colors // 颜色数量
    unsigned int height;      // Terminal height // 终端高度
    unsigned int width;       // Terminal width // 终端宽度
    bool can_fade;            // Supports fading // 是否支持渐变
    bool can_change_palette;  // Can change palette // 是否可修改调色板
    bool supports_rgb;        // Supports direct RGB // 是否支持直接RGB
    bool can_pixel;           // Supports pixel rendering // 是否支持像素渲染
    bool can_openimages;      // Supports loading images // 是否支持加载图片
    bool can_openvideos;      // Supports loading videos // 是否支持加载视频
    struct tstyle_support
    {
      bool bold;      // Bold text style // 粗体文本样式
      bool underline; // Underline text style // 下划线文本样式
      bool italic;    // Italic text style // 斜体文本样式
      bool struck;    // Strikethrough text style // 删除线文本样式
      bool undercurl; // Undercurl text style // 下波浪线文本样式
    } tstyle_support;

  } TerminalCapabilities;

  // Used for greedy snake initialization options // 用于贪吃蛇初始化选项
  struct SnakeOpt
  {
    struct ncplane *snakeNcplane; // Pointer to the ncplane for the snake // 蛇的ncplane指针
    int width;                    // Width of the game area // 游戏区域宽度
    int height;                   // Height of the game area // 游戏区域高度
    bool fade;                    // Should fade effect be applied? // 是否应用渐变效果
    unsigned level;               // Current level // 当前关卡
    bool isMusicOn;               // Is music on? // 开启音乐吗？
    char musicPath[PATH_MAX];     // Find files in the default folder, specify the file, specify the folder, specify The folder calculates the relative directory of the executable program//若不进行指定，在默认文件夹寻找文件,指定文件夹以可执行程序相对目录进行计算
  };

  /*
   * @brief Get terminal capabilities // 获取终端能力
   * @param nc Standard notcurses instance // 标准notcurses实例
   * @param caps Terminal capabilities structure // 终端能力结构体
   */
  void SNAKE_get_terminal_capabilities(struct notcurses *nc, TerminalCapabilities *caps);
  /*
   * @brief Snake game input // 蛇游戏输入
   * @param state Snake game state // 蛇游戏状态
   * @param ncInput Input // 输入
   */
  void SnakeGameInput(struct SnakeState *state, const struct ncinput ncInput);

  /**
   * @brief Initialize the greedy snake game state // 初始化贪吃蛇游戏状态
   *
   * @param base_nc Base notcurses window // 基础notcurses窗口
   * @param opt Game configuration options // 游戏配置选项
   *
   * @return struct SnakeState* Pointer to the initialized game state on success, NULL on failure // 初始化成功的游戏状态指针，失败返回NULL
   *
   * @note Performs game state initialization, thread creation, etc. // 执行游戏状态初始化、线程创建等操作
   * @warning If the plane size is smaller than the required drawing ideal width and height, // 如果平面大小比需要绘制理想宽度高度小，
   *          the width and height of the plane will be used, which may lead to incorrect levels. // 则会使用平面的宽度和高度，这可能会导致关卡不正确。
   *          Note that the plane will reserve 3 rows for the plane status bar. // 注意平面会预留3行空间给平面状态栏。
   *          The fade effect depends on terminal support. // fade效果由终端支持决定。
   * @note The best instance is to have the plane width be twice the map width and the height be 4 more than the map height, // 最佳实例是平面宽度为地图两倍，高度比地图高4，
   *       so the screen will fill the entire plane. // 这样画面会铺满整个平面。
   */
  struct SnakeState *SnakeGameInit(struct notcurses *base_nc, struct SnakeOpt *opt);

  // Clean up the snake structure, Release the related memory and set the memory to null// 清理贪吃蛇结构，释放相关内存并设置内存为NULL
  void SnakeGameClean(struct SnakeState *state);

  // Pause the snake game function // 暂停贪吃蛇游戏函数
  bool SnakeGameTryPause(struct SnakeState *state);

  /**
   * @brief Global greedy snake game cleanup function // 全局贪吃蛇游戏清理函数
   *
   * Clear all registered Snake game states and release related resources // 清理所有已注册的贪吃蛇游戏状态，释放相关资源
   */
  void SnakeGameAllClean(void);
  // Unlock the rendering lock // 释放渲染锁
  void SnakeGameUnlockRender(struct SnakeState *state);

  // Determine if rendering is needed // 判断是否需要渲染
  bool SnakeGameShouldRender(struct SnakeState *state);

  /**
   * @brief Input debounce control mechanism // 输入防抖动控制机制
   * @param last_key_timestamp Atomic timestamp of last key press // 上一次按键的原子时间戳
   * @param min_interval_us Minimum time interval in microseconds // 最小时间间隔（微秒）
   * @param suggested_interval_us Suggested time interval in microseconds=50000LL// 建议时间间隔（微秒）microseconds=50000LL
   * @warning In order to compatible most graphic acceleration terminals, use this function to detect whether the input allows input to eliminate the input of the graphic accelerator terminal at a time as multiple bugs.//为了兼容大多数图形加速终端，请用这个函数检测是否允许输入，消除图形加速器终端一次输入视为多次的bug。
   * @return Whether input is allowed // 是否允许输入
   */
  bool SnakeInputDebounceControl(long long *last_key_timestamp, long long min_interval_us);

  // Exit the snake game function // 退出贪吃蛇游戏函数
  void snakeGameExit(struct SnakeState *state);

  // Determine if the snake game is running // 判断贪吃蛇游戏是否正在运行
  bool snakeGameIsRunning(struct SnakeState *state);

  /**
   * @brief Determine if the snake game is paused // 判断贪吃蛇游戏是否暂停
   * @param state The snake game state // 贪吃蛇游戏状态
   * @return Whether the snake game is paused // 是否暂停
   */
  bool snakeGameIsPaused(struct SnakeState *state);
#endif //__NOTCURSES_SNAKE_H