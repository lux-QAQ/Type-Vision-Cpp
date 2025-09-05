#pragma once

namespace type_vision::static_parse
{

#ifdef _WIN32
// 在包含 windows.h 之前定义这些宏，可以避免最常见的宏冲突
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace UTF_8_CONSOLE
{
// 在 Windows 上自动设置控制台代码页为 UTF-8 (65001)。
struct ConsoleUtf8Setter
{
    // 将构造函数直接在头文件中定义为 inline
    // 这样即使头文件被多次包含，也不会产生链接错误。
    ConsoleUtf8Setter()
    {
        // 设置控制台输出代码页为 UTF-8
        SetConsoleOutputCP(CP_UTF8);
        // 同时设置输入代码页，以保持一致
        SetConsoleCP(CP_UTF8);
    }
};

// 静态实例，其构造函数会在 main 函数之前被调用。
inline const static ConsoleUtf8Setter console_setter;
}  // namespace UTF_8_CONSOLE
#endif

/**
 * @brief 控制类型可视化器打印行为的配置结构体。
 */
struct PrintConfig
{
    /**
     * @brief 如果为 true，则为每个类型打印其大小和对齐信息。
     *        例如: [size: 4, align: 4]
     */
    bool type_size_align = false;
};

// 可在运行时修改的全局配置实例。
inline const PrintConfig global_config;

}  // namespace type_vision::static_parse