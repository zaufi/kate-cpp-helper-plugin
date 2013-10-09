namespace test {
struct foo
{
    int a_member;
    static int b_member;
};
int foo::b_member = 123;
int* b_pointer = &foo::b_member;
}
