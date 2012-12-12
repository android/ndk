#include "hanoi.h"

void hanoi(int from, int to, int mid, int n, void (*callback)(int, int)) {
  if (n == 1) {
    callback(from, to);
  } else {
    hanoi(from, mid, to, n - 1, callback);
    callback(from, to);
    hanoi(mid, to, from, n - 1, callback);
  }
}
