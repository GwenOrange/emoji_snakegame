#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/random.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <ctype.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>
#include <signal.h>
#include <notcurses/notcurses.h>
#include "notcurses-snake.h"
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <pwd.h>
#include <libgen.h>

/////////////////
//? Internal function header declarations //? 内部函数头文件声明
////////////////

#define SNAKE_SHIRE_MUSIC_PATH "/usr/local/share/notcurses-snake-sounds/"

struct SnakeState
{
    struct SnakeGameState
    {
        struct
        {
            int *map;   // Game map // 游戏地图
            int width;  // Map width // 地图宽度
            int height; // Map height // 地图高度
            struct
            {
                int totalIdlePositions; // Total number of idle positions // 所有空闲位置的数量
                int *allIdlePositions;  // Indices of all idle positions // 所有空闲位置的索引

                int nearbyIdlePositions; // Number of nearby idle positions // 近距离空闲位置的数量
                int *nearbyPositions;    // Indices of nearby positions (0 to 8) // 距离0到8的空闲位置索引

                int remoteIdlePositions; // Number of remote idle positions // 远距离空闲位置的数量
                int *remotePositions;    // Indices of remote positions (8 to infinity) // 距离8到无限的空闲位置索引
            } snakePositionByDistance;   // Positions organized by distance // 按距离组织的位置
        } Map;
        struct
        {
            int snakeX;     // Snake's X position // 蛇的X坐标
            int snakeY;     // Snake's Y position // 蛇的Y坐标
            int length;     // Snake's length // 蛇的长度
            int score;      // Current score // 当前分数
            char direction; // Current direction // 当前方向
            struct
            {
                atomic_bool isTimeMoment;       // Time stop status // 时停状态
                atomic_bool isMoreScore;        // Double score status // 加倍积分状态
                atomic_bool isGodOfDestruction; // Double score status // 加倍积分状态
                double moreScoreTimeLeft;       // Remaining time for double score // 加倍积分剩余时间
                double timeStopTimeLeft;        // Remaining time for time stop // 时停时刻剩余时间
                double God_of_Destruction;      // God of Destruction time // 破坏神时间
                int GodlikeScore;               // Godlike score // 破坏神分数
            } Buff;                             // Buff effects // Buff效果
            struct
            {
                atomic_bool is_mushroom; // Has eaten mushroom? // 吃了迷幻蘑菇吗？
                double mushroomTimeLeft; // Remaining time for mushroom effect // 迷幻蘑菇剩余时间
                atomic_bool is_hot;      // Has eaten chili? // 吃了火热番茄吗？
                double hotTimeLeft;      // Remaining time for chili effect // 火热番茄剩余时间

                atomic_bool is_insect; // Has eaten insect? // 吃了虫子吗？
                double insectTimeLeft; // Remaining time for insect effect // 虫子剩余时间
            } Debuff;                  // Debuff effects // Debuff效果
        } Snake;

        atomic_bool gameOver;       // Is the game over? // 游戏是否结束
        atomic_bool gameOverByUser; // The game is over by the user to send the end command//游戏由用户发送结束命令结束
        double timeLeft;            // Countdown timer // 倒计时
        int targetScore;            // Target score // 目标分数
        int level;                  // Current level // 当前关卡
        struct
        {
            int FoodCount;         // Number of food items generated each time // 每次生成食品数量
            int BuffCount;         // Number of Buff items generated each time // 每次生成Buff数量
            int DebuffCount;       // Number of Debuff items generated each time // 每次生成Debuff数量
            int *Food;             // Food items (indices -1 to -99) // 食物-1~-99
            int *Buff;             // Buff items (0 means no item, -201 to -299) // 若为0则代表没有道具,-201~-299
            int *Debuff;           // Debuff items (0 means no item, -301 to -399) // 若为0则代表没有道具-301~-399
        } Level_Item;              // Items that may refresh in this level // 本关道具可能刷新的道具组
        pthread_mutex_t gameMutex; // Mutex for game state
        pthread_cond_t updateCond; // Condition variable for updates
        bool stateChanged;         // Has the state changed? // 状态是否改变
        bool gameWon;              // Is the game won? // 胜利状态
        double timeCount;          // Time stop multiplier // 时停倍率
    } snakeGameState;

    struct SnakePauseState
    {
        atomic_bool isPaused;          // Is the game paused? // 游戏是否暂停
        pthread_mutex_t pauseMutex;    // Mutex for pause state // 暂停状态互斥锁
        pthread_cond_t pauseCond;      // Condition variable to avoid false waiting // 避免假等待
        struct timespec lastPauseTime; // Last pause time // 上次暂停时间
    } snakePauseState;

    struct SnakeInputBuffer
    {
        char buffer[SNAKE_SIZEOF_BUFFER_INPUT]; // Input buffer // 输入缓冲区
        int head;                               // Buffer head // 缓冲区头部
        int tail;                               // Buffer tail // 缓冲区尾部
        pthread_mutex_t mutex;                  // Mutex for input buffer // 输入缓冲区互斥锁
        long long last_key_timestamp;           // Timestamp of last key pressed // 上次按键时间戳
        char last_key;                          // Last key pressed // 上次按下的键
    } snakeInputBuffer;

    struct SnakeRender
    {
        pthread_mutex_t renderMutex; // Mutex for rendering // 渲染互斥锁
        pthread_cond_t renderCond;   // Condition variable for rendering // 渲染条件变量
        bool isRender;               // Is rendering active? // 渲染是否激活
        bool isRunning;              // Is rendering running? // 渲染是否运行中
        bool shouldFade;             // Should fade effect be applied? // 是否应用渐变效果
        unsigned width;              // Width of the render area // 渲染区域宽度
        unsigned height;             // Height of the render area // 渲染区域高度
        struct
        {
            uint64_t left_up;       // Upper left corner // 左上角
            uint64_t right_up;      // Upper right corner // 右上角
            uint64_t left_down;     // Lower left corner // 左下角
            uint64_t right_down;    // Lower right corner // 右下角
            uint64_t snake_fade[3]; // snake fade effects // 蛇食物渐变效果
            int whenfade;           // Fade time // 渐变时间
        } normal;                   // Normal rendering state // 正常渲染状态
        struct
        {
            uint64_t left_up;       // Upper left corner // 左上角
            uint64_t right_up;      // Upper right corner // 右上角
            uint64_t left_down;     // Lower left corner // 左下角
            uint64_t right_down;    // Lower right corner // 右下角
            uint64_t snake_fade[3]; // snake fade effects // 蛇食物渐变效果
        } timemoment;               // Time moment rendering state // 时停渲染状态
        struct
        {
            uint64_t left_up;       // Upper left corner // 左上角
            uint64_t right_up;      // Upper right corner // 右上角
            uint64_t left_down;     // Lower left corner // 左下角
            uint64_t right_down;    // Lower right corner // 右下角
            uint64_t snake_fade[3]; // snake fade effects // 蛇食物渐变效果
        } hot;                      // Hot rendering state // 火热番茄渲染状态
        struct
        {
            uint64_t left_up;       // Upper left corner // 左上角
            uint64_t right_up;      // Upper right corner // 右上角
            uint64_t left_down;     // Lower left corner // 左下角
            uint64_t right_down;    // Lower right corner // 右下角
            uint64_t snake_fade[3]; // snake fade effects // 蛇食物渐变效果
        } infected;                 // Infected rendering state // 中毒渲染状态
    } snakeRender;

    bool isMusicOn;             // Is music on? // 开启音乐吗？
    struct ncplane *snakePlane; // Pointer to the ncplane for rendering // ncplane指针用于渲染
    TerminalCapabilities caps;  // Terminal capabilities // 终端能力
    pthread_t gameThread;       // Game thread ID // 游戏线程ID
    bool isrunning;             // Is the game running? // 游戏是否运行
    char *musicPath;            // Path to the music file // 音乐文件路径
};

/**
 * @brief Thread-safe random number generator // 线程安全随机数生成器
 * @return Random integer value // 随机整数值
 * @note Auto-resets seed periodically: //重设种子周期性：
 *       - Every 1000 calls // 每1000次调用
 *       - Every 60 seconds // 每60秒
 *       - Initial call // 首次调用
 */
static int random_get(void);
// Reset random seed (srand equivalent) // 重设种子，srand
static void random_reset(void);
// Handle game pause state internally // 内部程序进行暂停
static void SnakeGamePauseState(struct SnakeState *state);

/**
 * Process special position values: -1=leftmost/top, -2=rightmost/bottom
 * 处理特殊位置值：-1 是最左/最上面，-2 是最右/最下面
 * @param pos Position value to process // 要处理的位置值
 * @param max Maximum position value (e.g. width/height) // 最大的位置值（比如宽度或高度）
 * @return Processed position value, -1 for invalid positions // 返回处理后的位置值，如果无效位置返回 -1
 */
static int adjustPosition(int pos, int max);

/**
 * Verify wall type validity // 检查墙体的类型对不对
 * @param wallType Wall type value // 墙的类型值
 * @return true if between -199~-101, false otherwise // 如果在 -199 到 -101 之间返回 true，其他情况返回 false
 */
static bool isValidWallType(int wallType);

struct AllOfsnakeState
{
    struct SnakeState *state;
    struct AllOfsnakeState *next;
};

// Global linked list head //全局链表头部
static struct AllOfsnakeState *g_snakeStateHead = NULL;
// Mutex lock protecting global linked list // 保护全局链表的互斥锁
static pthread_mutex_t g_snakeStateMutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Add the Snake game state to the global linked list// 将贪吃蛇游戏状态加入全局链表
 *
 * @param state Snake game state pointer to be added to the linked list //要加入链表的贪吃蛇游戏状态指针
 * @return bool Whether the linked list is successfu //加入链表是否成功
 *
 * @note Thread security function, use mutual lock protection linked list operation //线程安全函数，使用互斥锁保护链表操作
 */
static bool SnakeGameStateJoin(struct SnakeState *state);

/**
 * Set horizontal/vertical wall // 设置水平或者垂直的墙
 * @param state Game state pointer // 游戏状态的指针
 * @param wallType Wall type (-101~-199) // 墙的类型,-101~-199
 * @param position Wall position (-1=leftmost/top, -2=rightmost/bottom) // 墙的位置，-1 最左/最上面，-2 最右/最下面
 * @param isVertical Vertical wall flag (1=vertical) // 是否是垂直墙,1为竖直墙
 * @return true on success // 成功返回 true
 */
static bool setWall(struct SnakeState *state, int wallType, int position, bool isVertical);

/**
 * Draw openable square wall with item placement // 画有开口的方形墙并放置物品
 * @param state Game state pointer // 游戏状态的指针
 * @param wallType Wall type // 墙的类型
 * @param y Start Y (-1=top, -2=bottom) // 方形墙起始Y位置，-1最上面
 * @param x Start X (-1=left, -2=right) // 方形墙起始X位置，-1最左边
 * @param width Width (-1=to rightmost, -2=full) // 宽度，-1到最右，-2整个地图
 * @param height Height (-1=to bottom, -2=full) // 高度，-1到最下，-2整个地图
 * @param openMask Opening flags (0b1000=top) // 开口掩码，0b1000上边
 * @param item Item to place (negative=special) // 要放的道具（负数表示有道具）
 * @return true on success // 成功返回 true
 */
static bool setBoxWall(struct SnakeState *state, int wallType, int y, int x, int width, int height, uint8_t openMask, int item);

/**
 * Clear specified area (set to 0) // 清空指定区域（变成0）
 * @param state Game state pointer // 游戏状态的指针
 * @param y Start Y (-1=top, -2=bottom) // 清空区域起始Y位置
 * @param x Start X (-1=left, -2=right) // 清空区域起始X位置
 * @param width Width (-1=to rightmost, -2=full) // 宽度设置
 * @param height Height (-1=to bottom, -2=full) // 高度设置
 * @return true on success // 成功返回 true
 */
static bool clearArea(struct SnakeState *state, int y, int x, int width, int height);

/**
 * @brief Generate food items // 生成食物道具
 * @param state Snake game state // 蛇的游戏状态指针
 * @param Count Food quantity // 生成的食物数量
 * @param distance Minimum spawn distance // 生成的最小距离限制
 * @return Generation success status // 生成是否成功
 */
static inline bool snakeGenerateItemFood(struct SnakeState *state, int Count, int distance);

/**
 * @brief Generate buff items // 生成Buff道具
 * @param state Snake game state // 蛇的游戏状态指针
 * @param Count Buff quantity // 生成的Buff数量
 * @param distance Minimum spawn distance // 生成的最小距离限制
 * @return Generation success status // 生成是否成功
 */
static inline bool snakeGenerateItemBuff(struct SnakeState *state, int Count, int distance);

/**
 * @brief Generate debuff items // 生成Debuff道具
 * @param state Snake game state // 蛇的游戏状态指针
 * @param Count Debuff quantity // 生成的Debuff数量
 * @param distance Minimum spawn distance // 生成的最小距离限制
 * @return Generation success status // 生成是否成功
 */
static inline bool snakeGenerateItemDebuff(struct SnakeState *state, int Count, int distance);

// Validate move input // 验证移动输入有效性
static bool isValidMove(char current, char next);

// Push input to buffer // 将输入推入缓冲区
static void snakePushInput(struct SnakeState *state, char input);

// Get next direction input // 读取下一个方向操作符
static char snakePopInput(struct SnakeState *state);

// Clear input buffer // 清理输入缓冲区
static void snakeClearInputBuffer(struct SnakeState *state);

/**
 * @brief Set level items with dynamic allocation // 设置关卡道具（动态内存分配）
 * @param state Snake game state // 蛇的游戏状态指针
 * @param items Item array (0-terminated) // 道具数组（0结尾）
 * @param[in] FoodCount Food generation count // 每次生成食物数目
 * @param[in] BuffCount Buff generation count // 每次生成Buff数目
 * @param[in] DebuffCount Debuff generation count // 每次生成Debuff数目
 * @note Item categories: // 道具分类规则：
 *       - Food: 1-99 // 食物：1-99
 *       - Buff: 201-299 // Buff：201-299
 *       - Debuff: 301-399 // Debuff：301-399
 * @warning Requires matching memory release // 需要配套内存释放函数
 */
static void snakeSnakeLevelItem_set(struct SnakeState *state, int *items, int FoodCount, int BuffCount, int DebuffCound);

/**
 * @brief Clear level items (free memory) // 清空关卡道具（释放内存）
 * @param state Snake game state // 蛇的游戏状态指针
 * @note Reset all pointers to NULL // 所有指针置NULL
 * @note Prevent double-free // 避免重复释放
 */
static void snakeSnakeLevelItem_clear(struct SnakeState *state);

/* Classify available map spaces by distance from player
根据到主角距离分类地图空闲位置
@param state Game state pointer // 游戏状态结构体指针
*/
static void snakeCheckAvailableSpaces(struct SnakeState *state);

/* Generate items in specified distance range
在指定距离范围生成物品
@param state Game state pointer // 游戏状态结构体指针
@param items Item array (0-terminated) // 待生成物品数组
@param Count Item quantity // 生成数量
@param distance Range flags // 距离范围标志
@warning Items must be 0-terminated // items数组必须0结尾
@return Generation success // 是否成功生成
*/
static bool snakeGenerateItem(struct SnakeState *state, int *items, int Count, int distance);

/* Clear dynamically allocated map statistics
清理地图统计信息内存
@param state Game state pointer // 游戏状态结构体指针
*/
static void snakeClearAvailableSpaces(struct SnakeState *state);

//! Load game level // 载入关卡
static void initializeGame(struct SnakeState *state);

// Render game interface // 渲染游戏内部
static void render_game(struct SnakeState *state);

// Render game over screen // 渲染游戏结束
static void render_game_over(struct SnakeState *state);

// Render level transition // 渲染进入下一关
static void render_game_next_level(struct SnakeState *state);

//! Main game logic loop (3000+ = infinite time)
// !游戏主循环逻辑（≥3000为无限时间）
static void *updateSnakeState(void *arg);

// Start game engine // 启用游戏
static void *StartSnakeGame(void *the_state);
/////////////////
//? End of internal function declarations //? 内部函数头文件声明结束
////////////////

////?????????????? Path functions // 路径相关函数

/**
 * @brief Normalize file path // 规范化文件路径
 * @param path Input path // 输入路径
 * @return Newly allocated normalized path string // 新分配的标准路径字符串
 * @note Caller must free return value // 调用者必须释放返回值
 */
