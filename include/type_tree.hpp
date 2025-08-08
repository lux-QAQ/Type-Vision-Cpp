#pragma once
#include "yalantinglibs/include/ylt/reflection/member_names.hpp"
#include "yalantinglibs/include/ylt/reflection/member_ptr.hpp"
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <type_traits>
#include <cstdint>  // for std::uint*_t
#include <cstring>  // for memcpy
#include <array>    // for std::array
#include <map>
#include <utility>

#include "config.hpp"

#if __has_include(<format>)
#include <format>
#endif

namespace type_vision::static_parse
{

//  为 Parser 添加前向声明
template <typename T>
struct Parser;

//  现有代码：颜色配置和预设方案
struct HighlightConfig
{
    std::uint32_t type;
    std::uint32_t nttp;
    std::uint32_t tmpl;
    std::uint32_t builtin;
    std::uint32_t modifier;
    std::uint32_t tag;
    bool show_size_align = true;  // <-- 新增配置项
};

constexpr static inline HighlightConfig Dark = {
        .type = 0xE5C07B,      // Light Yellow
        .nttp = 0xD19A66,      // Orange
        .tmpl = 0x61AFEF,      // Blue
        .builtin = 0xC678DD,   // Purple
        .modifier = 0x98C379,  // Green
        .tag = 0x5C6370,       // Grey
};

constexpr static inline HighlightConfig Light = {
        .type = 0x4285F4,      // Blue
        .nttp = 0x0F9D58,      // Green
        .tmpl = 0xFF5722,      // Deep Orange
        .builtin = 0x7C4DFF,   // Deep Purple
        .modifier = 0xFBC02D,  // Yellow
        .tag = 0x03A9F4,       // Light Blue
};

template <typename T>
constexpr std::string_view raw_name_of()
{
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view pretty_function = __PRETTY_FUNCTION__;
    constexpr size_t start = pretty_function.find('=') + 2;
    constexpr size_t end = pretty_function.find_first_of("];", start);
    return pretty_function.substr(start, end - start);
#elif defined(_MSC_VER)
    constexpr std::string_view pretty_function = __FUNCSIG__;
    constexpr std::string_view prefix = "class std::basic_string_view<char,struct std::char_traits<char> > __cdecl type_vision::static_parse::raw_name_of<";
    constexpr std::string_view suffix = ">(void)";
    constexpr size_t start = pretty_function.find(prefix) + prefix.size();
    constexpr size_t end = pretty_function.rfind(suffix);
    return pretty_function.substr(start, end - start);
#else
    return "unknown_type";
#endif
}

// 前向声明
template <typename Pointee>
struct StaticPointer;
template <typename T>
struct StaticBasicType;
template <auto V>
struct StaticValue;
template <typename T>
struct StaticEnum;  // <-- 新增前向声明
template <typename ClassType, typename MemberType>
struct StaticMemberDataPointer;
template <typename ClassType, typename ReturnType, typename ArgsTuple, typename Qualifiers>
struct StaticMemberFunctionPointer;
template <typename ReturnType, typename ArgsTuple, bool IsNoexcept, bool IsVariadic>
struct StaticFunction;

// 打印机制
namespace details
{
//  编译期枚举反射工具
struct enum_reflection
{
    template <auto value>
    static constexpr auto enum_name()
    {
        std::string_view name;
#if __GNUC__ || __clang__
        name = __PRETTY_FUNCTION__;
        std::size_t start = name.find('=') + 2;
        std::size_t end = name.size() - 1;
        name = std::string_view{name.data() + start, end - start};
        if (name.find('(') != std::string_view::npos)
        {
            return std::string_view{};
        }
        start = name.rfind("::");
#elif _MSC_VER
        name = __FUNCSIG__;
        std::size_t start = name.find('<') + 1;
        std::size_t end = name.rfind(">(");
        name = std::string_view{name.data() + start, end - start};
        if (name.find('(') != std::string_view::npos)
        {
            return std::string_view{};
        }
        start = name.rfind("::");
#endif
        return start == std::string_view::npos ? name : std::string_view{name.data() + start + 2, name.size() - start - 2};
    }

    template <typename T, std::size_t N = 0>
    static constexpr auto enum_max()
    {
        if constexpr (N > 1024)
        {
            return N;
        }
        else
        {
            constexpr auto name = enum_name<static_cast<T>(N)>();
            if constexpr (name.empty())
            {
                return N;
            }
            else
            {
                return enum_max<T, N + 1>();
            }
        }
    }

