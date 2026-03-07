template <class T>
struct Foo {};

using i32 = int;

struct Bar {};

int main() {
    Foo<i32> foo;
}