static char *normalize_path(const char *path)
{
    /* Path components array // 路径组件数组 */
    char **components = NULL;
    int count = 0, capacity = 10;
    char *normalized = NULL;

    if ((components = malloc(capacity * sizeof(char *))) == NULL)
    {
        return NULL;
    }

    /* Handle root directory // 处理根目录 */
    if (path[0] == '/')
    {
        if (asprintf(&normalized, "/") == -1)
        {
            free(components);
            return NULL;
        }
    }
    else
    {
        if ((normalized = strdup("")) == NULL)
        {
            free(components);
            return NULL;
        }
    }

    /* Create modifiable path copy // 创建可修改的路径副本 */
    char *path_copy = strdup(path);
    if (!path_copy)
    {
        free(components);
        free(normalized);
        return NULL;
    }

    /* Use thread-safe strtok_r // 使用线程安全的strtok_r */
    char *saveptr;
    char *token = strtok_r(path_copy, "/", &saveptr);

    while (token)
    {
        if (strcmp(token, ".") != 0)
        {
            if (strcmp(token, "..") == 0)
            {
                /* Handle parent directory // 处理上级目录 */
                if (count > 0)
                {
                    free(components[--count]);
                }
            }
            else
            {
                /* Expand components array // 扩展组件数组 */
                if (count >= capacity)
                {
                    capacity *= 2;
                    char **temp = realloc(components, capacity * sizeof(char *));
                    if (!temp)
                        goto normalize_error;
                    components = temp;
                }

                /* Store path component // 存储路径组件 */
                if ((components[count] = strdup(token)) == NULL)
                {
                    goto normalize_error;
                }
                count++;
            }
        }
        token = strtok_r(NULL, "/", &saveptr);
    }

    free(path_copy);

    /* Rebuild normalized path // 重建规范化路径 */
    for (int i = 0; i < count; i++)
    {
        char *temp;
        int need_slash = (strlen(normalized) > 0 &&
                          normalized[strlen(normalized) - 1] != '/');

        if (asprintf(&temp, "%s%s%s",
                     normalized,
                     need_slash ? "/" : "",
                     components[i]) == -1)
        {
            goto normalize_error;
        }

        free(normalized);
        normalized = temp;
        free(components[i]);
    }

    free(components);

    /* Handle empty path case // 处理空路径情况 */
    if (!normalized || strlen(normalized) == 0)
    {
        free(normalized);
        return strdup(".");
    }

    return normalized;

normalize_error:
    free(path_copy);
    for (int i = 0; i < count; i++)
        free(components[i]);
    free(components);
    free(normalized);
    return NULL;
}

/**
 * @brief Expand tilde-prefixed path // 展开波浪号路径
 * @param path Tilde-containing path // 包含波浪号的路径
 * @return Newly allocated expanded path // 新分配的展开路径
 * @note Supports current user's home // 支持当前用户的home目录
 */
static char *expand_tilde(const char *path)
{
    if (path[0] != '~')
        return strdup(path);

    /* Thread-safe password struct access // 线程安全的密码结构获取 */
    struct passwd pwd, *result;
    size_t bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    char *buf = malloc(bufsize);
    if (!buf)
        return NULL;

    /* Use reentrant function // 使用可重入版本函数 */
    if (getpwuid_r(getuid(), &pwd, buf, bufsize, &result) != 0 || !result)
    {
        free(buf);
        return NULL;
    }

    char *expanded;
    if (asprintf(&expanded, "%s%s", pwd.pw_dir, path + 1) == -1)
    {
        free(buf);
        return NULL;
    }
    free(buf);
    return expanded;
}

/* Global initialization control // 全局初始化控制 */
static pthread_once_t base_once = PTHREAD_ONCE_INIT;
static char *global_base = NULL;
static char *global_music_path = NULL;
/**
 * @brief Initialize executable directory // 初始化可执行文件所在目录
 * @note Via /proc/self/exe // 通过/proc/self/exe获取
 */
static void init_global_base()
{
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1)
        return;

    path[len] = '\0';
    char *dir = dirname(path); // Modifies path content // 该函数会修改path内容

    /* Allocate permanent storage // 分配永久存储 */
    global_base = strdup(dir);
}

/**
 * @brief Get absolute path // 获取绝对路径
 * @param relative_path Relative path // 相对路径
 * @return Newly allocated absolute path // 新分配的绝对路径字符串
 * @note Must be freed by caller // 必须通过free()释放
 * @warning Depends on Linux /proc // 依赖Linux的/proc文件系统
 */
static char *absolutePath(const char *relative_path)
{
    /* One-time initialization // 一次性初始化 */
    pthread_once(&base_once, init_global_base);

    if (!global_base)
        return NULL;

    /* Expand tilde path // 展开波浪号路径 */
    char *expanded_relative = expand_tilde(relative_path);
    if (!expanded_relative)
        return NULL;

    /* Handle absolute path // 处理绝对路径 */
    if (expanded_relative[0] == '/')
    {
        char *result = normalize_path(expanded_relative);
        free(expanded_relative);
        return result;
    }

    /* Combine base and relative path // 拼接基础路径和相对路径 */
    char *combined;
    if (asprintf(&combined, "%s/%s", global_base, expanded_relative) == -1)
    {
        free(expanded_relative);
        return NULL;
    }
    free(expanded_relative);

    /* Normalize final path // 规范化最终路径 */
    char *normalized = normalize_path(combined);
    free(combined);
    return normalized;
}

/**
 * @briefGet the absolute path of the relative path relative to the absolute path // 获取相对路径相对于绝对路径的绝对路径
 * @param relative_path Relative path // 相对路径
 * @param absolute_Path Absolute path // 绝对路径
 * @return Newly allocated absolute path // 新分配的绝对路径字符串
 * @note Must be freed by caller // 必须通过free()释放
 * @warning Depends on Linux /proc // 依赖Linux的/proc文件系统
 */
static char *absolutePath_Re(const char *relative_path, const char *absolute_Path)
{

    if (!absolute_Path)
        return NULL;

    /* Expand tilde path // 展开波浪号路径 */
    char *expanded_relative = expand_tilde(relative_path);
    if (!expanded_relative)
        return NULL;

    /* Handle absolute path // 处理绝对路径 */
    if (expanded_relative[0] == '/')
    {
        char *result = normalize_path(expanded_relative);
        free(expanded_relative);
        return result;
    }

    /* Combine base and relative path // 拼接基础路径和相对路径 */
    char *combined;
    if (asprintf(&combined, "%s/%s", absolute_Path, expanded_relative) == -1)
    {
        free(expanded_relative);
        return NULL;
    }
    free(expanded_relative);

    /* Normalize final path // 规范化最终路径 */
    char *normalized = normalize_path(combined);
    free(combined);
    return normalized;
}

////?????????????? Music Function // 音乐功能模块
static pthread_once_t music_once = PTHREAD_ONCE_INIT;
static atomic_bool *snakeMusicOn = NULL;
static atomic_bool *isDebuffMusic = NULL;
static atomic_bool *audio_cleaned = NULL;

static enum {
    NormalMusic,
    godLikeMusic,
    Hotmusic,
    InsectMusic,
    MushroomMusic
} nowMusicType = 0;

#define BGM_CHANNEL 7
#define SFX_CHANNELS 7

typedef struct
{
    Mix_Chunk *current;
    Mix_Chunk *pending;
    int fade_in_ms;
    bool is_active;
    bool is_paused;
    bool is_fading;
    Uint32 last_switch_time;
} BGM_State;

static BGM_State bgm = {NULL, NULL, 0, false, false, false, 0};
static SDL_mutex *bgm_mutex = NULL;
static Mix_Chunk *sfx_pool[SFX_CHANNELS] = {NULL};
static int bgm_volume = MIX_MAX_VOLUME;
static const Uint32 MIN_SWITCH_INTERVAL = 5;
static void bgm_finished(int channel);
static void audio_cleanup(void);
static void process_bgm_switch()
{
    if (bgm.current && bgm.current != bgm.pending)
    {
        Mix_FreeChunk(bgm.current);
        bgm.current = NULL;
    }
    bgm.current = bgm.pending;
    bgm.pending = NULL;

    if (bgm.current)
    {
        if (bgm.fade_in_ms > 0)
        {
            Mix_FadeInChannel(BGM_CHANNEL, bgm.current, -1, bgm.fade_in_ms);
        }
        else
        {
            Mix_PlayChannel(BGM_CHANNEL, bgm.current, -1);
        }
        Mix_Volume(BGM_CHANNEL, bgm_volume);
    }
    else
    {
        bgm.is_active = false;
    }
}

/**
 * @brief Initialize audio system and SDL mixer 初始化音频系统和SDL混音器
 * @return Boolean indicating successful audio initialization 表示音频初始化是否成功的布尔值
 */
static bool audio_init(char *MusicPath)
{

    for (int i = 0; atomic_load(audio_cleaned) != true; i++)
    {
        if (i == 10)
        {
            audio_cleanup();
            usleep(1000);
            break;
        }
    }
    memset(&bgm, 0, sizeof(bgm));
    // Initialize SDL audio subsystem 初始化SDL音频子系统
    if (SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        return false;
    }
    __sync_synchronize();
    if (global_music_path != NULL)
    {
        free(global_music_path);
        global_music_path = NULL;
    }
    if (MusicPath == NULL)
    {
        global_music_path = strdup(SNAKE_SHIRE_MUSIC_PATH);
    }
    else
    {
        global_music_path = absolutePath(MusicPath);
    }

    atomic_store(audio_cleaned, false);
    atomic_store(snakeMusicOn, true);
    __sync_synchronize();
    // Open audio mixer with specific parameters 以特定参数打开音频混音器
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0)
    {
        SDL_Quit();
        return false;
    }

    // Create mutex for thread-safe audio operations 为线程安全的音频操作创建互斥锁
    bgm_mutex = SDL_CreateMutex();
    if (!bgm_mutex)
    {
        Mix_CloseAudio();
        SDL_Quit();
        return false;
    }

    // Allocate audio channels and set channel finished callback 分配音频通道并设置通道完成回调
    Mix_AllocateChannels(BGM_CHANNEL + 1); // 分配足够的通道
    Mix_ChannelFinished(bgm_finished);

    return true;
}

/**
 * @brief Callback function when background music channel finishes 背景音乐通道播放完成的回调函数
 * @param channel The channel that finished playing 完成播放的通道
 */
static void bgm_finished(int channel)
{
    // Only process for BGM channel 仅处理BGM通道
    if (channel != BGM_CHANNEL)
        return;

    // Use TryLock to avoid potential deadlock 使用TryLock避免潜在的死锁
    if (SDL_TryLockMutex(bgm_mutex) == 0)
    {
        // Process BGM switch if active 如果处于活动状态则处理BGM切换
        if (bgm.is_active)
        {
            process_bgm_switch();
        }
        SDL_UnlockMutex(bgm_mutex);
    }
    else
    {
        // Fallback if unable to acquire lock 无法获取锁时的备用方案
        // Temporarily continue playing current music 临时继续播放当前音乐
        Mix_PlayChannel(BGM_CHANNEL, bgm.current, -1);
    }
}

/**
 * @brief Play sound effect (SFX) from file 从文件播放音效
 * @param path Path to the sound effect file 音效文件路径
 * @return Boolean indicating successful SFX playback 表示音效播放是否成功的布尔值
 */
static bool audio_play_sfx(const char *path)
{
    SDL_LockMutex(bgm_mutex);

    // Convert relative path to absolute path 将相对路径转换为绝对路径
    char *abs_path = absolutePath_Re(path, global_music_path);
    Mix_Chunk *sfx = Mix_LoadWAV(abs_path);
    free(abs_path);

    // Check if SFX loaded successfully 检查音效是否成功加载
    if (!sfx)
    {
        SDL_UnlockMutex(bgm_mutex);
        return false;
    }

    // Find available channel in 0-6 range 在0-6通道中查找可用通道
    for (int i = 0; i < SFX_CHANNELS; ++i)
    {
        if (!Mix_Playing(i) && !Mix_Paused(i))
        {
            // Free existing chunk if present 如果存在则释放现有音效块
            if (sfx_pool[i])
            {
                Mix_FreeChunk(sfx_pool[i]);
            }
            sfx_pool[i] = sfx;
            Mix_PlayChannel(i, sfx, 0);
            SDL_UnlockMutex(bgm_mutex);
            return true;
        }
    }

    // No available channel found 未找到可用通道
    Mix_FreeChunk(sfx);
    SDL_UnlockMutex(bgm_mutex);
    return false;
}
/**
 * @brief Switch background music with fade effects 切换背景音乐并使用淡入淡出效果
 * @param path Music file path 音乐文件路径
 * @param fade_out_ms Fade out duration in milliseconds 淡出持续时间（毫秒）
 * @param fade_in_ms Fade in duration in milliseconds 淡入持续时间（毫秒）
 * @return Boolean indicating successful music switch 表示音乐切换是否成功的布尔值
 */
static bool audio_switch_bgm(const char *path, int fade_out_ms, int fade_in_ms)
{
    SDL_LockMutex(bgm_mutex);

    // Check if enough time has passed since last switch 检查是否距离上次切换已经过足够时间
    Uint32 current_time = SDL_GetTicks();
    if (current_time - bgm.last_switch_time < MIN_SWITCH_INTERVAL)
    {
        SDL_UnlockMutex(bgm_mutex);
        return false;
    }

    // Convert relative path to absolute path 将相对路径转换为绝对路径
    char *abs_path = absolutePath_Re(path, global_music_path);
    Mix_Chunk *new_chunk = Mix_LoadWAV(abs_path);
    free(abs_path);

    // Check if music chunk loaded successfully 检查音乐是否成功加载
    if (!new_chunk)
    {
        SDL_UnlockMutex(bgm_mutex);
        return false;
    }

    // Clean up any pending music chunk 清理任何待定的音乐块
    if (bgm.pending)
    {
        Mix_FreeChunk(bgm.pending);
    }
    bgm.pending = new_chunk;
    bgm.fade_in_ms = fade_in_ms;
    bgm.last_switch_time = current_time;

    // Handle active background music 处理活动背景音乐
    if (bgm.is_active)
    {
        bgm.is_fading = true;
        if (fade_out_ms > 0)
        {
            Mix_FadeOutChannel(BGM_CHANNEL, fade_out_ms);
        }
        else
        {
            Mix_HaltChannel(BGM_CHANNEL);
        }
    }
    else
    {
        bgm.is_active = true;
        process_bgm_switch();
    }

    SDL_UnlockMutex(bgm_mutex);
    return true;
}
/**
 * @brief Clean up and release all audio resources 清理和释放所有音频资源
 * @details Stops all channels, frees BGM and SFX chunks, destroys mutex and closes audio system 停止所有声道，释放BGM和音效资源，销毁互斥锁并关闭音频系统
 */
static void audio_cleanup()
{
    if (atomic_load(audio_cleaned) == true)
    {
        return;
    }
    atomic_store(audio_cleaned, true);

    if (atomic_load(snakeMusicOn) == false)
    {
        return;
    }
    atomic_store(snakeMusicOn, false);

    Mix_HaltChannel(-1);

    SDL_LockMutex(bgm_mutex);

    if (bgm.current)
    {
        Mix_Chunk *temp = bgm.current;
        bgm.current = NULL;
        Mix_FreeChunk(temp);
    }
    if (bgm.pending)
    {
        Mix_Chunk *temp = bgm.pending;
        bgm.pending = NULL;
        Mix_FreeChunk(temp);
    }

    for (int i = 0; i < SFX_CHANNELS; ++i)
    {
        if (sfx_pool[i])
        {
            Mix_Chunk *temp = sfx_pool[i];
            sfx_pool[i] = NULL;
            Mix_FreeChunk(temp);
        }
    }

    SDL_UnlockMutex(bgm_mutex);

    if (bgm_mutex)
    {
        SDL_DestroyMutex(bgm_mutex);
        bgm_mutex = NULL;
    }

    Mix_CloseAudio();
    SDL_Quit();

    memset(&bgm, 0, sizeof(bgm));
}
/**
 * @brief Pause the currently playing background music 暂停当前播放的背景音乐
 * @details Pauses the BGM channel if it is active and not already paused 如果背景音乐处于活动状态且未暂停，则暂停
 */
static void audio_pause_bgm()
{
    SDL_LockMutex(bgm_mutex);
    if (bgm.is_active && !bgm.is_paused)
    {
        Mix_Pause(BGM_CHANNEL);
        bgm.is_paused = true;
    }
    SDL_UnlockMutex(bgm_mutex);
}

/**
 * @brief Resume background music playback 恢复背景音乐播放
 * @details Resumes the paused background music if it is active 如果背景音乐处于暂停状态，则恢复播放
 */
static void audio_resume_bgm()
{
    SDL_LockMutex(bgm_mutex);
    if (bgm.is_active && bgm.is_paused)
    {
        Mix_Resume(BGM_CHANNEL);
        bgm.is_paused = false;
    }
    SDL_UnlockMutex(bgm_mutex);
}

/**
 * @brief Set the volume of background music 设置背景音乐音量
 * @param volume Volume level (0-128) 音量大小（0-128）
 * @details Clamps the volume between 0 and MIX_MAX_VOLUME 将音量限制在0和MIX_MAX_VOLUME之间
 */
static void audio_set_bgm_volume(int volume)
{
    SDL_LockMutex(bgm_mutex);
    bgm_volume = (volume < 0) ? 0 : (volume > MIX_MAX_VOLUME) ? MIX_MAX_VOLUME
                                                              : volume;
    if (bgm.is_active)
    {
        Mix_Volume(BGM_CHANNEL, bgm_volume);
    }
    SDL_UnlockMutex(bgm_mutex);
}

/**
 * @brief Switch back to normal music based on priority // 根据优先级切换回原背景音
 * @param state Game state pointer // 游戏状态指针
 * @param id Current music type ID // 当前音乐类型ID
 */