    template <typename T>
        requires std::is_enum_v<T>
    static constexpr auto get_enum_pairs()
    {
        // constexpr auto num = enum_max<T>();
        constexpr auto num = 256;
        std::array<std::pair<T, std::string_view>, num> pairs{};
        std::size_t valid_count = 0;

        [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            ((
                     [&]()
                     {
                         constexpr auto name = enum_name<static_cast<T>(Is)>();
                         if constexpr (!name.empty())
                         {
                             pairs[valid_count++] = {static_cast<T>(Is), name};
                         }
                     }()),
             ...);
        }(std::make_index_sequence<num>{});

        return std::make_pair(pairs, valid_count);
    }
};

#ifndef __cpp_lib_format
template <typename... Args>
std::string format(std::string_view fmt, Args&&... args)
{
    std::array<char, 256> buffer{};
    std::size_t args_index = 0;
    std::array<std::string, sizeof...(Args)> args_str{};
    auto make_str = [&](auto&& arg)
    {
        using Type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_integral_v<Type> || std::is_floating_point_v<Type>)
        {
            args_str[args_index] = std::to_string(arg);
        }
        else if constexpr (std::is_same_v<Type, std::string>)
        {
            args_str[args_index] = std::forward<decltype(arg)>(arg);
        }
        else if constexpr (std::is_constructible_v<std::string, Type>)
        {
            args_str[args_index] = std::string{arg};
        }
        args_index += 1;
    };
    (make_str(std::forward<decltype(args)>(args)), ...);

    args_index = 0;
    std::size_t buffer_index = 0;
    for (std::size_t fmt_index = 0; fmt_index < fmt.size();)
    {
        if (fmt[fmt_index] == '{' && fmt[fmt_index + 1] == '}')
        {
            fmt_index += 2;
            std::size_t size = args_str[args_index].size();
            if (buffer_index + size < buffer.size())
            {
                memcpy(buffer.data() + buffer_index, args_str[args_index].data(), size);
                buffer_index += size;
            }
            args_index += 1;
        }
        else
        {
            if (buffer_index < buffer.size())
            {
                buffer[buffer_index] = fmt[fmt_index];
                buffer_index += 1;
            }
            fmt_index += 1;
        }
    }
    return std::string{buffer.data(), buffer_index};
}
#else
using std::format;
#endif

template <typename T>
std::string colorize(T&& text, std::uint32_t color, bool enable_color)
{
    std::string str_text;
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
    {
        str_text = std::forward<T>(text);
    }
    else if constexpr (std::is_convertible_v<T, std::string_view>)
    {
        str_text = std::string(std::forward<T>(text));
    }
    else
    {
        str_text = std::to_string(std::forward<T>(text));
    }

    if (!enable_color || str_text.empty())
    {
        return str_text;
    }
    std::uint8_t R = (color >> 16) & 0xFF;
    std::uint8_t G = (color >> 8) & 0xFF;
    std::uint8_t B = color & 0xFF;
    return format("\033[38;2;{};{};{}m{}\033[0m", R, G, B, str_text);
}

static std::string format_qualifiers(const std::vector<std::string_view>& qualifiers, const HighlightConfig& config, bool enable_color)
{
    if (qualifiers.empty()) return "";
    std::string result = " [";
    for (size_t i = 0; i < qualifiers.size(); ++i)
    {
        result += colorize(qualifiers[i], config.modifier, enable_color);
        if (i < qualifiers.size() - 1)
        {
            result += ", ";
        }
    }
    result += "]";
    return result;
}

struct ReflectedMembersPrinter
{
private:
    //  将这些成员移到结构体顶层作用域
    template <typename T>
    struct MembersInfo
    {
        // 1. 在编译期获取所有成员的名称
        static constexpr auto member_names_ = ylt::reflection::get_member_names<T>();

        // 2. 在编译期获取所有成员的类型
        using members_as_tuple_ = decltype(ylt::reflection::object_to_tuple(std::declval<T&>()));

