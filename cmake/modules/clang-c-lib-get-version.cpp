// kate: hl C++11;
#include <iostream>
#include <clang-c/Index.h>

int main()
{
    CXString version_str = clang_getClangVersion();
    std::cout << clang_getCString(version_str);
    clang_disposeString(version_str);
    return 0;
}
