namespace test {
struct
{
    int a_member;
} some;
namespace {
union
{
    long as_long;
    char buf[sizeof(long)];
};
}
}
