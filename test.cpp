template <class T>
struct Foo {};

using i32 = int;

struct Bar {};

#define MAIN
#undef MAIN

int main() {
    Foo<i32> foo;
}