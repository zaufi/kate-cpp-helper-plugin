class foo
{
    int a : 1;
    int b : 4;
    int c;
    bool operator ==(const foo);
};

bool operator !=(const foo, const foo);

bool operator !=(const foo, const foo)
{
    auto result = false;
    return result;
}