        // 3. 递归地解析每一个成员的类型
        template <typename Tuple, size_t... Is>
        static auto parse_members(std::index_sequence<Is...>)
        {
            return std::make_tuple(typename Parser<std::remove_reference_t<std::tuple_element_t<Is, Tuple>>>::type{}...);
        }
        static inline const auto parsed_members_ = parse_members<members_as_tuple_>(std::make_index_sequence<member_names_.size()>{});
    };

public:
    template <typename T>
    static void print(std::string prefix, const HighlightConfig& config, bool enable_color)
    {
        // 使用内部的 MembersInfo 结构体来访问静态成员
        constexpr auto& member_names_ = MembersInfo<T>::member_names_;
        const auto& parsed_members_ = MembersInfo<T>::parsed_members_;

        if constexpr (member_names_.empty())
        {
            return;  // 如果没有成员，直接返回
        }

        // 4. 运行时遍历并打印每个成员
        [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            (
                    [&]()
                    {
                        constexpr bool is_last_member = (Is == member_names_.size() - 1);
                        auto& member_parser = std::get<Is>(parsed_members_);

                        // 打印成员名称
                        std::cout << prefix << (is_last_member ? "└── " : "├── ")
                                  << details::colorize(member_names_[Is], config.modifier, enable_color)
                                  << details::colorize(": ", config.tag, enable_color);

                        // 递归打印成员的类型树
                        std::vector<std::string_view> member_qualifiers;
                        member_parser.print_impl(prefix, is_last_member, member_qualifiers, true, config, enable_color);
                    }(),
                    ...);
        }(std::make_index_sequence<member_names_.size()>{});
    }
};

}  // namespace details

template <typename Derived>
struct StaticTypeCRTP
{
    static void print(bool enable_color = true, const HighlightConfig& config = Dark, std::string prefix = "", bool is_last = true)
    {
        std::vector<std::string_view> qualifiers;
        Derived::print_impl(prefix, is_last, qualifiers, false, config, enable_color);
    }

    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color);
};

template <auto V>
struct StaticValue : StaticTypeCRTP<StaticValue<V>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }
        std::cout << details::colorize("Value: ", config.tag, enable_color)
                  << details::colorize(V, config.nttp, enable_color)
                  << " (type: " << details::colorize(raw_name_of<decltype(V)>(), config.type, enable_color) << ")"
                  << details::format_qualifiers(qualifiers, config, enable_color) << '\n';
    }
};

template <typename T>
struct StaticBasicType : StaticTypeCRTP<StaticBasicType<T>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }
        std::cout << details::colorize(raw_name_of<T>(), config.type, enable_color)
                  << details::format_qualifiers(qualifiers, config, enable_color);

        // 同时排除 void 和函数类型，避免对它们调用 sizeof
        if constexpr (!std::is_void_v<T> && !std::is_function_v<T>)
        {
            // 仅当 T 不是 void 或函数时，才实例化包含 sizeof 的代码块
            if constexpr (sizeof(T) > 0)
            {
                if (global_config.type_size_align && config.show_size_align)
                {
                    std::cout << details::colorize(" [size: ", config.tag, enable_color)
                              << details::colorize(sizeof(T), config.nttp, enable_color)
                              << details::colorize(", align: ", config.tag, enable_color)
                              << details::colorize(alignof(T), config.nttp, enable_color)
                              << details::colorize("]", config.tag, enable_color);
                }
            }
        }

        std::cout << '\n';
    }
};

//  StaticEnum 专门用于打印枚举类型
template <typename T>
struct StaticEnum : StaticTypeCRTP<StaticEnum<T>>
{
private:
    // 将编译期计算结果存储为静态成员变量。
    // 这样，它们只在编译时计算一次，并成为类型的一部分。
    static constexpr auto pairs_info_ = details::enum_reflection::get_enum_pairs<T>();
    static constexpr auto& pairs_ = pairs_info_.first;
    static constexpr auto count_ = pairs_info_.second;

public:
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }

        // 打印 "Enum: " 标签和枚举类型名
        std::cout << details::colorize("Enum: ", config.tag, enable_color)
                  << details::colorize(raw_name_of<T>(), config.type, enable_color);

        // 在运行时函数中，直接使用预先计算好的静态成员。
        if (count_ > 0)
        {
            std::string members_str = " [";
            size_t limit = std::min((size_t)10, (size_t)count_);

            for (size_t i = 0; i < limit; ++i)
            {
                // 添加键名
                members_str += details::colorize(pairs_[i].second, config.modifier, enable_color);
                // 添加 = 和值
                members_str += details::colorize("=", config.tag, enable_color);
                using underlying_type = std::underlying_type_t<T>;
                members_str += details::colorize(static_cast<underlying_type>(pairs_[i].first), config.nttp, enable_color);

                if (i < limit - 1)
                {
                    members_str += details::colorize(", ", config.tag, enable_color);
                }
            }

            if (count_ > 10)
            {
                members_str += details::colorize(", ...", config.tag, enable_color);
            }
            members_str += "]";
            std::cout << members_str;
        }

        std::cout << '\n';
    }
};

