#include <format>

struct Foo {
    int n;
};

template <>
struct std::formatter<Foo> : std::formatter<int> {
    auto format(Foo foo, std::format_context& ctx) const {
        return std::formatter<int>::format(foo.n, ctx);
    }
};

int main() {
    auto a = std::format("{}", Foo(1));
}