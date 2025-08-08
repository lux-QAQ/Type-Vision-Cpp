#pragma once
// 包含 yalantinglibs 的头文件，is_aggregate_v 在 <type_traits> 中
#include "yalantinglibs/include/ylt/reflection/user_reflect_macro.hpp"
#include "type_tree.hpp"
#include <type_traits>
#include <tuple>

namespace type_vision::static_parse
{

// 前向声明
template <typename T>
struct Parser;

namespace details
{
// 1. 统一的参数包装器
// 用于包装非类型模板参数 (NTTP)
template <auto V>
struct value_wrapper
{
};

// 用于包装类型模板参数
template <typename T>
struct type_wrapper
{
};

// 2. "魔法"核心：通过大量特化来提取模板参数
// 主模板，用于非模板特化类型
template <typename T>
struct template_parser_helper
{
    static constexpr bool is_specialization = false;
};

// 删除所有手写的 template_parser_helper 特化，
// 例如从 "//  1 个参数 " 到 "// 为了简洁..." 的所有内容，
// 然后用下面的 #include 替换。

#include "inc/template_parser_helper.inc"

// 别名，方便后续使用
template <typename T>
using template_info = template_parser_helper<T>;

}  // namespace details

// 模板类型的解析器 (无需改变)
template <typename RawTemplateType, typename TupleOfArgs>
struct ParserForTemplate;

template <typename RawTemplateType, typename... Args>
struct ParserForTemplate<RawTemplateType, std::tuple<Args...>>
{
    // 对每个包装后的参数递归调用 Parser
    using type = StaticTemplateType<RawTemplateType, typename Parser<Args>::type...>;
};

// 基础类型处理 (自动识别聚合类型)
template <typename T>
struct BasicTypeWrapper
{
    // 移除 T 的 const/volatile/引用 修饰，以便 is_aggregate_v 能正确工作
    using clean_t = std::remove_cvref_t<T>;

    using type = std::conditional_t<
            // 1. 优先检查是否为聚合类型 (或手动反射的类型)
            (std::is_aggregate_v<clean_t> && !std::is_array_v<clean_t>) || ylt::reflection::is_ylt_refl_v<clean_t>,
            StaticReflectedStruct<clean_t>,
            // 2. 如果不是，再检查是否为枚举
            std::conditional_t<
                    std::is_enum_v<clean_t>,
                    StaticEnum<clean_t>,
                    // 3. 如果还不是，再检查是否为联合
                    std::conditional_t<
                            std::is_union_v<clean_t>,
                            StaticUnion<clean_t>,
                            // 4. 最后回退到普通基础类型
                            StaticBasicType<clean_t>>>>;
};

// 主解析器的分派器 (无需改变)
namespace details
{
template <bool is_spec, typename T>
struct ParserDispatcher;

template <typename T>
struct ParserDispatcher<true, T>
{
    using type = typename ParserForTemplate<T, typename template_info<T>::args_as_tuple>::type;
};

template <typename T>
struct ParserDispatcher<false, T>
{
    using type = typename BasicTypeWrapper<T>::type;
};
}  // namespace details

// 主解析器模板 (无需改变)
template <typename T>
struct Parser
{
    using type = typename details::ParserDispatcher<details::template_info<T>::is_specialization, T>::type;
};

// 3. 为包装器提供解析特化
// 用于解析 NTTP 值的特化
template <auto V>
struct Parser<details::value_wrapper<V>>
{
    using type = StaticValue<V>;
};

// 用于解析类型包装器的特化
template <typename T>
struct Parser<details::type_wrapper<T>>
{
    // "解包"并继续递归解析
    using type = typename Parser<T>::type;
};

//  所有其他特化保持不变

// 修饰符的特化
template <typename T>
struct Parser<const T>
{
    using type = StaticConst<typename Parser<T>::type>;
};
template <typename T>
struct Parser<volatile T>
{
    using type = StaticVolatile<typename Parser<T>::type>;
};
template <typename T>
struct Parser<const volatile T>
{
    using type = StaticConst<typename Parser<volatile T>::type>;
};

// 引用的特化
template <typename T>
struct Parser<T &>
{
    using type = StaticLValueReference<typename Parser<T>::type>;
};
template <typename T>
struct Parser<T &&>
{
    using type = StaticRValueReference<typename Parser<T>::type>;
};

// 基本类型的特化
template <typename T>
struct Parser<T *>
{
    using type = StaticPointer<typename Parser<T>::type>;
};
template <typename T, std::size_t N>
struct Parser<T[N]>
{
    using type = StaticArray<typename Parser<T>::type, N>;
};
template <typename T>
struct Parser<T[]>
{
    using type = StaticArray<typename Parser<T>::type, 0>;
};
// 成员数据指针的特化
template <typename T, typename C>
struct Parser<T C::*>
{
    using type = StaticMemberDataPointer<typename Parser<C>::type, typename Parser<T>::type>;
};

// 函数特化... (保持不变)
template <typename R, typename... Args>
struct Parser<R(Args...)>
{
    using type = StaticFunction<typename Parser<R>::type, std::tuple<typename Parser<Args>::type...>, false, false>;
};

template <typename R, typename... Args>
struct Parser<R(Args...) noexcept>
{
    using type = StaticFunction<typename Parser<R>::type, std::tuple<typename Parser<Args>::type...>, true, false>;
};

template <typename R, typename... Args>
struct Parser<R(Args..., ...)>
{
    using type = StaticFunction<typename Parser<R>::type, std::tuple<typename Parser<Args>::type...>, false, true>;
};

template <typename R, typename... Args>
struct Parser<R(Args..., ...) noexcept>
{
    using type = StaticFunction<typename Parser<R>::type, std::tuple<typename Parser<Args>::type...>, true, true>;
};

template <typename R, typename... Args>
struct Parser<std::function<R(Args...)>>
{
    // 直接将其视为一个普通的函数类型进行解析
    using type = typename Parser<R(Args...)>::type;
};

// 成员函数指针的特化... (保持不变)
// 基础形式
template <typename R, typename C, typename... Args>
struct Parser<R (C::*)(Args...)>
{
    using type = StaticMemberFunctionPointer<typename Parser<C>::type, typename Parser<R>::type, std::tuple<typename Parser<Args>::type...>, Qualifiers<false, false, false, false, false>>;
};
// ... 其他成员函数指针特化 ...
template <typename R, typename C, typename... Args>
struct Parser<R (C::*)(Args...) const && noexcept>
{
    using type = StaticMemberFunctionPointer<typename Parser<C>::type, typename Parser<R>::type, std::tuple<typename Parser<Args>::type...>, Qualifiers<true, false, false, true, true>>;
};

}  // namespace type_vision::static_parse