static void snakeSwitchNormalMusic(struct SnakeState *state, int id)
{
    if (atomic_load(isDebuffMusic) == true)
    {
        if (id > MushroomMusic && atomic_load(&(state->snakeGameState.Snake.Debuff.is_insect)))
        {
            audio_switch_bgm("./BackDebuffMushroom.wav", 500, 1000);
            nowMusicType = MushroomMusic;
        }
        else if (id > InsectMusic && atomic_load(&(state->snakeGameState.Snake.Debuff.is_insect)))
        {
            audio_switch_bgm("./BackDebuffInsect.wav", 500, 1000);
            nowMusicType = InsectMusic;
        }
        else if (id > Hotmusic && atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)))
        {
            audio_switch_bgm("./BackDebuffHot.wav", 500, 1000);
            nowMusicType = Hotmusic;
        }
        else if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)) && atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)))
        {
            audio_switch_bgm("./BackDebuffHot.wav", 500, 1000);
            nowMusicType = Hotmusic;
            atomic_store(isDebuffMusic, false);
        }
        else if (nowMusicType != NormalMusic && atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)) == false && atomic_load(&(state->snakeGameState.Snake.Debuff.is_mushroom)) == false)
        {
            if (atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)))
            {
                audio_switch_bgm("./BackBuffGod.wav", 200, 500);
                nowMusicType = godLikeMusic;
                atomic_store(isDebuffMusic, false);
            }
            else
            {
                nowMusicType = NormalMusic;
                audio_switch_bgm("./BackNormal.wav", 500, 1000);
                atomic_store(isDebuffMusic, false);
            }
        }
    }
    else if (nowMusicType != NormalMusic && atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)) == false && atomic_load(&(state->snakeGameState.Snake.Debuff.is_mushroom)) == false)
    {
        if (atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)))
        {
            audio_switch_bgm("./BackBuffGod.wav", 200, 500);
            nowMusicType = godLikeMusic;
            atomic_store(isDebuffMusic, false);
        }
        else
        {
            nowMusicType = NormalMusic;
            audio_switch_bgm("./BackNormal.wav", 500, 1000);
            atomic_store(isDebuffMusic, false);
        }
    }
    else if (atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)) && nowMusicType == NormalMusic)
    {
        audio_switch_bgm("./BackBuffGod.wav", 200, 500);
        nowMusicType = godLikeMusic;
        atomic_store(isDebuffMusic, false);
    }
    else
    {
        nowMusicType = NormalMusic;
        audio_switch_bgm("./BackNormal.wav", 200, 500);
        atomic_store(isDebuffMusic, false);
    }
}

/**
 * @brief Switch to specified music type with priority handling // 根据优先级切换到指定音乐类型
 * @param state Game state pointer // 游戏状态指针
 * @param id Target music type ID // 目标音乐类型ID
 */
static void snakeSwitchMusicToID(struct SnakeState *state, int id)
{
    // Handle God of Destruction counter items // 处理克制破坏神的道具
    if (id == Hotmusic && nowMusicType < Hotmusic)
    {
        audio_switch_bgm("./BackDebuffHot.wav", 500, 1000);
        nowMusicType = Hotmusic;
        atomic_store(isDebuffMusic, true);
        return;
    }

    // Handle debuffs countered by God of Destruction // 处理被破坏神克制的Debuff道具
    if (atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)) == false || atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)))
    {
        if (id == InsectMusic && nowMusicType < InsectMusic)
        {
            audio_switch_bgm("./BackDebuffInsect.wav", 500, 1000);
            nowMusicType = InsectMusic;
            atomic_store(isDebuffMusic, true);
            return;
        }
        if (id == MushroomMusic && nowMusicType < MushroomMusic)
        {
            audio_switch_bgm("./BackDebuffMushroom.wav", 500, 1000);
            nowMusicType = MushroomMusic;
            atomic_store(isDebuffMusic, true);
            return;
        }
    }

    // Handle God of Destruction buff // 处理破坏神增益
    if (id == godLikeMusic && atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)) == false && nowMusicType != godLikeMusic)
    {
        audio_switch_bgm("./BackBuffGod.wav", 500, 1000);
        nowMusicType = godLikeMusic;
        atomic_store(isDebuffMusic, false);
        return;
    }

    // Fallback to normal music // 回退到普通音乐
    if (id == NormalMusic)
    {
        snakeSwitchNormalMusic(state, -1);
        return;
    }
}

///////////
//??????? Random Number Module // 随机数模块
///////////
/* 声明线程本地变量 */
static __thread unsigned int seed;
static __thread int call_count = 0;
static __thread time_t last_reset_time = 0;

static int random_get(void)
{
    time_t current_time = time(NULL);

    // 重置条件检查
    if (call_count == 0 || call_count >= 1000 ||
        (current_time - last_reset_time) >= 60)
    {
        random_reset();
        call_count = 0;
        last_reset_time = current_time;
    }

    int result = rand_r(&seed);
    call_count++;
    return result;
}

// Enhanced version entropy hybrid function//增强版熵混合函数
static unsigned int entropy_hash(unsigned int a, unsigned int b)
{
    // 基于MurmurHash3的最终混合步骤
    a ^= b;
    a *= 0xcc9e2d51;
    a = (a << 15) | (a >> 17);
    a *= 0x1b873593;
    return a;
}

static void random_reset(void)
{
    struct
    {
        struct timeval tv;
        struct timespec ts;
        pid_t pid;
        pthread_t tid;
        clock_t clk;
        void *stack_addr;
        unsigned hw_counter;
    } entropy;

    // 第一阶段：收集基础熵源
    gettimeofday(&entropy.tv, NULL);
    clock_gettime(CLOCK_MONOTONIC, &entropy.ts);
    entropy.pid = getpid();
    entropy.tid = pthread_self();
    entropy.clk = clock();
    entropy.stack_addr = &entropy; // 利用栈地址随机化
    entropy.hw_counter = 0;

    // 第二阶段：尝试获取硬件熵源
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    __asm__ __volatile__("rdtsc" : "=a"(entropy.hw_counter));
#elif defined(__aarch64__)
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(entropy.hw_counter));
#endif

    // 第三阶段：优先使用系统级随机源
    unsigned int sys_rand = 0;
    if (getrandom(&sys_rand, sizeof(sys_rand), GRND_NONBLOCK) == sizeof(sys_rand))
    {
        seed = sys_rand;
        return;
    }

    // 第四阶段：混合所有熵源
    seed = entropy_hash(entropy.tv.tv_sec, entropy.tv.tv_usec);
    seed = entropy_hash(seed, entropy.ts.tv_nsec);
    seed = entropy_hash(seed, entropy.pid);
    seed = entropy_hash(seed, (unsigned int)entropy.tid);
    seed = entropy_hash(seed, entropy.clk);
    seed = entropy_hash(seed, (uintptr_t)entropy.stack_addr);
    seed = entropy_hash(seed, entropy.hw_counter);

    // 最终强化混合
    seed ^= (seed >> 16);
    seed *= 0x85ebca6b;
    seed ^= (seed >> 13);
    seed *= 0xc2b2ae35;
    seed ^= (seed >> 16);
}

// Handle game pause state with audio control // 处理游戏暂停状态（含音频控制）
static void SnakeGamePauseState(struct SnakeState *state)
{
    pthread_mutex_lock(&state->snakePauseState.pauseMutex);
    if (atomic_load(&state->snakePauseState.isPaused) == false)
    {
        pthread_mutex_unlock(&state->snakePauseState.pauseMutex);
        return;
    }
    // 进入时立即检查游戏结束状态
    if (atomic_load(&(state->snakeGameState.gameOver)))
        goto exit_unpause;

    // 暂停音乐和设置状态
    if (state->isMusicOn)
        audio_pause_bgm();
    atomic_store(&state->snakePauseState.isPaused, true);

    // 设置100ms超时检测（单位：纳秒）
    const long timeout_nsec = 100 * 1000000; // 100毫秒
    struct timespec timeout;
    while (1)
    {
        // 获取当前时间并计算超时点
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_nsec += timeout_nsec;
        if (timeout.tv_nsec >= 1000000000)
        {
            timeout.tv_sec += 1;
            timeout.tv_nsec -= 1000000000;
        }

        if (atomic_load(&(state->snakeGameState.gameOver)) || !atomic_load(&state->snakePauseState.isPaused))
            break;

        int rc = pthread_cond_timedwait(&state->snakePauseState.pauseCond,
                                        &state->snakePauseState.pauseMutex,
                                        &timeout);
        if (rc == ETIMEDOUT)
            continue;
    }

    if (state->isMusicOn)
        audio_resume_bgm();

exit_unpause:
    atomic_store(&state->snakePauseState.isPaused, false);
    pthread_cond_broadcast(&state->snakePauseState.pauseCond);
    pthread_mutex_unlock(&state->snakePauseState.pauseMutex);
}

// Validate move direction transition // 验证移动方向转换有效性
static bool isValidMove(char current, char next)
{
    return !((current == 'a' && next == 'd') ||
             (current == 'd' && next == 'a') ||
             (current == 'w' && next == 's') ||
             (current == 's' && next == 'w'));
}

// Push input to circular buffer // 将输入推入循环缓冲区
static void snakePushInput(struct SnakeState *state, char input)
{
    pthread_mutex_lock(&state->snakeInputBuffer.mutex);

    input = tolower(input);
    int prevIndex = (state->snakeInputBuffer.tail - 1 + SNAKE_SIZEOF_BUFFER_INPUT) % SNAKE_SIZEOF_BUFFER_INPUT;

    // Get previous direction, use current if buffer empty // 获取上一个方向，缓冲区空时使用当前方向
    char lastInput = (state->snakeInputBuffer.tail == state->snakeInputBuffer.head) ? '\0' : state->snakeInputBuffer.buffer[prevIndex];

    if (isValidMove(lastInput, input))
    {
        int nextTail = (state->snakeInputBuffer.tail + 1) % SNAKE_SIZEOF_BUFFER_INPUT;
        if (nextTail == state->snakeInputBuffer.head)
        {
            // Replace last input when buffer full // 缓冲区将满时替换最后一个输入
            state->snakeInputBuffer.buffer[prevIndex] = input;
        }
        else
        {
            state->snakeInputBuffer.buffer[state->snakeInputBuffer.tail] = input;
            state->snakeInputBuffer.tail = nextTail;
        }
    }

    pthread_mutex_unlock(&state->snakeInputBuffer.mutex);
}

// Pop input from buffer // 从缓冲区取出输入
static char snakePopInput(struct SnakeState *state)
{
    pthread_mutex_lock(&state->snakeInputBuffer.mutex);

    char input = 0;
    if (state->snakeInputBuffer.head != state->snakeInputBuffer.tail)
    {
        input = state->snakeInputBuffer.buffer[state->snakeInputBuffer.head];
        state->snakeInputBuffer.head = (state->snakeInputBuffer.head + 1) % SNAKE_SIZEOF_BUFFER_INPUT;
    }

    pthread_mutex_unlock(&state->snakeInputBuffer.mutex);
    return input;
}

// Clear input buffer // 清空输入缓冲区
static void snakeClearInputBuffer(struct SnakeState *state)
{
    pthread_mutex_lock(&state->snakeInputBuffer.mutex);

    // Directly reset buffer pointers // 直接重置头尾指针
    state->snakeInputBuffer.head = 0;
    state->snakeInputBuffer.tail = 0;
    state->snakeInputBuffer.last_key_timestamp = 0;

    pthread_mutex_unlock(&state->snakeInputBuffer.mutex);
}

// Analyze available map positions // 分析地图可用位置
static void snakeCheckAvailableSpaces(struct SnakeState *state)
{
    // Release previous data // 释放之前可能存在的数据
    snakeClearAvailableSpaces(state);

    int maxSize = state->snakeGameState.Map.width * state->snakeGameState.Map.height;

    // Allocate memory for position tracking // 分配内存用于位置跟踪
    state->snakeGameState.Map.snakePositionByDistance.allIdlePositions = malloc(maxSize * sizeof(int));
    state->snakeGameState.Map.snakePositionByDistance.nearbyPositions = malloc(maxSize * sizeof(int));
    state->snakeGameState.Map.snakePositionByDistance.remotePositions = malloc(maxSize * sizeof(int));

    // Initialize counters // 初始化计数器
    state->snakeGameState.Map.snakePositionByDistance.totalIdlePositions = 0;
    state->snakeGameState.Map.snakePositionByDistance.nearbyIdlePositions = 0;
    state->snakeGameState.Map.snakePositionByDistance.remoteIdlePositions = 0;

    // Scan entire map // 遍历整个地图
    for (int y = 0; y < state->snakeGameState.Map.height; y++)
    {
        for (int x = 0; x < state->snakeGameState.Map.width; x++)
        {
            int index = y * state->snakeGameState.Map.width + x;

            if (state->snakeGameState.Map.map[index] == 0)
            {
                // Calculate distance from snake // 计算与蛇的距离
                double dx = x - state->snakeGameState.Snake.snakeX;
                double dy = y - state->snakeGameState.Snake.snakeY;
                double distance = sqrt(dx * dx + dy * dy);

                // Record all idle positions // 记录所有空闲位置
                state->snakeGameState.Map.snakePositionByDistance.allIdlePositions[state->snakeGameState.Map.snakePositionByDistance.totalIdlePositions++] = index;

                // Classify by distance // 根据距离分类
                if (distance <= 8.0)
                {
                    state->snakeGameState.Map.snakePositionByDistance.nearbyPositions[state->snakeGameState.Map.snakePositionByDistance.nearbyIdlePositions++] = index;
                }
                else
                {
                    state->snakeGameState.Map.snakePositionByDistance.remotePositions[state->snakeGameState.Map.snakePositionByDistance.remoteIdlePositions++] = index;
                }
            }
        }
    }
}

// Core item generation logic // 道具生成核心逻辑
static bool snakeGenerateItem(struct SnakeState *state, int *items, int Count, int distance)
{
    // Parameter validation // 参数有效性检查
    if (distance <= 0 || items == NULL || Count <= 0 || distance > 0b11)
    {
        return false;
    }

    // Refresh available positions // 检查可用空间
    snakeCheckAvailableSpaces(state);
    if (state->snakeGameState.Map.snakePositionByDistance.allIdlePositions == 0)
    {
        return false;
    }

    // Calculate valid item types // 计算可用物品数量
    int itemsLength = 0;
    while (items[itemsLength] != 0)
    {
        itemsLength++;
    }
    if (itemsLength == 0)
        return false;

    // Select target position array // 选择目标位置数组
    int *targetPositions = NULL;
    int availableCount = 0;

    if (distance == 0b11)
    {
        targetPositions = state->snakeGameState.Map.snakePositionByDistance.allIdlePositions;
        availableCount = state->snakeGameState.Map.snakePositionByDistance.totalIdlePositions;
    }
    else if (distance == 0b10)
    {
        targetPositions = state->snakeGameState.Map.snakePositionByDistance.remotePositions;
        availableCount = state->snakeGameState.Map.snakePositionByDistance.remoteIdlePositions;
    }
    else if (distance == 0b01)
    {
        targetPositions = state->snakeGameState.Map.snakePositionByDistance.nearbyPositions;
        availableCount = state->snakeGameState.Map.snakePositionByDistance.nearbyIdlePositions;
    }

    if (availableCount == 0)
    {
        return false;
    }

    // Create position copy for sampling // 创建位置副本用于采样
    int *availablePositions = malloc(availableCount * sizeof(int));
    memcpy(availablePositions, targetPositions, availableCount * sizeof(int));

    int generatedCount = 0;
    int remainingCount = availableCount;

    // Generate items through random sampling // 通过随机采样生成道具
    while (remainingCount > 0 && generatedCount < Count)
    {
        int positionIndex = random_get() % remainingCount;
        int selectedPosition = availablePositions[positionIndex];
        int itemIndex = random_get() % itemsLength;

        if (state->snakeGameState.Map.map[selectedPosition] == 0)
        {
            state->snakeGameState.Map.map[selectedPosition] = items[itemIndex];
            generatedCount++;
        }

        // Update sampling pool // 更新采样池
        availablePositions[positionIndex] = availablePositions[remainingCount - 1];
        remainingCount--;
    }

    free(availablePositions);
    return generatedCount > 0;
}

// Generate food items // 生成食物道具
static inline bool snakeGenerateItemFood(struct SnakeState *state, int Count, int distance)
{
    return snakeGenerateItem(state, state->snakeGameState.Level_Item.Food, Count, distance);
}

// Generate buff items // 生成增益道具
static inline bool snakeGenerateItemBuff(struct SnakeState *state, int Count, int distance)
{
    return snakeGenerateItem(state, state->snakeGameState.Level_Item.Buff, Count, distance);
}

// Generate debuff items // 生成减益道具
static inline bool snakeGenerateItemDebuff(struct SnakeState *state, int Count, int distance)
{
    return snakeGenerateItem(state, state->snakeGameState.Level_Item.Debuff, Count, distance);
}

