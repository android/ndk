#include <cstdio>

int main()
{
    std::FILE* fp;
    int a = std::getchar();
    int b = std::putchar(a);
    std::clearerr(fp);
    int c = std::feof(fp);
    int d = std::ferror(fp);

    return 0;
}
