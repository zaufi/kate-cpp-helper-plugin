struct foo
{
    template <typename T>
    foo(const int a)
      : m_a(a+2)
    {
        int b = a + 2;
    }
    int m_a;
};