// Clean up map position tracking data // 清理地图位置跟踪数据
static void snakeClearAvailableSpaces(struct SnakeState *state)
{
    if (state->snakeGameState.Map.snakePositionByDistance.allIdlePositions)
    {
        free(state->snakeGameState.Map.snakePositionByDistance.allIdlePositions);
        state->snakeGameState.Map.snakePositionByDistance.allIdlePositions = NULL;
    }

    if (state->snakeGameState.Map.snakePositionByDistance.nearbyPositions)
    {
        free(state->snakeGameState.Map.snakePositionByDistance.nearbyPositions);
        state->snakeGameState.Map.snakePositionByDistance.nearbyPositions = NULL;
    }

    if (state->snakeGameState.Map.snakePositionByDistance.remotePositions)
    {
        free(state->snakeGameState.Map.snakePositionByDistance.remotePositions);
        state->snakeGameState.Map.snakePositionByDistance.remotePositions = NULL;
    }

    // Reset position counters // 重置位置计数器
    state->snakeGameState.Map.snakePositionByDistance.totalIdlePositions = 0;
    state->snakeGameState.Map.snakePositionByDistance.nearbyIdlePositions = 0;
    state->snakeGameState.Map.snakePositionByDistance.remoteIdlePositions = 0;
}

// Handle special position values (-1=leftmost/top, -2=rightmost/bottom) // 处理特殊位置值（-1=最左/最上，-2=最右/最下）
static inline int adjustPosition(int pos, int max)
{
    if (pos >= 0)
        return pos;
    if (pos == -1)
        return 0; // Convert to leftmost/top // 转换为最左/最上位置
    if (pos == -2)
        return max - 1; // Convert to rightmost/bottom // 转换为最右/最下位置
    return -1;          // Invalid position marker // 无效位置标记
}

// Validate wall type range (-199 ~ -101) // 验证墙体类型范围（-199 ~ -101）
static inline bool isValidWallType(int wallType)
{
    return wallType >= -199 && wallType <= -101;
}

// Set vertical/horizontal wall with position validation // 设置垂直/水平墙体（带位置验证）
static bool setWall(struct SnakeState *state, int wallType, int position, bool isVertical)
{
    // Parameter validation // 参数有效性检查
    if (!state || !isValidWallType(wallType))
        return false;

    // Get map dimensions // 获取地图尺寸
    int width = state->snakeGameState.Map.width;
    int height = state->snakeGameState.Map.height;

    // Adjust position with special values // 处理特殊位置值
    position = adjustPosition(position, isVertical ? width : height);
    if (position < 0)
        return false;

    if (isVertical)
    {
        // Vertical wall painting logic // 垂直墙体绘制逻辑
        if (position >= width)
            return false;
        for (int i = 0; i < height; i++)
        {
            state->snakeGameState.Map.map[i * width + position] = wallType;
        }
    }
    else
    {
        // Horizontal wall painting logic // 水平墙体绘制逻辑
        if (position >= height)
            return false;
        for (int i = 0; i < width; i++)
        {
            state->snakeGameState.Map.map[position * width + i] = wallType;
        }
    }
    return true;
}

// Create box wall with specified openings and center item // 创建带开口和中心道具的箱型墙体
static bool setBoxWall(struct SnakeState *state, int wallType, int y, int x, int width, int height, uint8_t openMask, int item)
{
    // Initial validation // 初始验证
    if (!state || !isValidWallType(wallType))
        return false;

    // Get map dimensions // 获取地图尺寸
    int mapWidth = state->snakeGameState.Map.width;
    int mapHeight = state->snakeGameState.Map.height;

    // Process special position values // 处理特殊位置值
    x = adjustPosition(x, mapWidth);
    y = adjustPosition(y, mapHeight);
    if (x < 0 || y < 0)
        return false;

    // Handle width special values // 处理宽度特殊值
    if (width <= 0)
    {
        if (width == -1)
            width = mapWidth - x; // From start to rightmost // 从起点到最右
        else if (width == -2)
            width = mapWidth; // Full map width // 地图全宽
        else
            return false;
    }
    // Handle height special values // 处理高度特殊值
    if (height <= 0)
    {
        if (height == -1)
            height = mapHeight - y; // From start to bottom // 从起点到底部
        else if (height == -2)
            height = mapHeight; // Full map height // 地图全高
        else
            return false;
    }

    // Adjust dimensions to fit map // 调整尺寸适配地图边界
    if (x + width > mapWidth)
        width = mapWidth - x;
    if (y + height > mapHeight)
        height = mapHeight - y;
    if (width <= 2 || height <= 2) // Minimum size check // 最小尺寸检查
        return false;

    // Draw top/bottom edges with opening check // 绘制上下边（检查开口）
    for (int i = x; i < x + width; i++)
    {
        if (!(openMask & 0b1000)) // Top edge opening mask // 上边开口掩码
        {
            state->snakeGameState.Map.map[y * mapWidth + i] = wallType;
        }
        if (!(openMask & 0b0100)) // Bottom edge opening mask // 下边开口掩码
        {
            state->snakeGameState.Map.map[(y + height - 1) * mapWidth + i] = wallType;
        }
    }

    // Draw left/right edges with opening check // 绘制左右边（检查开口）
    for (int i = y; i < y + height; i++)
    {
        if (!(openMask & 0b0010)) // Left edge opening mask // 左边开口掩码
        {
            state->snakeGameState.Map.map[i * mapWidth + x] = wallType;
        }
        if (!(openMask & 0b0001)) // Right edge opening mask // 右边开口掩码
        {
            state->snakeGameState.Map.map[i * mapWidth + (x + width - 1)] = wallType;
        }
    }

    // Place center item if specified // 放置指定中心道具
    if (item < 0)
    {
        int centerX = x + width / 2;
        int centerY = y + height / 2;
        // Validate center position // 验证中心位置有效性
        if (centerX > x && centerX < x + width - 1 &&
            centerY > y && centerY < y + height - 1)
        {
            state->snakeGameState.Map.map[centerY * mapWidth + centerX] = item;
        }
    }

    return true;
}

// Create solid filled box wall (no openings) // 创建实心填充箱型墙体（无开口）
static bool setFilledBoxWall(struct SnakeState *state, int wallType, int y, int x, int width, int height)
{
    if (!state || !isValidWallType(wallType))
        return false;

    int mapWidth = state->snakeGameState.Map.width;
    int mapHeight = state->snakeGameState.Map.height;

    x = adjustPosition(x, mapWidth);
    y = adjustPosition(y, mapHeight);
    if (x < 0 || y < 0)
        return false;

    if (width <= 0)
    {
        if (width == -1)
            width = mapWidth - x;
        else if (width == -2)
            width = mapWidth;
        else
            return false;
    }
    if (height <= 0)
    {
        if (height == -1)
            height = mapHeight - y;
        else if (height == -2)
            height = mapHeight;
        else
            return false;
    }

    if (x + width > mapWidth)
        width = mapWidth - x;
    if (y + height > mapHeight)
        height = mapHeight - y;
    if (width <= 0 || height <= 0)
        return false;

    // Fill entire area with wall type // 用墙体类型填充整个区域
    for (int i = y; i < y + height; i++)
    {
        for (int j = x; j < x + width; j++)
        {
            state->snakeGameState.Map.map[i * mapWidth + j] = wallType;
        }
    }

    return true;
}

// Clear specified area (set to 0) // 清空指定区域（设置为0）
static bool clearArea(struct SnakeState *state, int y, int x, int width, int height)
{
    if (!state)
        return false;

    int mapWidth = state->snakeGameState.Map.width;
    int mapHeight = state->snakeGameState.Map.height;

    x = adjustPosition(x, mapWidth);
    y = adjustPosition(y, mapHeight);
    if (x < 0 || y < 0)
        return false;

    if (width <= 0)
    {
        if (width == -1)
            width = mapWidth - x;
        else if (width == -2)
            width = mapWidth;
        else
            return false;
    }
    if (height <= 0)
    {
        if (height == -1)
            height = mapHeight - y;
        else if (height == -2)
            height = mapHeight;
        else
            return false;
    }

    if (x + width > mapWidth)
        width = mapWidth - x;
    if (y + height > mapHeight)
        height = mapHeight - y;

    // Clear target area cells // 清空目标区域单元格
    for (int i = y; i < y + height; i++)
    {
        for (int j = x; j < x + width; j++)
        {
            state->snakeGameState.Map.map[i * mapWidth + j] = 0;
        }
    }

    return true;
}

// Reset level item configuration // 重置关卡道具配置
static void snakeSnakeLevelItem_clear(struct SnakeState *state)
{
    state->snakeGameState.Level_Item.BuffCount = 0;
    state->snakeGameState.Level_Item.FoodCount = 0;
    state->snakeGameState.Level_Item.DebuffCount = 0;
    if (state->snakeGameState.Level_Item.Buff != NULL)
    {
        free(state->snakeGameState.Level_Item.Buff);
        state->snakeGameState.Level_Item.Buff = NULL;
    }
    if (state->snakeGameState.Level_Item.Debuff != NULL)
    {
        free(state->snakeGameState.Level_Item.Debuff);
        state->snakeGameState.Level_Item.Debuff = NULL;
    }
    if (state->snakeGameState.Level_Item.Food != NULL)
    {
        free(state->snakeGameState.Level_Item.Food);
        state->snakeGameState.Level_Item.Food = NULL;
    }
    return;
}

// Configure level item parameters // 配置关卡道具参数
static void snakeSnakeLevelItem_set(struct SnakeState *state, int *items, int FoodCount, int BuffCount, int DebuffCound)
{
    int item = -1;
    int food_count = 1, buff_count = 0, debuff_count = 0;

    // Clear existing configuration first // 先清空现有配置
    snakeSnakeLevelItem_clear(state);

    // Set generation counts (absolute values) // 设置生成次数（取绝对值）
    state->snakeGameState.Level_Item.BuffCount = abs(BuffCount);
    state->snakeGameState.Level_Item.FoodCount = abs(FoodCount);
    state->snakeGameState.Level_Item.DebuffCount = abs(DebuffCound);

    // First pass: count item categories // 第一次遍历：统计道具类别
    int *temp = items;
    while (*temp != 0)
    {
        item = abs(*temp);
        if (item >= 1 && item <= 99)
            food_count++; // Food items // 食物道具
        if (item >= 201 && item <= 299)
            buff_count++; // Buff items // 增益道具
        if (item >= 301 && item <= 399)
            debuff_count++; // Debuff items // 减益道具
        temp++;
    }

    // Allocate memory for item arrays // 为道具数组分配内存
    state->snakeGameState.Level_Item.Food = malloc(sizeof(int) * (food_count + 1));
    state->snakeGameState.Level_Item.Buff = malloc(sizeof(int) * (buff_count + 1));
    state->snakeGameState.Level_Item.Debuff = malloc(sizeof(int) * (debuff_count + 1));

    // Reset counters for population // 重置填充计数器
    food_count = 1;
    buff_count = 0;
    debuff_count = 0;

    // Default food item // 默认食物道具
    state->snakeGameState.Level_Item.Food[0] = -1;

    // Second pass: populate item arrays // 第二次遍历：填充道具数组
    while (*items != 0)
    {
        item = abs(*items);

        // Categorize and store items // 分类存储道具
        if (item >= 1 && item <= 99)
        {
            state->snakeGameState.Level_Item.Food[food_count++] = -item;
        }
        if (item >= 201 && item <= 299)
        {
            state->snakeGameState.Level_Item.Buff[buff_count++] = -item;
        }
        if (item >= 301 && item <= 399)
        {
            state->snakeGameState.Level_Item.Debuff[debuff_count++] = -item;
        }

        items++;
    }

    // Add termination markers // 添加结束标记
    if (state->snakeGameState.Level_Item.Food)
    {
        state->snakeGameState.Level_Item.Food[food_count] = 0;
    }
    if (state->snakeGameState.Level_Item.Buff)
    {
        state->snakeGameState.Level_Item.Buff[buff_count] = 0;
    }
    if (state->snakeGameState.Level_Item.Debuff)
    {
        state->snakeGameState.Level_Item.Debuff[debuff_count] = 0;
    }
    return;
}

