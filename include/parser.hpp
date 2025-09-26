#pragma once
// 包含 yalantinglibs 的头文件，is_aggregate_v 在 <type_traits> 中
#include "ylt/reflection/user_reflect_macro.hpp"
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
// =================================================================
// ====================== Lambda 解析逻辑 ==========================
// =================================================================

// 1. 类型萃取：判断 T 是否为 Lambda
template <typename T, typename = void>
struct is_lambda : std::false_type
{
};

template <typename T>
struct is_lambda<T, std::void_t<decltype(&T::operator())>> : std::true_type
{
};





template <typename T>
constexpr bool is_lambda_v = is_lambda<std::remove_cvref_t<T>>::value;

// 2. 辅助模板：用于拆解 operator() 的类型
template <typename T>
struct lambda_signature_parser;

// 基础形式 (无任何限定符) - 这是我们唯一需要手动定义的
template <typename R, typename C, typename... Args>
struct lambda_signature_parser<R (C::*)(Args...)>
{
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    static constexpr bool is_lvalue_ref = false;
    static constexpr bool is_rvalue_ref = false;
    static constexpr bool is_noexcept = false;
};

// 包含由脚本生成的所有其他限定符组合
#include "inc/lambda_signature_parser.inc"

// 3. 主 Lambda 解析器，负责构建 StaticLambda 节点
template <typename T>
struct LambdaParser
{
private:
    using signature_parser = details::lambda_signature_parser<decltype(&T::operator())>;
    using return_type = typename signature_parser::return_type;
    using args_tuple = typename signature_parser::args_tuple;

    // 递归地解析返回类型
    using parsed_return_type = typename Parser<return_type>::type;

    // 递归地解析所有参数类型，并将结果打包成一个 std::tuple
    template <typename Tuple, size_t... Is>
    static auto parse_args_helper(std::index_sequence<Is...>)
    {
        return std::tuple<typename Parser<std::tuple_element_t<Is, Tuple>>::type...>{};
    }
    using parsed_args_tuple = decltype(parse_args_helper<args_tuple>(std::make_index_sequence<std::tuple_size_v<args_tuple>>{}));

public:
    // 最终构建出的类型树节点 - 传递所有解析出的限定符
    using type = StaticLambda<T, parsed_return_type, parsed_args_tuple,
                              signature_parser::is_const,
                              signature_parser::is_volatile,
                              signature_parser::is_lvalue_ref,
                              signature_parser::is_rvalue_ref,
                              signature_parser::is_noexcept>;
};

// 统一的参数包装器
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

// 通过大量特化来提取模板参数
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

// 新增：用于延迟实例化的基础类型选择器
template <typename T, bool is_lambda>
struct BasicTypeSelector;

// is_lambda 为 true 的特化
template <typename T>
struct BasicTypeSelector<T, true>
{
    using type = typename LambdaParser<T>::type;
};

// is_lambda 为 false 的特化
template <typename T>
struct BasicTypeSelector<T, false>
{
    using type = std::conditional_t<
            (std::is_aggregate_v<T> && !std::is_array_v<T>) || ylt::reflection::is_ylt_refl_v<T>,
            StaticReflectedStruct<T>,
            std::conditional_t<
                    std::is_enum_v<T>,
                    StaticEnum<T>,
                    std::conditional_t<
                            std::is_union_v<T>,
                            StaticUnion<T>,
                            StaticBasicType<T>>>>;
};

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

    // 使用 BasicTypeSelector 来延迟实例化，避免编译错误
    using type = typename details::BasicTypeSelector<clean_t, details::is_lambda_v<clean_t>>::type;
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

// 主解析器模板
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

// 为 const 数组添加更具体的特化以解决歧义
template <typename T, std::size_t N>
struct Parser<const T[N]>
{
    using type = StaticArray<typename Parser<const T>::type, N>;
};

template <typename T>
struct Parser<const T[]>
{
    using type = StaticArray<typename Parser<const T>::type, 0>;
};

// 为 volatile 和 const volatile 数组添加特化以解决歧义
template <typename T, std::size_t N>
struct Parser<volatile T[N]>
{
    using type = StaticArray<typename Parser<volatile T>::type, N>;
};

template <typename T>
struct Parser<volatile T[]>
{
    using type = StaticArray<typename Parser<volatile T>::type, 0>;
};

template <typename T, std::size_t N>
struct Parser<const volatile T[N]>
{
    using type = StaticArray<typename Parser<const volatile T>::type, N>;
};

template <typename T>
struct Parser<const volatile T[]>
{
    using type = StaticArray<typename Parser<const volatile T>::type, 0>;
};

// 成员数据指针的特化
template <typename T, typename C>
struct Parser<T C::*>
{
    using type = StaticMemberDataPointer<typename Parser<C>::type, typename Parser<T>::type>;
};

// 函数特化
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

// 成员函数指针的特化
// 基础形式 (无任何限定符)
template <typename R, typename C, typename... Args>
struct Parser<R (C::*)(Args...)>
{
    using type = StaticMemberFunctionPointer<typename Parser<C>::type, typename Parser<R>::type, std::tuple<typename Parser<Args>::type...>, Qualifiers<false, false, false, false, false>>;
};

// 包含由脚本生成的所有其他限定符组合
#include "inc/member_function_parser.inc"

}  // namespace type_vision::static_parse