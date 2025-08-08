#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include "parser.hpp"  // 引入之前定义的类型树
#include <vector>
#include <functional>

template <typename T, int N>
auto arr_ret()
{
    return std::array<T, N>{};
}

template <typename T, int N>
class testclass
{
public:
    using type = T;
    static constexpr int size = N;
    T arr[N];

    static void print()
    {
        std::cout << "Type: " << typeid(T).name() << ", Size: " << N << std::endl;
    }
};

// 包含上面我们写的代码和 raw_name_of 的定义
template <typename T, int N>
using arr = T[N];
int main()
{
    // 在 main.cpp 中测试
    struct MyClass
    {
        int member;
    };
    enum MyEnum
    {
        A = 20,
        B
    };

    using T = std::function<int(const std::vector<int>&, std::tuple<int, int, int>)>;
    type_vision::static_parse::Parser<T>::type::print();

    static auto* arr_ret_ptr = arr_ret<int, 5>;
    using T2 = decltype(arr_ret_ptr);
    type_vision::static_parse::Parser<T2>::type::print();

    /*   using T3 = testclass<int, 10>;
    type_vision::static_parse::Parser<T3>::type::print(); */

    return 0;
}