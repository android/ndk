#include <stdio.h>

typedef unsigned (*VP8LPredictorFunc)(unsigned left, const unsigned* const top);
extern const VP8LPredictorFunc kPredictorsC[];

int main(void) {
  const unsigned top[2] = {0xff7a7a7a, 0xff7a7a7a};
  const unsigned left = 0xff7b7b7b;
  const unsigned pred = kPredictorsC[0](left, top + 1);
  fprintf(stderr, "top[-1]: %8x top[0]: %8x left: %8x pred: %8x\n",
          top[0], top[1], left, pred);
  if (pred == left)
    return 0;
  fprintf(stderr, "pred != left\n");
  return 1;
}
