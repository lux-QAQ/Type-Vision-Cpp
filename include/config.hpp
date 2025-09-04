#pragma once

namespace type_vision::static_parse
{

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