template <typename Pointee>
struct StaticPointer : StaticTypeCRTP<StaticPointer<Pointee>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }
        std::cout << details::colorize("Pointer", config.builtin, enable_color)
                  << details::format_qualifiers(qualifiers, config, enable_color) << '\n';

        std::string new_prefix = prefix + (is_last ? "    " : "│   ");
        Pointee::print(enable_color, config, new_prefix, true);
    }
};

template <typename QualifiedType>
struct StaticConst : StaticTypeCRTP<StaticConst<QualifiedType>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        qualifiers.push_back("const");
        QualifiedType::print_impl(prefix, is_last, qualifiers, is_inlined, config, enable_color);
    }
};

template <typename QualifiedType>
struct StaticVolatile : StaticTypeCRTP<StaticVolatile<QualifiedType>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        qualifiers.push_back("volatile");
        QualifiedType::print_impl(prefix, is_last, qualifiers, is_inlined, config, enable_color);
    }
};

template <typename ReferencedType>
struct StaticLValueReference : StaticTypeCRTP<StaticLValueReference<ReferencedType>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }
        std::cout << details::colorize("Ref", config.builtin, enable_color) << " "
                  << details::colorize("[LValue]", config.tag, enable_color) << '\n';
        std::string new_prefix = prefix + (is_last ? "    " : "│   ");
        ReferencedType::print(enable_color, config, new_prefix, true);
    }
};

template <typename ReferencedType>
struct StaticRValueReference : StaticTypeCRTP<StaticRValueReference<ReferencedType>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }
        std::cout << details::colorize("Ref", config.builtin, enable_color) << " "
                  << details::colorize("[RValue]", config.tag, enable_color) << '\n';
        std::string new_prefix = prefix + (is_last ? "    " : "│   ");
        ReferencedType::print(enable_color, config, new_prefix, true);
    }
};

namespace details
{
struct TemplateArgsPrinter
{
    template <size_t I, typename First, typename... Rest>
    static void print(std::string prefix, const HighlightConfig& config, bool enable_color, bool is_last_group)
    {
        constexpr bool is_last_arg = (sizeof...(Rest) == 0);
        // 如果这是最后一组要打印的东西，则最后一个参数使用 '└──'
        std::cout << prefix << ((is_last_arg && is_last_group) ? "└── " : "├── ") << details::colorize(std::to_string(I) + ": ", config.tag, enable_color);

        std::vector<std::string_view> arg_qualifiers;
        First::print_impl(prefix, (is_last_arg && is_last_group), arg_qualifiers, true, config, enable_color);

        if constexpr (sizeof...(Rest) > 0)
        {
            TemplateArgsPrinter::template print<I + 1, Rest...>(prefix, config, enable_color, is_last_group);
        }
    }
};
}  // namespace details

template <typename T, typename... ParsedArgs>
struct StaticTemplateType : StaticTypeCRTP<StaticTemplateType<T, ParsedArgs...>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }
        auto full_name = raw_name_of<T>();
        auto template_name = full_name.substr(0, full_name.find('<'));
        std::cout << details::colorize("Template: ", config.tag, enable_color)
                  << details::colorize(template_name, config.tmpl, enable_color)
                  << details::format_qualifiers(qualifiers, config, enable_color) << '\n';

        std::string new_prefix = prefix + (is_last ? "    " : "│   ");

        //  调整打印逻辑以插入 "Class Members" 标签
        constexpr bool has_args = sizeof...(ParsedArgs) > 0;
        constexpr bool is_reflected = std::is_aggregate_v<T> || ylt::reflection::is_ylt_refl_v<T>;

        if constexpr (has_args)
        {
            // 如果有模板参数，使用 TemplateArgsPrinter 打印它们
            details::TemplateArgsPrinter::template print<0, ParsedArgs...>(new_prefix, config, enable_color, !is_reflected);
        }

        if constexpr (is_reflected)
        {
            // 检查是否有成员可以打印
            if constexpr (ylt::reflection::members_count_v<T> > 0)
            {
                // 打印 "Class Members" 标签
                std::cout << new_prefix << "└── " << details::colorize("Class Members", config.tag, enable_color) << '\n';

                // 为实际成员创建更深一级的缩进
                std::string members_prefix = new_prefix + "    ";
                details::ReflectedMembersPrinter::print<T>(members_prefix, config, enable_color);
            }
        }
    }
};

