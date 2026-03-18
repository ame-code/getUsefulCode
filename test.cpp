template <class T = double, T Eps = 1e-7>
struct FloatWithEps
{
    FloatWithEps(double) {}
};

using Float = FloatWithEps<double, 1e-7>;

int main() {
    Float(0);
}