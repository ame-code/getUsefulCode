#define MAIN int main() {}

namespace aaa {
    int bbb = 1;
    namespace ccc {
        int ddd = 2;
    };
};

int main() {
    aaa::bbb = 2;
    // aaa::ccc::ddd = 3;
}