template <typename Element, std::size_t N>
struct StaticArray : StaticTypeCRTP<StaticArray<Element, N>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }
        std::cout << details::colorize("Array", config.builtin, enable_color)
                  << " [N = " << details::colorize(N, config.nttp, enable_color) << "]"
                  << details::format_qualifiers(qualifiers, config, enable_color) << '\n';

        std::string new_prefix = prefix + (is_last ? "    " : "│   ");
        Element::print(enable_color, config, new_prefix, true);
    }
};

template <typename ClassType, typename MemberType>
struct StaticMemberDataPointer : StaticTypeCRTP<StaticMemberDataPointer<ClassType, MemberType>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }
        std::cout << details::colorize("Member Data Pointer", config.builtin, enable_color)
                  << details::format_qualifiers(qualifiers, config, enable_color) << '\n';
        std::string new_prefix = prefix + (is_last ? "    " : "│   ");

        std::cout << new_prefix << "├── " << details::colorize("Class:", config.tag, enable_color) << '\n';
        ClassType::print(enable_color, config, new_prefix + "│   ", true);

        std::cout << new_prefix << "└── " << details::colorize("Type:", config.tag, enable_color) << '\n';
        MemberType::print(enable_color, config, new_prefix + "    ", true);
    }
};

namespace details
{
template <bool IsVariadic>
struct FunctionArgsPrinter
{
    template <size_t I, typename First, typename... Rest>
    static void print(std::string prefix, const HighlightConfig& config, bool enable_color)
    {
        constexpr bool is_last_arg = (sizeof...(Rest) == 0) && !IsVariadic;
        std::cout << prefix << (is_last_arg ? "└── " : "├── ") << details::colorize(std::to_string(I) + ": ", config.tag, enable_color);
        std::vector<std::string_view> arg_qualifiers;
        First::print_impl(prefix, is_last_arg, arg_qualifiers, true, config, enable_color);

        if constexpr (sizeof...(Rest) > 0)
        {
            FunctionArgsPrinter<IsVariadic>::template print<I + 1, Rest...>(prefix, config, enable_color);
        }
    }
};
}  // namespace details

template <typename ReturnType, typename... Args, bool IsNoexcept, bool IsVariadic>
struct StaticFunction<ReturnType, std::tuple<Args...>, IsNoexcept, IsVariadic>
    : StaticTypeCRTP<StaticFunction<ReturnType, std::tuple<Args...>, IsNoexcept, IsVariadic>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (IsNoexcept)
        {
            qualifiers.push_back("noexcept");
        }

        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }
        std::cout << details::colorize("Function", config.builtin, enable_color)
                  << details::format_qualifiers(qualifiers, config, enable_color) << '\n';

        std::string new_prefix = prefix + (is_last ? "    " : "│   ");
        constexpr bool has_args = sizeof...(Args) > 0;
        constexpr bool has_children = has_args || IsVariadic;

        std::cout << new_prefix << (has_children ? "├── " : "└── ") << details::colorize("R: ", config.tag, enable_color);
        std::vector<std::string_view> ret_qualifiers;
        ReturnType::print_impl(new_prefix, !has_children, ret_qualifiers, true, config, enable_color);

        if constexpr (has_args)
        {
            details::FunctionArgsPrinter<IsVariadic>::template print<0, Args...>(new_prefix, config, enable_color);
        }

        if (IsVariadic)
        {
            std::cout << new_prefix << "└── " << details::colorize("...", config.builtin, enable_color) << "\n";
        }
    }
};

