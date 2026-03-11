#include <format>

template <class T>
struct Foo {
    int n;
};

template <class T>
struct std::formatter<Foo<T>> : std::formatter<T> {
    auto format(Foo<T> foo, std::format_context& ctx) const {
        return std::formatter<T>::format(foo.n, ctx);
    }
};

int main() {
    auto a = std::format("{}", Foo<int>());
}