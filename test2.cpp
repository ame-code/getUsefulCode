
template <class T> struct ParamTypeImpl {using type = T;};
template <class T> requires (sizeof(T) <= 16)
struct ParamTypeImpl<T> {using type = const T&;};
template <class T> using ParamType = typename ParamTypeImpl<T>::type;

using i32 = int;

template <int N>
struct Value {
    inline static constexpr int value = N;
};

template <class T,  int N>
ParamType<T> a = Value<N>::value;

template <i32 N>
struct Foo {
    void func(i32& n) {
        n = N;
    }
};

int main() {
    Foo<5> foo;
    ParamType<i32> param = a<i32, 10>;
    int n = 10;
    i32& a = n;
    foo.func(n);
}