namespace details
{
struct MemberFunctionArgsPrinter
{
    template <size_t I, typename First, typename... Rest>
    static void print(std::string prefix, const HighlightConfig& config, bool enable_color)
    {
        constexpr bool is_last_arg = (sizeof...(Rest) == 0);
        std::cout << prefix << (is_last_arg ? "└── " : "├── ") << details::colorize(std::to_string(I) + ": ", config.tag, enable_color);
        std::vector<std::string_view> arg_qualifiers;
        First::print_impl(prefix, is_last_arg, arg_qualifiers, true, config, enable_color);

        if constexpr (sizeof...(Rest) > 0)
        {
            MemberFunctionArgsPrinter::template print<I + 1, Rest...>(prefix, config, enable_color);
        }
    }
};
}  // namespace details

template <bool C, bool V, bool LR, bool RR, bool NE>
struct Qualifiers
{
};

template <typename ClassType, typename ReturnType, typename... Args, bool C, bool V, bool LR, bool RR, bool NE>
struct StaticMemberFunctionPointer<ClassType, ReturnType, std::tuple<Args...>, Qualifiers<C, V, LR, RR, NE>>
    : StaticTypeCRTP<StaticMemberFunctionPointer<ClassType, ReturnType, std::tuple<Args...>, Qualifiers<C, V, LR, RR, NE>>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (C) qualifiers.push_back("const");
        if (V) qualifiers.push_back("volatile");
        if (LR) qualifiers.push_back("&");
        if (RR) qualifiers.push_back("&&");
        if (NE) qualifiers.push_back("noexcept");

        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }
        std::cout << details::colorize("Member Function Pointer", config.builtin, enable_color)
                  << details::format_qualifiers(qualifiers, config, enable_color) << '\n';

        std::string new_prefix = prefix + (is_last ? "    " : "│   ");
        constexpr bool has_args = sizeof...(Args) > 0;

        std::cout << new_prefix << "├── " << details::colorize("Class:", config.tag, enable_color) << '\n';
        ClassType::print(enable_color, config, new_prefix + "│   ", true);

        std::cout << new_prefix << (has_args ? "├── " : "└── ") << details::colorize("R: ", config.tag, enable_color);
        std::vector<std::string_view> ret_qualifiers;
        ReturnType::print_impl(new_prefix, !has_args, ret_qualifiers, true, config, enable_color);

        if constexpr (has_args)
        {
            details::MemberFunctionArgsPrinter::template print<0, Args...>(new_prefix, config, enable_color);
        }
    }
};

template <typename T>
struct StaticUnion : StaticTypeCRTP<StaticUnion<T>>
{
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }

        std::cout << details::colorize("Union: ", config.tag, enable_color)
                  << details::colorize(raw_name_of<T>(), config.type, enable_color)
                  << details::format_qualifiers(qualifiers, config, enable_color);

        if constexpr (!std::is_void_v<T> && !std::is_function_v<T>)
        {
            if constexpr (sizeof(T) > 0)
            {
                if (global_config.type_size_align)
                {
                    std::cout << details::colorize(" [size: ", config.tag, enable_color)
                              << details::colorize(sizeof(T), config.nttp, enable_color)
                              << details::colorize(", align: ", config.tag, enable_color)
                              << details::colorize(alignof(T), config.nttp, enable_color)
                              << details::colorize("]", config.tag, enable_color);
                }
            }
        }
        std::cout << '\n';
    }
};

//  将 StaticReflectedStruct 的定义移回到命名空间内部
template <typename T>
struct StaticReflectedStruct : StaticTypeCRTP<StaticReflectedStruct<T>>
{
public:
    static void print_impl(std::string prefix, bool is_last, std::vector<std::string_view>& qualifiers, bool is_inlined, const HighlightConfig& config, bool enable_color)
    {
        if (!is_inlined)
        {
            std::cout << prefix << (is_last ? "└── " : "├── ");
        }

        // 打印 "Struct: " 标签和类型名
        std::cout << details::colorize("Struct: ", config.tag, enable_color)
                  << details::colorize(raw_name_of<T>(), config.type, enable_color)
                  << details::format_qualifiers(qualifiers, config, enable_color) << '\n';

        std::string new_prefix = prefix + (is_last ? "    " : "│   ");

        //  调用可复用的打印器
        details::ReflectedMembersPrinter::print<T>(new_prefix, config, enable_color);
    }
};

}  // namespace type_vision::static_parse