//! Load game levels // 载入游戏关卡
static void initializeGame(struct SnakeState *state)
{
    int width = state->snakeGameState.Map.width;
    int height = state->snakeGameState.Map.height;

    // Allocate map memory if not initialized // 如果地图还没有被分配内存，则分配内存
    if (!state->snakeGameState.Map.map)
    {
        state->snakeGameState.Map.map = (int *)calloc(width * height, sizeof(int));
        if (!state->snakeGameState.Map.map)
        {
            exit(1);
        }
    }

    // Initialize synchronization primitives // 初始化互斥锁和条件变量
    if (pthread_mutex_init(&state->snakeGameState.gameMutex, NULL) != 0 ||
        pthread_cond_init(&state->snakeGameState.updateCond, NULL) != 0)
    {
        exit(1);
    }

    // Set initial snake properties // 设置初始蛇属性
    state->snakeGameState.Snake.snakeX = width / 2;                         // Spawn point // 出生点
    state->snakeGameState.Snake.snakeY = height / 2;                        // Spawn point // 出生点
    state->snakeGameState.Snake.length = 4;                                 // Initial length // 初始长度
    state->snakeGameState.Snake.score = state->snakeGameState.Snake.length; // Initial score // 初始分数
    state->snakeGameState.Snake.direction = 'd';                            // Starting direction // 初始方向

    state->snakeGameState.gameWon = false;
    state->snakeGameState.stateChanged = false;

    // Clear buff/debuff status // 清除BUFF，可能出现的道具等
    memset(&(state->snakeGameState.Snake.Debuff), 0, sizeof(state->snakeGameState.Snake.Debuff));
    memset(&(state->snakeGameState.Snake.Buff), 0, sizeof(state->snakeGameState.Snake.Buff));

    // Set default time stop duration // 每关默认初始时停时间
    state->snakeGameState.Snake.Buff.timeStopTimeLeft = 5;
    state->snakeGameState.stateChanged = false;

    // Default game parameters // 默认时间和目标分数
    state->snakeGameState.timeLeft = 3000.0;
    state->snakeGameState.targetScore = (state->snakeGameState.Map.width) * (state->snakeGameState.Map.height);

    // Level-specific configurations // 根据关卡设置特定配置
    switch (state->snakeGameState.level)
    {
    case 1: // Basic Tutorial // 认识贪吃蛇
        state->snakeGameState.timeLeft = 3000.0;
        state->snakeGameState.targetScore = 24;
        state->snakeGameState.Snake.Buff.timeStopTimeLeft = 30;
        state->snakeGameState.Snake.snakeX = 1; // Top-left spawn // 出生点
        state->snakeGameState.Snake.snakeY = 1;
        snakeSnakeLevelItem_set(state, (int[]){-1, 0}, 0, 0, 0);
        snakeGenerateItemFood(state, 20, 0b11);
        break;

    case 2:
        state->snakeGameState.timeLeft = 60;
        state->snakeGameState.targetScore = 30;
        state->snakeGameState.Snake.Buff.timeStopTimeLeft = 30;
        state->snakeGameState.Snake.snakeX = 1;
        state->snakeGameState.Snake.snakeY = 1;
        // Create box obstacles // 创建箱型障碍
        setBoxWall(state, -101, 22, 30, 40, 50, 0b0101, 0);
        setBoxWall(state, -101, 0, 33, 20, 10, 0b1001, 0);
        snakeSnakeLevelItem_set(state, (int[]){-1, 0}, 1, 0, 0);
        snakeGenerateItemFood(state, 5, 0b11);
        break;

    case 3: // Virus Invasion // 病毒入侵
        state->snakeGameState.timeLeft = 90.0;
        state->snakeGameState.targetScore = 70;
        int centerX = width / 2, centerY = height / 2;
        setBoxWall(state, -101, centerY - 8, centerX - 8, 16, 16, 0b1111, -201);
        clearArea(state, 2, 2, 8, 8);
        clearArea(state, height - 10, width - 10, 8, 8);
        snakeSnakeLevelItem_set(state, (int[]){-1, -202, -203, -303, 0}, 2, 3, 2);
        snakeGenerateItemFood(state, 15, 0b11);
        snakeGenerateItemDebuff(state, 5, 0b01);
        break;

    case 4: // Time Attack // 争分夺秒
        state->snakeGameState.timeLeft = 40.0;
        state->snakeGameState.targetScore = 25;

        setWall(state, -101, -1, false);
        setWall(state, -101, -2, false);
        setWall(state, -101, width / 2, true);
        snakeSnakeLevelItem_set(state, (int[]){-1, -201, 301, 0}, 0, 1, 30);
        snakeGenerateItemFood(state, 30, 0b11);
        snakeGenerateItemDebuff(state, 10, 0b11);
        snakeGenerateItemBuff(state, 3, 0b01);
        break;

    case 5: // War God // 战神
    {
        state->snakeGameState.timeLeft = 70.0;
        state->snakeGameState.targetScore = 100;
        // Create perimeter walls // 创建边界墙
        setWall(state, -101, -1, false);       // Top wall // 上墙
        setWall(state, -101, -2, false);       // Bottom wall // 下墙
        setWall(state, -101, -1, true);        // Left wall // 左墙
        setWall(state, -101, -2, true);        // Right wall // 右墙
        setWall(state, -101, width / 2, true); // Center wall // 中间垂直墙

        // Create quadrant boxes // 大盒子设计
        int boxes[][7] = {
            {3, 3, 10, 10, 0, 0, 0},                   // NW box // 左上大盒子
            {width - 13, 3, 10, 10, 0, 0, 0},          // NE box // 右上大盒子
            {3, height - 13, 10, 10, 0, 0, 0},         // SW box // 左下大盒子
            {width - 13, height - 13, 10, 10, 0, 0, 0} // SE box // 右下大盒子
        };

        for (int boxIndex = 0; boxIndex < 4; boxIndex++)
        {
            int startX = boxes[boxIndex][0];
            int startY = boxes[boxIndex][1];
            int boxWidth = boxes[boxIndex][2];
            int boxHeight = boxes[boxIndex][3];
            setBoxWall(state, -101, startY, startX, boxWidth, boxHeight, 0, -202);
        }

        snakeSnakeLevelItem_set(state, (int[]){-201, -202, -203, -203, -203, -203, -303, 0}, 1, 1, 1);
        snakeGenerateItemFood(state, 10, 0b01);
        snakeGenerateItemBuff(state, 3, 0b01);
    }
    break;

    case 6: // Survival // 绝地求生
        state->snakeGameState.timeLeft = 60.0;
        state->snakeGameState.targetScore = 50;
        setFilledBoxWall(state, -101, 0, 0, width, height);
        state->snakeGameState.Snake.snakeX = width / 2 + 1;
        state->snakeGameState.Snake.snakeY = height / 2 + 1;
        state->snakeGameState.Snake.direction = 'a';
        clearArea(state, (height - 3) / 2, (width - 3) / 2, 8, 8);
        clearArea(state, 2, 2, 10, 10);
        clearArea(state, height - 8, width - 8, 4, 4);
        snakeSnakeLevelItem_set(state, (int[]){-1, -203, 0}, 0, 1, 0);
        snakeGenerateItemFood(state, 20, 0b11);
        state->snakeGameState.Snake.Buff.God_of_Destruction = 10;
        atomic_store(&(state->snakeGameState.Snake.Buff.isGodOfDestruction), true);
        break;

    case 7:
        state->snakeGameState.timeLeft = 60.0;
        state->snakeGameState.targetScore = 150;
        state->snakeGameState.Snake.snakeX = 5;
        state->snakeGameState.Snake.snakeY = 5;

        for (int i = 0; i < 4; i++)
        {
            setBoxWall(state, -101,
                       10, 10,
                       8, 8,
                       8,
                       -302);
        }
        snakeSnakeLevelItem_set(state, (int[]){-201, -202, -302, 0}, 1, 3, 15);
        snakeGenerateItemBuff(state, 4, 0b11);
        snakeGenerateItemFood(state, 20, 0b10);
        break;

    case 8: // Trident // 三叉戟
        state->snakeGameState.timeLeft = 90.0;
        state->snakeGameState.targetScore = 60;

        setWall(state, -101, -1, false);
        setWall(state, -101, -2, false);
        setWall(state, -101, width / 3, true);
        setWall(state, -101, width * 2 / 3, true);

        int boxes[][7] = {
            {width / 4, height / 4, 10, 10, 0, 0, 0},
            {width * 3 / 4 - 10, height * 3 / 4 - 10, 10, 10, 0, 0, 0}};

        for (int boxIndex = 0; boxIndex < 2; boxIndex++)
        {
            setBoxWall(state, -101,
                       boxes[boxIndex][1],
                       boxes[boxIndex][0],
                       boxes[boxIndex][2],
                       boxes[boxIndex][3],
                       0b1000,
                       -303);
        }

        state->snakeGameState.Snake.snakeX = 5;
        state->snakeGameState.Snake.snakeY = 5;

        snakeSnakeLevelItem_set(state, (int[]){-202, 302, 301, 0}, 1, 3, 20);
        snakeGenerateItemFood(state, 6, 0b11);
        snakeGenerateItemBuff(state, 1, 0b01);
        break;

    default: // Classic Mode // 经典模式
    {
        state->snakeGameState.timeLeft = 3000.0;
        state->snakeGameState.targetScore = (width) * (height);
        // Basic walls setup // 设置基础墙壁
        setWall(state, -101, -1, false);       // Top wall // 上墙
        setWall(state, -101, -2, false);       // Bottom wall // 下墙
        setWall(state, -101, width / 2, true); // Center wall // 中间垂直墙

        // Quadrant boxes // 四象限箱体
        int boxes[][7] = {
            {3, 3, 10, 10, 0, 0, 0},                   // NW box // 左上
            {width - 13, 3, 10, 10, 0, 0, 0},          // NE box // 右上
            {3, height - 13, 10, 10, 0, 0, 0},         // SW box // 左下
            {width - 13, height - 13, 10, 10, 0, 0, 0} // SE box // 右下
        };

        for (int boxIndex = 0; boxIndex < 4; boxIndex++)
        {
            int startX = boxes[boxIndex][0];
            int startY = boxes[boxIndex][1];
            int boxWidth = boxes[boxIndex][2];
            int boxHeight = boxes[boxIndex][3];
            setBoxWall(state, -101, startY, startX, boxWidth, boxHeight, 0, 0);
        }

        snakeSnakeLevelItem_set(state, (int[]){-1, -201, -202, -203, -301, -302, -303, 0}, 1, 1, 1);
        snakeGenerateItemBuff(state, 4, 0b11);
        snakeGenerateItemDebuff(state, 4, 0b10);
        snakeGenerateItemFood(state, 5, 0b11);
    }
    break;
    }

    // Post-initialization cleanup // 生成基础食物
    snakeClearInputBuffer(state);
    if (state->isMusicOn)
    {
        audio_set_bgm_volume(100);
        while (atomic_load(audio_cleaned) != false)
            ;
        audio_resume_bgm();
        snakeSwitchMusicToID(state, NormalMusic);
    }
    if (state->snakeGameState.targetScore > width * height)
    {
        state->snakeGameState.targetScore = width * height / 3;
    }
}

void SnakeGameInput(struct SnakeState *state, const struct ncinput ncInput)
{

    if (atomic_load(&state->snakePauseState.isPaused) == true)
        return;
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    long long current_timestamp =
        current_time.tv_sec * 1000000LL + current_time.tv_nsec / 1000;
    pthread_mutex_lock(&state->snakeInputBuffer.mutex);
    long long time_diff = current_timestamp - state->snakeInputBuffer.last_key_timestamp;
    if (time_diff < 125000)
    {
        pthread_mutex_unlock(&state->snakeInputBuffer.mutex);
        return;
    }
    char input;
    if (ncInput.utf8[0] != '\0')
    {
        input = ncInput.utf8[0];
    }
    else
    {
        switch (ncInput.id)
        {
        case NCKEY_LEFT:
            input = 'a';
            break;
        case NCKEY_UP:
            input = 'w';
            break;
        case NCKEY_RIGHT:
            input = 'd';
            break;
        case NCKEY_DOWN:
            input = 's';
            break;
        default:
            pthread_mutex_unlock(&state->snakeInputBuffer.mutex);
            return;
        }
    }

    if (time_diff < 250000 && input == state->snakeInputBuffer.last_key)
    {
        pthread_mutex_unlock(&state->snakeInputBuffer.mutex);
        return;
    }
    state->snakeInputBuffer.last_key = input;
    state->snakeInputBuffer.last_key_timestamp = current_timestamp;
    pthread_mutex_unlock(&state->snakeInputBuffer.mutex);
    switch (input)
    {
    case 'a':
    case 'A':
    case 'd':
    case 'D':
    case 'w':
    case 'W':
    case 's':
    case 'S':
        snakePushInput(state, input);
        break;
    case 'k':
    case 'K':
    case ' ':
    {
        pthread_mutex_lock(&state->snakeGameState.gameMutex);
        if (state->snakeGameState.Snake.Buff.timeStopTimeLeft > 0 &&
            atomic_load(&(state->snakeGameState.Snake.Debuff.is_mushroom)) == false &&
            !atomic_load(&(state->snakeGameState.gameOver)))
        {
            atomic_store(&(state->snakeGameState.Snake.Buff.isTimeMoment), !atomic_load(&(state->snakeGameState.Snake.Buff.isTimeMoment)));
            if (atomic_load(&(state->snakeGameState.Snake.Buff.isTimeMoment)))
            {
                audio_play_sfx("./timeMoment.wav");
                audio_set_bgm_volume(30);
            }
            else
            {
                audio_set_bgm_volume(100);
            }
        }
        pthread_mutex_unlock(&state->snakeGameState.gameMutex);
        break;
    }
    }
}

bool SnakeGameShouldRender(struct SnakeState *state)
{
    pthread_mutex_lock(&state->snakeRender.renderMutex);

    bool result = state->snakeRender.isRender;
    pthread_mutex_unlock(&state->snakeRender.renderMutex);
    return result;
}

void SnakeGameUnlockRender(struct SnakeState *state)
{
    pthread_mutex_lock(&state->snakeRender.renderMutex);
    state->snakeRender.isRender = false;
    pthread_cond_signal(&state->snakeRender.renderCond);
    pthread_mutex_unlock(&state->snakeRender.renderMutex);
}

// Render game interface // 渲染游戏内部
static void render_game(struct SnakeState *state)
{
    pthread_mutex_lock(&state->snakeGameState.gameMutex);
    if (!state->snakeGameState.stateChanged && !state->snakeRender.isRender)
    {
        pthread_mutex_unlock(&state->snakeGameState.gameMutex);
        return;
    }

    // Clear screen // 清屏
    ncplane_erase(state->snakePlane);
    const int mapWidth = state->snakeGameState.Map.width;
    const int mapHeight = state->snakeGameState.Map.height;
    char target[256], states[256];

    for (int x = 0, y = 0; y < 3; y++)
    {
        for (x = 0; x < mapWidth * 2; x++)
        {
            ncplane_putwc_yx(state->snakePlane, y, x, L' ');
        }
    }

    // Format status bar content // 格式化状态栏内容
    if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_mushroom)))
    {
        snprintf(target, 256, "🏆 ∞  🕛∞ 🎯 ∞/∞");
    }
    else if (state->snakeGameState.timeLeft >= 3000)
    {
        snprintf(target, 256, "🏆 %d  🕛∞ 🎯 %d/%d",
                 state->snakeGameState.level,
                 state->snakeGameState.Snake.score,
                 state->snakeGameState.targetScore);
    }
    else
    {
        snprintf(target, 256, "🏆 %3d  🕛 %5.2lf  🎯 %3d/%3d",
                 state->snakeGameState.level,
                 state->snakeGameState.timeLeft,
                 state->snakeGameState.Snake.score,
                 state->snakeGameState.targetScore);
    }

    // Format buff/debuff status // 格式化增益/减益状态
    if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_mushroom)))
    {
        snprintf(states, 256, "🔪 ∞ 🧲 ∞ ⌛ ∞");
    }
    else
    {
        snprintf(states, 256, "🔪 %5.2lf 🧲 %5.2lf ⌛ %5.2lf",
                 atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)) ? state->snakeGameState.Snake.Buff.God_of_Destruction : 0,
                 atomic_load(&(state->snakeGameState.Snake.Buff.isMoreScore)) ? state->snakeGameState.Snake.Buff.moreScoreTimeLeft : 0,
                 state->snakeGameState.Snake.Buff.timeStopTimeLeft > 0 ? state->snakeGameState.Snake.Buff.timeStopTimeLeft : 0);
    }

    // Draw status bar // 绘制状态栏
    ncplane_putstr_aligned(state->snakePlane, 0, NCALIGN_CENTER, target);
    ncplane_putstr_aligned(state->snakePlane, 1, NCALIGN_CENTER, states);

    // Initialize separator line // 初始化分隔线
    static struct nccell nccell_line;
    static bool loadnccell = false;
    if (!loadnccell)
    {
        nccell_init(&nccell_line);
        nccell_load_ucs32(state->snakePlane, &nccell_line, 0x2501);
        loadnccell = true;
    }

    // Draw separator line // 绘制分隔线
    ncplane_cursor_move_yx(state->snakePlane, 2, 0);
    ncplane_hline(state->snakePlane, &nccell_line, 2 * mapWidth);

    // Render game map // 状态栏绘制完毕下面绘制地图
    for (int y = 0; y < mapHeight; y++)
    {
        for (int x = 0; x < mapWidth; x++)
        {
            int value = state->snakeGameState.Map.map[y * mapWidth + x];
            char item[10] = {0};

            // Map value to display character // 地图值映射到显示字符
            __sync_synchronize();
            switch (value)
            {
            case 0:
                strncpy(item, "  ", sizeof(item)); // Empty space // 空地
                break;
            case -1: /* Food items: -1~-99 */
                strncpy(item, "🍊", sizeof(item));
                break;
            case -101: /* Walls: -101~-199 */
                // Wall with special effect // 墙壁
                if (atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)))
                {
                    strncpy(item, "🥔", sizeof(item));
                }
                else
                {
                    strncpy(item, "🗿", sizeof(item));
                }
                break;
            case -201:                             /* Buff items: -201~-299 */
                strncpy(item, "⌛", sizeof(item)); // Time stop // 时停道具
                break;
            case -202:
                strncpy(item, "🧲", sizeof(item)); // Score multiplier // 加倍积分道具
                break;
            case -203:
                strncpy(item, "🔪", sizeof(item)); // God of Destruction // 破坏神时间
                break;
            case -301:                             /* Debuff items: -301~399 */
                strncpy(item, "🍅", sizeof(item)); // Hot pepper //番茄
                break;
            case -302:
                strncpy(item, "🍄", sizeof(item)); // Hallucination mushroom // 迷幻蘑菇
                break;
            case -303:
                strncpy(item, "🐛", sizeof(item)); // Stomach ache // 胃痛
                break;
            default: /* Snake body: positive numbers */
                if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_mushroom)))
                {
                    strncpy(item, "😱", sizeof(item));
                }
                else if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_insect)))
                {
                    strncpy(item, "🤮", sizeof(item));
                }
                else if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)))
                {
                    strncpy(item, "🤯", sizeof(item));
                }
                else if (atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)))
                {
                    if (atomic_load(&(state->snakeGameState.Snake.Buff.isMoreScore)))
                    {
                        strncpy(item, "🤬", sizeof(item));
                    }
                    else
                    {
                        strncpy(item, "😡", sizeof(item));
                    }
                }
                else if (atomic_load(&(state->snakeGameState.Snake.Buff.isMoreScore)))
                {
                    strncpy(item, "🤩", sizeof(item));
                }
                else
                {
                    strncpy(item, "😋", sizeof(item));
                }
                break;
            }
            __sync_synchronize();
            // Draw map cell // 绘制地图单元格
            unsigned int pos_y = y + 3;
            unsigned int pos_x = x * 2;
            ncplane_putstr_yx(state->snakePlane, pos_y, pos_x, item);
        }
    }

    // Skip fade effect if not needed // 不染色就提前返回
    if (!state->snakeRender.shouldFade)
    {
        state->snakeGameState.stateChanged = false;
        state->snakeRender.isRender = true;
        pthread_mutex_unlock(&state->snakeGameState.gameMutex);
        return;
    }

    // Map rendering constants // 地图渲染常量
    const int MAP_OFFSET = 3;
    const int mapRenderWidth = mapWidth * 2;
    const int renderHeadY = state->snakeGameState.Snake.snakeY + MAP_OFFSET;
    const int renderHeadX = state->snakeGameState.Snake.snakeX * 2;

    // Fade effect constants // 渐变效果常量
    const int VERTICAL_SECTION = 3;
    const int CENTER_WIDTH = 2;
    const int HORIZONTAL_SECTION = 3 * 2;

    // Setup fade effect parameters // 设置渐变效果参数
    struct ncplane *FadePlane = state->snakePlane;
    int fadeIndex = (state->snakeRender.normal.whenfade % 12) / 2;
    state->snakeRender.normal.whenfade++;
    uint64_t *snake_fade;
    uint64_t *left_up;
    uint64_t *right_up;
    uint64_t *left_down;
    uint64_t *right_down;

    // Reset fade counter // 重置fade计数器
    if (state->snakeRender.normal.whenfade % 12 == 0)
    {
        state->snakeRender.normal.whenfade = 0;
    }

    // Select fade effect based on game state // 根据游戏状态选择渐变效果
    if (atomic_load(&(state->snakeGameState.Snake.Buff.isTimeMoment)))
    {
        snake_fade = (uint64_t *)&state->snakeRender.timemoment.snake_fade;
        left_up = &state->snakeRender.timemoment.left_up;
        right_up = &state->snakeRender.timemoment.right_up;
        left_down = &state->snakeRender.timemoment.left_down;
        right_down = &state->snakeRender.timemoment.right_down;
    }
    else if (atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)) &&
             atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)) == false)
    {
        snake_fade = (uint64_t *)&state->snakeRender.hot.snake_fade;
        left_up = &state->snakeRender.hot.left_up;
        right_up = &state->snakeRender.hot.right_up;
        left_down = &state->snakeRender.hot.left_down;
        right_down = &state->snakeRender.hot.right_down;
    }
    else if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_insect)) ||
             atomic_load(&(state->snakeGameState.Snake.Debuff.is_mushroom)))
    {
        snake_fade = (uint64_t *)&state->snakeRender.hot.snake_fade;
        left_up = &state->snakeRender.infected.left_up;
        right_up = &state->snakeRender.infected.right_up;
        left_down = &state->snakeRender.infected.left_down;
        right_down = &state->snakeRender.infected.right_down;
    }
    else
    {
        snake_fade = (uint64_t *)&state->snakeRender.normal.snake_fade;
        left_up = &state->snakeRender.normal.left_up;
        right_up = &state->snakeRender.normal.right_up;
        left_down = &state->snakeRender.normal.left_down;
        right_down = &state->snakeRender.normal.right_down;
    }

    // Create symmetric fade effect // 创建对称的渐变效果
    fadeIndex = (fadeIndex >= 3) ? (5 - fadeIndex) : fadeIndex;
    uint64_t fadeColor = snake_fade[fadeIndex];

    // Apply global background fade // 全局背景染色
    __sync_synchronize();
    ncplane_stain(
        FadePlane,
        3,
        0,
        state->snakeRender.height,
        state->snakeRender.width,
        *left_up,   // Top-left // 左上
        *right_up,  // Top-right // 右上
        *left_down, // Bottom-left // 左下
        *right_down // Bottom-right // 右下
    );
    __sync_synchronize();

    // Vertical coloring // 垂直染色
    const int snakeMapY = state->snakeGameState.Snake.snakeY;

    // Vertical fade processing (including wrap-around) // 垂直染色处理（包括环绕）
    for (int i = 1; i <= VERTICAL_SECTION; i++)
    {
        // Upper coloring (handle map top wrap) // 上方染色（处理地图顶部环绕）
        int wrappedMapYUp = (snakeMapY - i + mapHeight) % mapHeight;
        int renderYUp = wrappedMapYUp + MAP_OFFSET;
        ncplane_stain(FadePlane,
                      renderYUp,
                      renderHeadX,
                      1, CENTER_WIDTH,
                      fadeColor, fadeColor, fadeColor, fadeColor);

        // Lower coloring (handle map bottom wrap) // 下方染色（处理地图底部环绕）
        int wrappedMapYDown = (snakeMapY + i) % mapHeight;
        int renderYDown = wrappedMapYDown + MAP_OFFSET;
        ncplane_stain(FadePlane,
                      renderYDown,
                      renderHeadX,
                      1, CENTER_WIDTH,
                      fadeColor, fadeColor, fadeColor, fadeColor);
    }

    // Always color center position (snake head row) // 始终染色中心位置（蛇头所在行）
    ncplane_stain(FadePlane,
                  renderHeadY,
                  renderHeadX,
                  1, CENTER_WIDTH,
                  fadeColor, fadeColor, fadeColor, fadeColor);

    // Horizontal coloring // 水平染色
    // Calculate left start position // 计算左侧起始位置
    int leftStart = renderHeadX - HORIZONTAL_SECTION;
    if (leftStart < 0)
    {
        // If start position is negative, need to start from map right side
        // 如果起始位置为负，需要从地图右侧开始
        int rightPart = -leftStart;
        // Right part // 右侧部分
        ncplane_stain(FadePlane,
                      renderHeadY,
                      mapRenderWidth - rightPart,
                      1, rightPart,
                      fadeColor, fadeColor, fadeColor, fadeColor);
        // Remaining left part // 左侧剩余部分
        if (HORIZONTAL_SECTION - rightPart > 0)
        {
            ncplane_stain(FadePlane,
                          renderHeadY,
                          0,
                          1, HORIZONTAL_SECTION - rightPart,
                          fadeColor, fadeColor, fadeColor, fadeColor);
        }
    }
    else
    {
        ncplane_stain(FadePlane,
                      renderHeadY,
                      leftStart,
                      1, HORIZONTAL_SECTION,
                      fadeColor, fadeColor, fadeColor, fadeColor);
    }

    // Calculate right start position // 计算右侧起始位置
    int rightStart = renderHeadX + CENTER_WIDTH;
    if (rightStart + HORIZONTAL_SECTION > mapRenderWidth)
    {
        // Need to wrap to left side // 需要环绕到左侧
        int firstPart = mapRenderWidth - rightStart;
        // Right part // 右侧部分
        ncplane_stain(FadePlane,
                      renderHeadY,
                      rightStart,
                      1, firstPart,
                      fadeColor, fadeColor, fadeColor, fadeColor);
        // Left wrap part // 左侧环绕部分
        ncplane_stain(FadePlane,
                      renderHeadY,
                      0,
                      1, HORIZONTAL_SECTION - firstPart,
                      fadeColor, fadeColor, fadeColor, fadeColor);
    }
    else
    {
        ncplane_stain(FadePlane,
                      renderHeadY,
                      rightStart,
                      1, HORIZONTAL_SECTION,
                      fadeColor, fadeColor, fadeColor, fadeColor);
    }

    // Update render state // 更新渲染状态
    state->snakeGameState.stateChanged = false;
    state->snakeRender.isRender = true;
    pthread_mutex_unlock(&state->snakeGameState.gameMutex);
}

