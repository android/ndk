#include <chrono>
#include <cstdio>

#include "hanoi.h"

using namespace std;
using namespace std::chrono;

void hanoi_callback(int from, int to) {
}

int main()
{
  high_resolution_clock::time_point start = high_resolution_clock::now();
  hanoi(0, 2, 1, 25, &hanoi_callback);
  high_resolution_clock::time_point end = high_resolution_clock::now();

  printf("Duration: %lld ns\n",
    static_cast<long long>(duration_cast<nanoseconds>(end - start).count()));

  return 0;
}
