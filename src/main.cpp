#include "parser.hpp"
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#include <map>
#include <array>
#include <type_traits>

// 用于打印类型解析结果的辅助函数
template <typename T>
void run_test()
{
    // 使用 std::decay_t 来处理数组到指针的退化，避免模板特化歧义
    using DecayedType = std::decay_t<T>;

    type_vision::static_parse::Parser<DecayedType>::type::print();
    std::cout << "\n"
              << std::endl;
}

// 用于测试的函数模板
template <typename T, int N>
auto arr_ret()
{
    return std::array<T, N>{};
}

// 用于测试的类模板
template <typename T, int N>
class TestClass
{
public:
    using type = T;
    static constexpr int size = N;
    T arr[N];
};

// 用于测试的结构体和枚举
struct MyClass
{
    int member;
    double other_member;
};

enum class MyEnum
{
    A = 20,
    B
};

// --- 从 README.md 添加的定义 ---
enum class Color
{
    Red = 1,
    Green,
    Blue,
    Yellow,
    Purple
};

class Person
{
public:
    std::string name;
    int age;
    int id;
    Color favorite_color;
};

template <int N>
struct Array
{
    int data[N];
};
// --- 定义结束 ---

namespace test_namespace
{
namespace inner_namespace
{
class myDerived
{
public:
    int b;
};
}  // namespace inner_namespace
}  // namespace test_namespace

int main()
{
    using namespace test_namespace;
    run_test<inner_namespace::myDerived>();

    return 0;
}