/**
 * @brief Render game over screen // 渲染游戏结束界面
 * @param state Game state pointer // 游戏状态指针
 */
static void render_game_over(struct SnakeState *state)
{
    pthread_mutex_lock(&state->snakeGameState.gameMutex);
    while (state->snakeRender.isRender)
    {
        pthread_cond_wait(&state->snakeRender.renderCond, &state->snakeRender.renderMutex);
    }

    // Clear screen and show game over message // 清屏并显示游戏结束信息
    ncplane_erase(state->snakePlane);
    char over_str[256];
    snprintf(over_str, 256, "Game Over! Final Score: %d", state->snakeGameState.Snake.score);
    ncplane_putstr_aligned(state->snakePlane, state->snakeGameState.Map.height / 2, NCALIGN_CENTER, over_str);
    state->snakeGameState.stateChanged = false;
    state->snakeRender.isRender = true;
    pthread_mutex_unlock(&state->snakeGameState.gameMutex);
}

/**
 * @brief Render level completion screen // 渲染关卡完成界面
 * @param state Game state pointer // 游戏状态指针
 */
static void render_game_next_level(struct SnakeState *state)
{
    pthread_mutex_lock(&state->snakeGameState.gameMutex);
    while (state->snakeRender.isRender)
    {
        pthread_cond_wait(&state->snakeRender.renderCond, &state->snakeRender.renderMutex);
    }

    // Clear screen and show level completion message // 清屏并显示关卡完成信息
    ncplane_erase(state->snakePlane);
    char next_level_str[256];
    snprintf(next_level_str, 256, "Level %d completed!", state->snakeGameState.level);
    ncplane_putstr_aligned(state->snakePlane, state->snakeGameState.Map.height / 2, NCALIGN_CENTER, next_level_str);
    state->snakeGameState.stateChanged = false;
    state->snakeRender.isRender = true;
    pthread_mutex_unlock(&state->snakeGameState.gameMutex);
    sleep(2);
}

// The logic in the game, the main game loop, the system considers time greater than or equal to 3000 as infinite time. // 游戏中的逻辑,游戏主循环,系统认为时间大于等于3000为无限时间。
static void *updateSnakeState(void *arg)
{
    struct SnakeState *state = (struct SnakeState *)arg;

    // The time stop multiplier, that is, the sleep time multiplier. // 时停倍率，也就是睡眠时间倍率
    state->snakeGameState.timeCount = 1;
    double *time_count = &(state->snakeGameState.timeCount);
    int *Godlike = &(state->snakeGameState.Snake.Buff.GodlikeScore);
    // The base sleep time for movement; this value can be affected by items, larger values slow down movement, while smaller values speed it up. // 为基础移动时睡眠时间，这个值可以受到道具的影响，值越大移动速度越慢，反之越快。
    unsigned USLEEPTIME = 120000;
    double deltaTime = (double)USLEEPTIME / 1000000;
    while (!atomic_load(&(state->snakeGameState.gameOver)) && !state->snakeGameState.gameWon)
    {
        SnakeGamePauseState(state);
        if (atomic_load(&(state->snakeGameState.gameOver)))
        {
            pthread_mutex_unlock(&state->snakeGameState.gameMutex);
            break;
        }
        pthread_mutex_lock(&state->snakeGameState.gameMutex);
        // Time stop effect handling. // 时停效果处理
        if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_mushroom)))
        {
            // In a psychedelic state, time stop cannot be used. // 处于迷幻状态,时停无法使用
            *time_count = 1.5;
            state->snakeGameState.Snake.Debuff.mushroomTimeLeft -= deltaTime;
            atomic_store(&(state->snakeGameState.Snake.Buff.isTimeMoment), false);
            if (state->snakeGameState.Snake.Debuff.mushroomTimeLeft <= 0)
            {
                atomic_store(&(state->snakeGameState.Snake.Debuff.is_mushroom), false); // The mushroom effect state ends, time stop cannot be used. // 处于迷幻状态，时停无法使用
                state->snakeGameState.Snake.Debuff.mushroomTimeLeft = 0;                // Reset mushroom effect time. // 迷幻时间重置
                if (state->isMusicOn)
                {
                    snakeSwitchNormalMusic(state, MushroomMusic); // Switch back to normal music. // 切换回正常音乐
                }
            }
        }
        else if (atomic_load(&(state->snakeGameState.Snake.Buff.isTimeMoment)) == true)
        {
            state->snakeGameState.Snake.Buff.timeStopTimeLeft -= deltaTime; // Decrease time stop effect duration. // 时停效果时间减少
            *time_count = *time_count < 4 ? *time_count + 2 : 4;            // Maximum time stop multiplier is 4. // 最大时停倍率为4
            if (state->snakeGameState.Snake.Buff.timeStopTimeLeft <= 0)
            {
                state->snakeGameState.Snake.Buff.timeStopTimeLeft = 0;                 // Reset time stop duration. // 重置时停效果时间
                atomic_store(&(state->snakeGameState.Snake.Buff.isTimeMoment), false); // End time stop state. // 结束时停状态
                *time_count = *time_count > 2 ? *time_count - 2 : 1;                   // Restore normal multiplier. // 恢复正常倍数
                if (state->isMusicOn)
                    audio_set_bgm_volume(100); // Restore background music volume. // 恢复背景音乐音量
            }
        }
        else
        {
            *time_count = *time_count > 2 ? *time_count - 2 : 1; // Decrease time stop multiplier in normal state. // 正常状态下减少时停倍数
        }

        // Countdown system update time; the system considers time greater than or equal to 3000 as infinite time. // 倒计时系统更新时间,系统认为时间大于等于3000为无限时间。
        if (state->snakeGameState.timeLeft >= 3000)
        {
            state->snakeGameState.timeLeft = 3000;
        }
        else
        {
            state->snakeGameState.timeLeft -= deltaTime / *time_count;
        }
        if (state->snakeGameState.timeLeft <= 0)
        {
            atomic_store(&(state->snakeGameState.gameOver), true);
            pthread_mutex_unlock(&state->snakeGameState.gameMutex);
            continue;
        }

        // Update acceleration score time. // 更新加速积分时间
        if (atomic_load(&(state->snakeGameState.Snake.Buff.isMoreScore)))
        {
            state->snakeGameState.Snake.Buff.moreScoreTimeLeft -= deltaTime / *time_count;
            if (state->snakeGameState.Snake.Buff.moreScoreTimeLeft <= 0)
            {
                state->snakeGameState.Snake.Buff.moreScoreTimeLeft = 0;
                atomic_store(&(state->snakeGameState.Snake.Buff.isMoreScore), false);
            }
        }

        // In the hot tomato state // 处于火热番茄状态
        if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)))
        {
            state->snakeGameState.Snake.Debuff.hotTimeLeft -= deltaTime / *time_count;
            if (state->snakeGameState.Snake.Debuff.hotTimeLeft <= 0)
            {
                atomic_store(&(state->snakeGameState.Snake.Debuff.is_hot), false);
                USLEEPTIME = 120000;
                state->snakeGameState.Snake.Debuff.hotTimeLeft = 0;
                if (state->isMusicOn)
                {
                    snakeSwitchNormalMusic(state, Hotmusic);
                }
            }
        }
        // In stomach ache state. // 处于胃痛状态
        if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_insect)))
        {
            state->snakeGameState.Snake.Debuff.insectTimeLeft -= deltaTime / *time_count;
            if (state->snakeGameState.Snake.Debuff.insectTimeLeft <= 0)
            {
                atomic_store(&(state->snakeGameState.Snake.Debuff.is_insect), false);
                state->snakeGameState.Snake.Debuff.insectTimeLeft = 0;
                if (state->isMusicOn)
                {
                    snakeSwitchNormalMusic(state, InsectMusic);
                }
            }
        }
        // Update God of Destruction time. // 更新破坏神时间
        if (state->snakeGameState.Snake.Buff.God_of_Destruction > 0)
        {
            state->snakeGameState.Snake.Buff.God_of_Destruction -= deltaTime / *time_count;
            if (state->snakeGameState.Snake.Buff.God_of_Destruction <= 0)
            {
                state->snakeGameState.Snake.Buff.God_of_Destruction = 0;
            }
            if (atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)) == false && atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)) == false)
            {
                atomic_store(&(state->snakeGameState.Snake.Buff.isGodOfDestruction), true);
                if (state->isMusicOn)
                {
                    snakeSwitchMusicToID(state, godLikeMusic);
                }
            }
        }
        else
        {
            state->snakeGameState.Snake.Buff.God_of_Destruction = 0;
            atomic_store(&(state->snakeGameState.Snake.Buff.isGodOfDestruction), false);
            if (state->isMusicOn && nowMusicType != NormalMusic && atomic_load(isDebuffMusic) == false)
            {
                audio_play_sfx("./getBuff.wav");
                snakeSwitchMusicToID(state, NormalMusic);
            }
        }

        // Get the next valid direction from the input buffer. // 从输入缓冲区获取下一个有效方向
        char nextDirection = snakePopInput(state);
        while (nextDirection != '\0')
        {
            if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_mushroom)))
            { // When the psychedelic mushroom effect exists, the direction will be reversed. // 迷幻蘑菇效果存在时候将是反方向
                switch (nextDirection)
                {
                case 'a':
                    nextDirection = 'd';
                    break;
                case 'd':
                    nextDirection = 'a';
                    break;
                case 's':
                    nextDirection = 'w';
                    break;
                case 'w':
                    nextDirection = 's';
                    break;
                default:
                    break;
                }
            }
            // Check if the new direction is the same as or opposite to the current direction. // 检查新方向是否与当前方向相同或者是相反方向
            if (nextDirection != state->snakeGameState.Snake.direction)
            {
                // Avoid 180-degree reverse movement. // 避免180度反向移动
                bool isValidDirectionChange =
                    !((nextDirection == 'a' && state->snakeGameState.Snake.direction == 'd') ||
                      (nextDirection == 'd' && state->snakeGameState.Snake.direction == 'a') ||
                      (nextDirection == 'w' && state->snakeGameState.Snake.direction == 's') ||
                      (nextDirection == 's' && state->snakeGameState.Snake.direction == 'w'));

                if (isValidDirectionChange)
                {
                    state->snakeGameState.Snake.direction = nextDirection;
                    break;
                }
            }

            // If the current direction is invalid, continue to get the next direction. // 如果当前方向无效，继续获取下一个方向
            nextDirection = snakePopInput(state);
        }

        switch (state->snakeGameState.Snake.direction)
        {
        case 'a':
            state->snakeGameState.Snake.snakeX = (state->snakeGameState.Snake.snakeX == 0) ? state->snakeGameState.Map.width - 1 : state->snakeGameState.Snake.snakeX - 1;
            break;
        case 'd':
            state->snakeGameState.Snake.snakeX = (state->snakeGameState.Snake.snakeX == state->snakeGameState.Map.width - 1) ? 0 : state->snakeGameState.Snake.snakeX + 1;
            break;
        case 'w':
            state->snakeGameState.Snake.snakeY = (state->snakeGameState.Snake.snakeY == 0) ? state->snakeGameState.Map.height - 1 : state->snakeGameState.Snake.snakeY - 1;
            break;
        case 's':
            state->snakeGameState.Snake.snakeY = (state->snakeGameState.Snake.snakeY == state->snakeGameState.Map.height - 1) ? 0 : state->snakeGameState.Snake.snakeY + 1;
            break;
        }

        // Calculate the new position. // 计算新位置
        int newPos = state->snakeGameState.Snake.snakeY * state->snakeGameState.Map.width + state->snakeGameState.Snake.snakeX;

        // Collision detection. // 碰撞检测
        if (state->snakeGameState.Map.map[newPos] > 1)
        {
            if (state->isMusicOn)
                audio_play_sfx("./getDebuff.wav");
            atomic_store(&(state->snakeGameState.gameOver), true);
            pthread_mutex_unlock(&state->snakeGameState.gameMutex);
            continue;
        }
        else if (state->snakeGameState.Map.map[newPos] == -101)
        {
            // God of Destruction time judgment, must not be in chili state. // 破坏神时间判定,必须不处于火热番茄状态
            if (atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)) && atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)) != true)
            {
                if (state->isMusicOn)
                    audio_play_sfx("./killWall.wav");
                state->snakeGameState.Map.map[newPos] = 0;
                *Godlike += 1;
                if (*Godlike >= 5)
                {
                    if (atomic_load(&(state->snakeGameState.Snake.Buff.isMoreScore)))
                    {
                        state->snakeGameState.Snake.score += 2;
                        state->snakeGameState.Snake.length += 2;
                    }
                    else
                    {
                        state->snakeGameState.Snake.score += 1;
                        state->snakeGameState.Snake.length += 1;
                    }

                    *Godlike = 0;
                }
            }
            else
            {
                atomic_store(&(state->snakeGameState.gameOver), true);
                if (state->isMusicOn)
                    audio_play_sfx("./getDebuff.wav");
                pthread_mutex_unlock(&state->snakeGameState.gameMutex);
                continue;
            }
        }

        // Handle item collection. // 处理物品收集
        if (state->snakeGameState.Map.map[newPos] == -1)
        { // Normal food. // 普通食物
        eatfood:
            if (!atomic_load(&state->snakeGameState.Snake.Debuff.is_insect))
            {
                if (state->isMusicOn)
                    audio_play_sfx("./eatFood.wav");
                if (atomic_load(&(state->snakeGameState.Snake.Buff.isMoreScore)))
                {
                    state->snakeGameState.Snake.length += 2;
                    state->snakeGameState.Snake.score += 2;
                }
                else
                {
                    state->snakeGameState.Snake.length++;
                    state->snakeGameState.Snake.score++;
                }
            }
            else
            {
                if (state->isMusicOn)
                    audio_play_sfx("./getDebuff.wav");
                // Deducting score and length. // 扣分和长度
                int temp = (state->snakeGameState.Snake.length / 3);
                if (temp < 2)
                    temp = 2;
                state->snakeGameState.Snake.length -= temp;
                state->snakeGameState.Snake.score -= temp;
                // Check failure, score below 2. // 检查失败,分数为2以下
                if (state->snakeGameState.Snake.score < 2)
                {
                    state->snakeGameState.Snake.score = -9999;
                    atomic_store(&(state->snakeGameState.gameOver), true);
                }
                int place = (state->snakeGameState.Map.height) * (state->snakeGameState.Map.width);
                for (int i = 0; i < place; i++)
                {
                    if (state->snakeGameState.Map.map[i] > 0)
                    {
                        state->snakeGameState.Map.map[i] -= temp;
                        if (state->snakeGameState.Map.map[i] < 0)
                            state->snakeGameState.Map.map[i] = 0;
                    }
                }
            }
            if (!snakeGenerateItemFood(state, state->snakeGameState.Level_Item.FoodCount, 0b01))
                snakeGenerateItemFood(state, state->snakeGameState.Level_Item.FoodCount, 0b10);
            if (state->snakeGameState.Snake.score % 10 == 0)
            {
                snakeGenerateItemBuff(state, state->snakeGameState.Level_Item.BuffCount, 0b10);
            }
            else if (state->snakeGameState.Snake.score % 19 == 0)
            {
                snakeGenerateItemDebuff(state, state->snakeGameState.Level_Item.DebuffCount, 0b10);
            }
        }
        else if (state->snakeGameState.Map.map[newPos] == -201)
        { // Time stop item. // 时停道具
            if (state->isMusicOn)
                audio_play_sfx("./getBuff.wav");
            state->snakeGameState.Snake.Buff.timeStopTimeLeft += 5;
            if (state->snakeGameState.Snake.Buff.timeStopTimeLeft >= 15)
                state->snakeGameState.Snake.Buff.timeStopTimeLeft = 15;
            state->snakeGameState.timeLeft += 5;
            if (atomic_load(&(state->snakeGameState.Snake.Buff.isMoreScore)))
                goto eatfood;
        }
        else if (state->snakeGameState.Map.map[newPos] == -202)
        { // Acceleration score item. // 加速积分道具
            if (state->isMusicOn)
                audio_play_sfx("./getBuff.wav");
            state->snakeGameState.Snake.Buff.moreScoreTimeLeft += 10;
            atomic_store(&(state->snakeGameState.Snake.Buff.isMoreScore), true);
            if (atomic_load(&(state->snakeGameState.Snake.Buff.isMoreScore)))
                goto eatfood;
        }
        else if (state->snakeGameState.Map.map[newPos] == -203)
        { // God of Destruction item. // 破坏神道具
            if (atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)) == false)
            {
                state->snakeGameState.Snake.Buff.God_of_Destruction += 10;
                atomic_store(&(state->snakeGameState.Snake.Buff.isGodOfDestruction), true);
                state->snakeGameState.Snake.Debuff.insectTimeLeft = 0;
                state->snakeGameState.Snake.Debuff.mushroomTimeLeft = 0;
                if (state->isMusicOn)
                {
                    audio_play_sfx("./getBuff.wav");
                    snakeSwitchMusicToID(state, godLikeMusic);
                }
                if (atomic_load(&(state->snakeGameState.Snake.Buff.isMoreScore)))
                    goto eatfood;
            }
        }
        else if (state->snakeGameState.Map.map[newPos] == -301)
        {
            if (state->isMusicOn)
            {
                audio_play_sfx("./getBuff.wav");
                snakeSwitchMusicToID(state, Hotmusic);
            }
            // Chili, chili prohibits God of Destruction, moving faster. Note that during the chili period, if in God of Destruction time, the state will also consume. // 火热番茄，火热番茄禁止破坏神,移动加快。注意火热番茄期间若处于破坏神时间，则状态也会消耗。
            atomic_store(&(state->snakeGameState.Snake.Debuff.is_hot), true);
            state->snakeGameState.Snake.Debuff.hotTimeLeft = 5;
            USLEEPTIME = 80000;
        }
        else if (state->snakeGameState.Map.map[newPos] == -302)
        {
            // Psychedelic mushrooms can be killed by God of Destruction, psychedelic mushrooms prohibit time stop, feeling dizzy. // 迷幻蘑菇可以被破坏神杀死，迷幻蘑菇禁止时停，晕乎乎
            if (atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)) && atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)) == false)
            {
                *Godlike += 1;
                goto eatfood;
            }
            // Psychedelic mushroom. // 迷幻蘑菇
            if (state->isMusicOn)
            {
                snakeSwitchMusicToID(state, MushroomMusic);
                audio_play_sfx("./getDebuff.wav");
                audio_set_bgm_volume(100);
            }
            // Psychedelic mushroom, prohibits time stop, feeling dizzy, the snake moves in the opposite direction. // 迷幻蘑菇，迷幻蘑菇禁止时停，晕乎乎，蛇乱走方向相反
            atomic_store(&(state->snakeGameState.Snake.Debuff.is_mushroom), true);
            state->snakeGameState.Snake.Debuff.mushroomTimeLeft = 10;
            atomic_store(&(state->snakeGameState.Snake.Buff.isTimeMoment), false);
        }
        else if (state->snakeGameState.Map.map[newPos] == -303)
        {
            // Worms, eating worms does not give points for 5 seconds; if food is consumed during this time, points will be deducted. Worms can be killed by God of Destruction. // 虫子，吃下虫子蛇吃东西不加分5秒,若此时吃下东西就扣分.虫子可以被破坏神杀死
            if (atomic_load(&(state->snakeGameState.Snake.Buff.isGodOfDestruction)) && atomic_load(&(state->snakeGameState.Snake.Debuff.is_hot)) == false)
            {
                *Godlike += 1;
                goto eatfood;
            }
            if (state->isMusicOn)
            {
                audio_play_sfx("./getDebuff.wav");
                snakeSwitchMusicToID(state, InsectMusic);
            }
            atomic_store(&(state->snakeGameState.Snake.Debuff.is_insect), true);
            state->snakeGameState.Snake.Debuff.insectTimeLeft = 10;
        }
        else
        {
            // Update the snake body. // 更新蛇身
            for (int i = 0; i < state->snakeGameState.Map.width * state->snakeGameState.Map.height; i++)
            {
                if (state->snakeGameState.Map.map[i] > 0)
                    state->snakeGameState.Map.map[i]--;
            }
        }

        state->snakeGameState.Map.map[newPos] = state->snakeGameState.Snake.length;

        // Check victory conditions. // 检查胜利条件
        if (state->snakeGameState.Snake.score >= state->snakeGameState.targetScore)
        {
            if (state->isMusicOn)
                audio_pause_bgm();
            state->snakeGameState.gameWon = true;
        }

        state->snakeGameState.stateChanged = true;

        pthread_cond_signal(&state->snakeGameState.updateCond);
        pthread_mutex_unlock(&state->snakeGameState.gameMutex);
        if (state->snakeGameState.stateChanged)
        {
            render_game(state); // Render game state. // 渲染游戏状态
        }
        usleep(*time_count * USLEEPTIME); // Sleep based on time stop multiplier and base sleep time. // 根据时停倍数和基础睡眠时间进行睡眠
    }
    return NULL; // Return null pointer. // 返回空指针
}

