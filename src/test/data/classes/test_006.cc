struct foo
{
    void bar() {};
    static int b;
    static constexpr int c = 123;
};

int foo::b = 0;
constexpr int foo::c;