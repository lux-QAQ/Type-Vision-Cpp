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
    type_vision::static_parse::Parser<T>::type::print();
    std::cout << "\n"
              << std::endl;
}

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


// 为成员函数指针测试添加的结构体
struct MemberFuncTest
{
    void f_const() const
    {
    }
    int f_ref() &
    {
        return 0;
    }
    double f_rref_noexcept() && noexcept
    {
        return 0.0;
    }
    char f_cv_ref(int a) const volatile&
    {
        return 'a';
    }
};

//  定义结束

int main()
{

    
    // 基础类型
    using Case1 = int;
    run_test<Case1>();
    using Case2 = const double;
    run_test<Case2>();
    using Case3 = char* volatile;
    run_test<Case3>();

    // 指针和引用
    using Case4 = MyClass*;
    run_test<Case4>();
    using Case5 = const int&;
    run_test<Case5>();
    using Case6 = int**;
    run_test<Case6>();

    // 数组
    using Case7 = int[10];
    run_test<Case7>();
    using Case8 = const char[];
    run_test<Case8>();
    using Case9 = decltype(arr_ret<int, 5>());
    run_test<Case9>();

    // 函数和 std::function
    using Case10 = void(int, double);
    run_test<Case10>();
    using Case11 = int (*)(char);
    run_test<Case11>();
    using Case12 = std::function<int(const std::vector<int>&, std::tuple<int, int, int>)>;
    run_test<Case12>();

    // STL 容器
    using Case13 = std::vector<std::string>;
    run_test<Case13>();
    using Case14 = std::map<int, std::vector<MyClass>>;
    run_test<Case14>();

    // 用户定义类型
    using Case15 = MyClass;
    run_test<Case15>();
    using Case16 = MyEnum;
    run_test<Case16>();
    using Case17 = TestClass<float, 20>;
    run_test<Case17>();

    // 复杂组合类型
    using Case18 = const std::vector<MyClass*>&;
    run_test<Case18>();
    using Case19 = int (MyClass::*)(int);
    run_test<Case19>();

    // 复杂的C风格函数指针类型
    using Case20 = int (*(*(*)(int*))[4])(int*);
    run_test<Case20>();

    // 枚举类型
    using Case21 = Color;
    run_test<Case21>();

    // 聚合类对象
    using Case22 = Person;
    run_test<Case22>();

    // NTTP的解析
    using Case23 = Array<5>;
    run_test<Case23>();

    using namespace test_namespace;
    run_test<inner_namespace::myDerived>();



    // const 成员函数
    using Case24 = decltype(&MemberFuncTest::f_const);
    run_test<Case24>();

    // 左值引用限定成员函数
    using Case25 = decltype(&MemberFuncTest::f_ref);
    run_test<Case25>();

    // 右值引用限定且 noexcept 的成员函数
    using Case26 = decltype(&MemberFuncTest::f_rref_noexcept);
    run_test<Case26>();

    // const volatile 且左值引用限定的成员函数
    using Case27 = decltype(&MemberFuncTest::f_cv_ref);
    run_test<Case27>();


    // Case 28: 简单无捕获 Lambda (隐式 const)
    auto p_simple = [](int, const char*) -> void {};
    using Case28 = decltype(p_simple);
    run_test<Case28>();

    // Case 29: 带捕获的 Lambda
    int x = 10;
    std::string y = "hello";
    auto p_capture = [x, &y](bool)
    { return y + std::to_string(x); };
    using Case29 = decltype(p_capture);
    run_test<Case29>();

    // Case 30: mutable 且 noexcept 的 Lambda
    auto p_mutable_noexcept = [x]() mutable noexcept
    { x++; };
    using Case30 = decltype(p_mutable_noexcept);
    run_test<Case30>();

    // Case 31: 泛型 Lambda (C++14)
    auto p_generic = [](auto a, auto b)
    { return a + b; };
    using Case31 = decltype(p_generic);
    run_test<Case31>();


    return 0;
}