static void *StartSnakeGame(void *the_state)
{
    struct SnakeState *state = (struct SnakeState *)the_state; // Cast the argument to SnakeState structure. // 将参数转换为SnakeState结构体
    bool continuePlaying = true;                               // Variable to control the continuation of the game. // 控制游戏继续进行的变量
    int width = state->snakeGameState.Map.width;               // Get the width of the game map. // 获取游戏地图的宽度
    int height = state->snakeGameState.Map.height;             // Get the height of the game map. // 获取游戏地图的高度
    while (continuePlaying)                                    // Loop to continue playing the game.
    {
        initializeGame(state);                                    // Initialize the game state. // 初始化游戏状态
        pthread_mutex_init(&state->snakeInputBuffer.mutex, NULL); // Initialize the input buffer mutex. // 初始化输入缓冲区互斥量

        pthread_create(&state->gameThread, NULL, updateSnakeState, state); // Create a thread to update the snake state. // 创建一个线程来更新蛇的状态

        while (!atomic_load(&(state->snakeGameState.gameOver)) && !state->snakeGameState.gameWon) // Loop until the game is over or won.
        {
            usleep(16666); // Sleep for approximately 16.67 milliseconds. // 睡眠约16.67毫秒
        }
        pthread_join(state->gameThread, NULL);                // Wait for the game thread to finish. // 等待游戏线程完成
        if (state->snakeGameState.gameWon && continuePlaying) // Check if the game is won and if playing should continue.
        {
            pthread_mutex_destroy(&state->snakeInputBuffer.mutex);                  // Destroy the input buffer mutex. // 销毁输入缓冲区互斥量
            pthread_mutex_destroy(&state->snakeGameState.gameMutex);                // Destroy the game mutex. // 销毁游戏互斥量
            pthread_mutex_destroy(&state->snakeRender.renderMutex);                 // Destroy the render mutex. // 销毁渲染互斥量
            pthread_cond_destroy(&state->snakeGameState.updateCond);                // Destroy the update condition variable. // 销毁更新条件变量
            pthread_cond_destroy(&state->snakeRender.renderCond);                   // Destroy the render condition variable. // 销毁渲染条件变量
            memset(state->snakeGameState.Map.map, 0, width * height * sizeof(int)); // Clear the game map. // 清空游戏地图
            render_game_next_level(state);                                          // Render the next level. // 渲染下一关卡
            memset(state->snakeInputBuffer.buffer, 0, SNAKE_SIZEOF_BUFFER_INPUT);   // Clear the input buffer. // 清空输入缓冲区
            state->snakeGameState.level += 1;                                       // Increment the level. // 增加关卡
        }
        else
        {
            if (atomic_load(&(state->snakeGameState.gameOver)))
            {
                if (state->isMusicOn)
                {
                    audio_pause_bgm();
                }
                if (!atomic_load(&(state->snakeGameState.gameOverByUser)))
                {
                    sleep(1);
                }
                render_game_over(state);
                if (!atomic_load(&(state->snakeGameState.gameOverByUser)))
                {
                    sleep(2);
                }
                if (state->isMusicOn)
                {
                    audio_cleanup();
                    state->isMusicOn = false;
                }
                state->isrunning = false;
                return NULL;
            }
            break; // Exit the loop if not continuing. // 如果不继续，则退出循环
        }
    }

    return NULL; // Return null to indicate completion. // 返回空指针表示完成
}

void snake_get_terminal_capabilities(struct notcurses *nc, TerminalCapabilities *caps)
{
    caps->color_count = notcurses_palette_size(nc);           // Get the number of colors available. // 获取可用颜色的数量
    caps->can_fade = notcurses_canfade(nc);                   // Check if fading is supported. // 检查是否支持淡入淡出
    caps->can_change_palette = notcurses_canchangecolor(nc);  // Check if palette can be changed. // 检查是否可以更改调色板
    caps->supports_rgb = notcurses_canpixel(nc);              // Check if RGB is supported. // 检查是否支持RGB
    caps->can_pixel = notcurses_canpixel(nc);                 // Check if pixel manipulation is possible. // 检查是否可以进行像素操作
    caps->can_openimages = notcurses_canopen_images(nc);      // Check if images can be opened. // 检查是否可以打开图像
    caps->can_openimages = notcurses_canopen_videos(nc);      // Check if videos can be opened. // 检查是否可以打开视频
    notcurses_stddim_yx(nc, &(caps->height), &(caps->width)); // Get terminal dimensions. // 获取终端尺寸
    {
        unsigned int supported_styles = notcurses_supported_styles(nc); // Get supported text styles.
        // Use bitwise operations to detect styles.// 使用位运算检测样式
        caps->tstyle_support.bold = supported_styles & NCSTYLE_BOLD;           // Check if bold is supported. // 检查是否支持粗体
        caps->tstyle_support.underline = supported_styles & NCSTYLE_UNDERLINE; // Check if underline is supported. // 检查是否支持下划线
        caps->tstyle_support.italic = supported_styles & NCSTYLE_ITALIC;       // Check if italic is supported. // 检查是否支持斜体
        caps->tstyle_support.struck = supported_styles & NCSTYLE_STRUCK;       // Check if strikethrough is supported. // 检查是否支持删除线
        caps->tstyle_support.undercurl = supported_styles & NCSTYLE_UNDERCURL; // Check if undercurl is supported. // 检查是否支持下划线
    }
}

// 初始化原子，只做一次
static void init_global_music_atomic()
{

    snakeMusicOn = malloc(sizeof(atomic_bool));
    atomic_init(snakeMusicOn, false);

    isDebuffMusic = malloc(sizeof(atomic_bool));
    atomic_init(isDebuffMusic, false);

    audio_cleaned = malloc(sizeof(atomic_bool));
    atomic_init(audio_cleaned, true);
}

struct SnakeState *SnakeGameInit(struct notcurses *base_nc, struct SnakeOpt *opt)
{
    struct SnakeState *state = calloc(1, sizeof(struct SnakeState)); // Allocate memory for the SnakeState structure. // 为SnakeState结构体分配内存,初始化原内存为0
    if (!state)                                                      // Check if memory allocation was successful.
        return NULL;                                                 // Return NULL if allocation failed. // 如果分配失败，返回NULL
    pthread_once(&music_once, init_global_music_atomic);
    while (music_once == PTHREAD_ONCE_INIT)
        ;
    snake_get_terminal_capabilities(base_nc, &(state->caps)); // Get terminal capabilities and store in state. // 获取终端能力并存储在状态中
    state->snakePlane = opt->snakeNcplane;                    // Set the snake plane from options. // 从选项中设置蛇的平面

    unsigned int width, height;                         // Declare width and height variables.
    ncplane_dim_yx(opt->snakeNcplane, &height, &width); // Get dimensions of the snake plane. // 获取蛇平面的维度

    //! Important: The width of the snake map must be divided by 2 because the map is printed with double characters. // 重要：计算贪吃蛇地图宽度必须除2，因为地图为双字符打印
    width /= 2;
    //! Important: Reserve 3 rows for the status bar in the height calculation, and leave one row for the bottom bar. // 重要：计算贪吃蛇地图高度预留3行，打印状态栏，并预留一个底栏
    height -= 4;

    // Handle custom width and height. // 处理自定义宽高
    width = (((unsigned)opt->width != 0 && (unsigned)opt->width < width) ? (unsigned)opt->width : (unsigned)width);       // Set custom width if valid. // 如果有效则设置自定义宽度
    height = (((unsigned)opt->height != 0 && (unsigned)opt->height < height) ? (unsigned)opt->height : (unsigned)height); // Set custom height if valid. // 如果有效则设置自定义高度

    state->snakeGameState.Map.width = width;                                    // Set the map width in the state. // 在状态中设置地图宽度
    state->snakeGameState.Map.height = height;                                  // Set the map height in the state. // 在状态中设置地图高度
    state->snakeGameState.Map.map = (int *)calloc(width * height, sizeof(int)); // Allocate memory for the game map.

    if (!(state->snakeGameState.Map.map)) // Check if the map allocation was successful.
    {
        atomic_store(&(state->snakeGameState.gameOver), true);
        return NULL; // Return NULL if allocation failed. // 返回NULL表示分配失败
    }

    state->snakeRender.shouldFade = (state->caps.can_fade) && (opt->fade); // Check if fading is supported and enabled.

    if (state->snakeRender.shouldFade) // If fading is enabled, set up colors.
    {
        state->snakeRender.width = width * 2; // Set the render width for fading. // 设置渲染的宽度
        state->snakeRender.height = height;   // Set the render height. // 设置渲染的高度
        // NORMAL
        ncchannels_set_bg_rgb(&state->snakeRender.normal.left_up, 0x24243e);       // Set background color for normal state (top left). // 设置正常状态的背景色（左上角）
        ncchannels_set_bg_rgb(&state->snakeRender.normal.left_down, 0x0f0c29);     // Set background color for normal state (bottom left). // 设置正常状态的背景色（左下角）
        ncchannels_set_bg_rgb(&state->snakeRender.normal.right_up, 0x24243e);      // Set background color for normal state (top right). // 设置正常状态的背景色（右上角）
        ncchannels_set_bg_rgb(&state->snakeRender.normal.right_down, 0x7303c0);    // Set background color for normal state (bottom right). // 设置正常状态的背景色（右下角）
        ncchannels_set_bg_rgb(&state->snakeRender.normal.snake_fade[0], 0x302b63); // Set background color for snake fade (0). // 设置食物渐变的背景色（0）
        ncchannels_set_bg_rgb(&state->snakeRender.normal.snake_fade[1], 0x25214d); // Set background color for snake fade (1). // 设置食物渐变的背景色（1）
        ncchannels_set_bg_rgb(&state->snakeRender.normal.snake_fade[2], 0x23112b); // Set background color for snake fade (2). // 设置食物渐变的背景色（2）
        // TIMESTOP
        ncchannels_set_bg_rgb(&state->snakeRender.timemoment.left_up, 0x000046);       // Set background color for time stop state (top left). // 设置时停状态的背景色（左上角）
        ncchannels_set_bg_rgb(&state->snakeRender.timemoment.left_down, 0x0f0c29);     // Set background color for time stop state (bottom left). // 设置时停状态的背景色（左下角）
        ncchannels_set_bg_rgb(&state->snakeRender.timemoment.right_up, 0x1cb5e0);      // Set background color for time stop state (top right). // 设置时停状态的背景色（右上角）
        ncchannels_set_bg_rgb(&state->snakeRender.timemoment.right_down, 0x302b63);    // Set background color for time stop state (bottom right). // 设置时停状态的背景色（右下角）
        ncchannels_set_bg_rgb(&state->snakeRender.timemoment.snake_fade[0], 0x24243e); // Set background color for time stop snake fade (0). // 设置时停食物渐变的背景色（0）
        ncchannels_set_bg_rgb(&state->snakeRender.timemoment.snake_fade[1], 0x1cb5e0); // Set background color for time stop snake fade (1). // 设置时停食物渐变的背景色（1）
        ncchannels_set_bg_rgb(&state->snakeRender.timemoment.snake_fade[2], 0x000046); // Set background color for time stop snake fade (2). // 设置时停食物渐变的背景色（2）
        // HOT
        ncchannels_set_bg_rgb(&state->snakeRender.hot.left_up, 0xeb5757);       // Set background color for hot state (top left). // 设置火热番茄状态的背景色（左上角）
        ncchannels_set_bg_rgb(&state->snakeRender.hot.left_down, 0x200122);     // Set background color for hot state (bottom left). // 设置火热番茄状态的背景色（左下角）
        ncchannels_set_bg_rgb(&state->snakeRender.hot.right_up, 0x6f0000);      // Set background color for hot state (top right). // 设置火热番茄状态的背景色（右上角）
        ncchannels_set_bg_rgb(&state->snakeRender.hot.right_down, 0x000000);    // Set background color for hot state (bottom right). // 设置火热番茄状态的背景色（右下角）
        ncchannels_set_bg_rgb(&state->snakeRender.hot.snake_fade[0], 0xeb5757); // Set background color for hot snake fade (0). // 设置火热番茄食物渐变的背景色（0）
        ncchannels_set_bg_rgb(&state->snakeRender.hot.snake_fade[1], 0x6f0000); // Set background color for hot snake fade (1). // 设置火热番茄食物渐变的背景色（1）
        ncchannels_set_bg_rgb(&state->snakeRender.hot.snake_fade[2], 0x200122); // Set background color for hot snake fade (2). // 设置火热番茄食物渐变的背景色（2）
        // INFECTED
        ncchannels_set_bg_rgb(&state->snakeRender.infected.left_up, 0x093028);    // Set background color for infected state (top left). // 设置感染状态的背景色（左上角）
        ncchannels_set_bg_rgb(&state->snakeRender.infected.left_down, 0x237a57);  // Set background color for infected state (bottom left). // 设置感染状态的背景色（左下角）
        ncchannels_set_bg_rgb(&state->snakeRender.infected.right_up, 0x6a9113);   // Set background color for infected state (top right). // 设置感染状态的背景色（右上角）
        ncchannels_set_bg_rgb(&state->snakeRender.infected.right_down, 0x141517); // Set background color for infected state (bottom right). // 设置感染状态的背景色（右下角）

        ncchannels_set_bg_rgb(&state->snakeRender.infected.snake_fade[0], 0x093028); // Set background color for infected snake fade (0). // 设置感染食物渐变的背景色（0）
        ncchannels_set_bg_rgb(&state->snakeRender.infected.snake_fade[1], 0x237a57); // Set background color for infected snake fade (1). // 设置感染食物渐变的背景色（1）
        ncchannels_set_bg_rgb(&state->snakeRender.infected.snake_fade[2], 0x141517); // Set background color for infected snake fade (2). // 设置感染食物渐变的背景色（2）
    }

    if (opt->isMusicOn && atomic_load(snakeMusicOn) == false && audio_init(state->musicPath) == true) // Check if music should be turned on.
    {
        state->isMusicOn = true;          // Set music state to on. // 设置音乐状态为开
        atomic_store(snakeMusicOn, true); // Set global music state to on. // 设置全局音乐状态为开
    }
    else
    {
        state->isMusicOn = false; // Set music state to off. // 设置音乐状态为关
    }

    // Initialize mutexes and condition variables. // 初始化互斥锁和条件变量
    pthread_mutex_init(&state->snakeGameState.gameMutex, NULL); // Initialize game mutex. // 初始化游戏互斥量
    pthread_mutex_init(&state->snakeInputBuffer.mutex, NULL);   // Initialize input buffer mutex. // 初始化输入缓冲区互斥量
    pthread_mutex_init(&state->snakeRender.renderMutex, NULL);  // Initialize render mutex. // 初始化渲染互斥量
    pthread_cond_init(&state->snakeGameState.updateCond, NULL); // Initialize update condition variable. // 初始化更新条件变量
    pthread_cond_init(&state->snakeRender.renderCond, NULL);    // Initialize render condition variable. // 初始化渲染条件变量

    atomic_init(&state->snakePauseState.isPaused, false);         // Set pause state to not paused. // 设置暂停状态为未暂停
    state->snakePauseState.lastPauseTime.tv_nsec = 0;             // Initialize last pause time nanoseconds to 0. // 初始化最后暂停时间的纳秒为0
    state->snakePauseState.lastPauseTime.tv_sec = 0;              // Initialize last pause time seconds to 0. // 初始化最后暂停时间的秒为0
    pthread_mutex_init(&state->snakePauseState.pauseMutex, NULL); // Initialize pause mutex. // 初始化暂停互斥量
    pthread_cond_init(&state->snakePauseState.pauseCond, NULL);   // Initialize pause condition variable. // 初始化暂停条件变量
                                                                  // 检查初始化是否成功
    if (!SnakeGameStateJoin(state))
    {
        // 如果加入失败，清理状态并返回 NULL
        SnakeGameClean(state);
        return NULL;
    }
    // Set running flags. // 设置运行标志
    state->isrunning = true;                  // Set the running state to true. // 设置运行状态为true
    state->snakeRender.isRunning = true;      // Set the render running state to true. // 设置渲染运行状态为true
    state->snakeGameState.level = opt->level; // Set the game level from options. // 从选项中设置游戏关卡
                                              // 检查初始化是否成功
    // Initialized atom //初始化原子
    atomic_init(&(state->snakeGameState.Snake.Buff.isTimeMoment), false);       // Set the time moment flag to false. // 设置时停标志为false
    atomic_init(&(state->snakeGameState.Snake.Buff.isMoreScore), false);        // Set the more score flag to false. // 设置更多分数标志为false
    atomic_init(&(state->snakeGameState.Snake.Buff.isGodOfDestruction), false); // Set the god of destruction flag to false. // 设置破坏神标志为false
    atomic_init(&(state->snakeGameState.Snake.Debuff.is_mushroom), false);      // Set the mushroom flag to false. // 设置蘑菇标志为false
    atomic_init(&(state->snakeGameState.Snake.Debuff.is_insect), false);        // Set the insect flag to false. // 设置感染标志为false
    atomic_init(&(state->snakeGameState.Snake.Debuff.is_hot), false);           // Set the hot flag to false. // 设置火热番茄标志为false
    atomic_init(&(state->snakeGameState.gameOver), false);                      // Set the game over flag to false. // 设置游戏结束标志为false
    atomic_init(&(state->snakeGameState.gameOverByUser), false);                // Set the game over flag to false. // 设置游戏结束标志为false
    if (opt->musicPath[0] == '\0')
    {
        state->musicPath = NULL;
    }
    else
    {
        state->musicPath = strdup(opt->musicPath);
    }
    if (state != NULL)
    {
        // 尝试将状态加入全局链表
        if (!SnakeGameStateJoin(state))
        {
            // 如果加入失败，清理状态并返回 NULL
            SnakeGameClean(state);
            return NULL;
        }
    }
    // Clear the map space counter. // 清除地图空间计数器
    snakeClearAvailableSpaces(state); // Clear available spaces in the game. // 清理游戏中的可用空间

    if (state->isMusicOn) // If music is on, play the background music.
    {
        nowMusicType = NormalMusic;         // Set the current music type to normal. // 设置当前音乐类型为正常
        atomic_store(isDebuffMusic, false); // Set debuff music state to false. // 设置减益音乐状态为false
        audio_set_bgm_volume(100);          // Set background music volume to 100. // 设置背景音乐音量为100
    }

    // Start the main game loop. // 启动游戏主循环
    pthread_create(&state->gameThread, NULL, StartSnakeGame, (void *)state); // Create a thread for the game loop. // 创建一个线程用于游戏循环

    return state; // Return the initialized state. // 返回初始化后的状态
}

// Clean up the Snake structure. // 清理贪吃蛇结构体
void SnakeGameClean(struct SnakeState *state)
{
    // Check if state pointer is NULL
    if (state == NULL)
    {
        return;
    }
    static atomic_flag cleaningFlag = ATOMIC_FLAG_INIT;
    if (atomic_flag_test_and_set(&cleaningFlag))
    {
        return;
    }
    state->isrunning = false;
    atomic_store(&(state->snakeGameState.gameOver), true);
    state->snakeGameState.gameWon = false;
    atomic_store(&state->snakePauseState.isPaused, false);

    if (state->isMusicOn && atomic_load(snakeMusicOn) == true)
    {
        audio_cleanup();
    }
    snakeClearAvailableSpaces(state);
    snakeSnakeLevelItem_clear(state);
    snakeClearInputBuffer(state);
    // Destroy mutex and condition variables//销毁互斥锁和条件变量
    pthread_mutex_destroy(&state->snakeInputBuffer.mutex);
    pthread_mutex_destroy(&state->snakeGameState.gameMutex);
    pthread_mutex_destroy(&state->snakeRender.renderMutex);
    pthread_mutex_destroy(&state->snakePauseState.pauseMutex);
    pthread_cond_destroy(&state->snakeGameState.updateCond);
    pthread_cond_destroy(&state->snakeRender.renderCond);
    pthread_cond_destroy(&state->snakePauseState.pauseCond);
    free(state->musicPath);
    free(state);
    state = NULL;
}
// External pause function. // 外部暂停函数
bool SnakeGameTryPause(struct SnakeState *state)
{
    // Method to get the current time. // 时间获取方法
    struct timespec currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime); // Get the current time in monotonic clock. // 获取单调时钟的当前时间

    pthread_mutex_lock(&state->snakePauseState.pauseMutex); // Lock the pause mutex.

    // Last pause time. // 上次暂停时间
    struct timespec *lastPauseTime = &(state->snakePauseState.lastPauseTime);
    // Calculate the elapsed time (in seconds). // 计算时间间隔（秒）
    double elapsedTime =
        (currentTime.tv_sec - lastPauseTime->tv_sec) +                 // Calculate seconds difference.
        (currentTime.tv_nsec - lastPauseTime->tv_nsec) / 1000000000.0; // Calculate nanoseconds difference.

    // Check if the elapsed time exceeds 0.5 seconds. // 检查是否超过0.5秒间隔
    if (elapsedTime < 0.5)
    {
        pthread_mutex_unlock(&state->snakePauseState.pauseMutex); // Unlock the pause mutex.
        return false;                                             // The trigger interval has not been reached, return directly. // 未达到触发间隔，直接返回
    }

    // Update the last pause time. // 更新最后暂停时间
    *lastPauseTime = currentTime;

    // Toggle pause state. // 切换暂停状态
    atomic_store(&state->snakePauseState.isPaused, !state->snakePauseState.isPaused);

    // Notify any waiting threads. // Notify可能等待的线程
    pthread_cond_broadcast(&state->snakePauseState.pauseCond);

    pthread_mutex_unlock(&state->snakePauseState.pauseMutex); // Unlock the pause mutex.
    return true;                                              // Return true to indicate successful pause. // 返回true表示成功暂停
}

bool SnakeGameStateJoin(struct SnakeState *state)
{
    // Check if the input status is valid//检查输入状态是否有效
    if (state == NULL)
    {
        return false;
    }

    // Allocate memory for new node//为新节点分配内存
    struct AllOfsnakeState *newNode = malloc(sizeof(struct AllOfsnakeState));
    if (newNode == NULL)
    {
        return false;
    }

    // Initialize new node//初始化新节点
    newNode->state = state;
    newNode->next = NULL;

    // Lock to protect linked list operation//加锁，保护链表操作
    pthread_mutex_lock(&g_snakeStateMutex);

    // Special circumstances to handle the linked list to empty//处理链表为空的特殊情况
    if (g_snakeStateHead == NULL)
    {
        g_snakeStateHead = newNode;
    }
    else
    {
        // Find the end of the linked list and insert a new node// 找到链表末尾并插入新节点
        struct AllOfsnakeState *current = g_snakeStateHead;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = newNode;
    }

    // unlock mutex//解锁
    pthread_mutex_unlock(&g_snakeStateMutex);
    return true;
}

void SnakeGameAllClean(void)
{
    // Lock to protect global linked list operations//加锁，保护全局链表操作
    pthread_mutex_lock(&g_snakeStateMutex);

    // Traversing and cleaning up all game status 遍历并清理所有游戏状态
    struct AllOfsnakeState *current = g_snakeStateHead;
    while (current != NULL)
    {
        struct AllOfsnakeState *next = current->next;

        if (current->state != NULL && current->state->isrunning == true)
        {
            snakeGameExit(current->state);
        }
        // Clean up individual game state// 清理单个游戏状态
        if (current->state != NULL)
            SnakeGameClean(current->state);

        //  Release linked list node//释放链表节点
        free(current);
        current = next;
    }

    // Reset the global linked list//重置全局链表头
    g_snakeStateHead = NULL;
    // Unlock//解锁
    pthread_mutex_unlock(&g_snakeStateMutex);

    // Destroy mutex//销毁互斥锁
    pthread_mutex_destroy(&g_snakeStateMutex);
}

bool SnakeInputDebounceControl(long long *last_key_timestamp, long long min_interval_us)
{
    struct timespec current_time;                  // Current system time structure // 当前系统时间结构
    clock_gettime(CLOCK_MONOTONIC, &current_time); // Get monotonic time // 获取单调时间

    // Calculate current timestamp in microseconds // 计算当前微秒时间戳
    long long current_timestamp =
        current_time.tv_sec * 1000000LL + current_time.tv_nsec / 1000;

    // load previous timestamp // 加载上一个时间戳
    long long prev_timestamp = *last_key_timestamp;

    // Calculate time difference // 计算时间差
    long long time_diff = current_timestamp - prev_timestamp;

    // Reject input if time interval is insufficient // 如果时间间隔不足则拒绝输入
    if (time_diff < min_interval_us)
    {
        return false; // Input rejected due to short interval // 由于间隔过短拒绝输入
    }

    // update timestamp // 更新时间戳
    *last_key_timestamp = current_timestamp;

    return true; // Input allowed // 允许输入
}

void snakeGameExit(struct SnakeState *state)
{
    if (state == NULL)
        return;
    if (state->isrunning == true)
    {
        pthread_mutex_lock(&state->snakeGameState.gameMutex);
        atomic_store(&(state->snakeGameState.gameOver), true);
        atomic_store(&(state->snakeGameState.gameOverByUser), true);
        pthread_cond_broadcast(&state->snakeGameState.updateCond);
        pthread_mutex_unlock(&state->snakeGameState.gameMutex);
    }
}

bool snakeGameIsRunning(struct SnakeState *state)
{
    if (state == NULL)
        return false;
    return state->isrunning;
}

/**
 * @brief Determine if the snake game is paused // 判断贪吃蛇游戏是否暂停
 * @param state The snake game state // 贪吃蛇游戏状态
 * @return Whether the snake game is paused // 是否暂停
 */
bool snakeGameIsPaused(struct SnakeState *state)
{
    if (state == NULL)
        return false;

    return atomic_load(&state->snakePauseState.isPaused);
}