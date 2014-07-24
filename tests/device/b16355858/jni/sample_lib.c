#include <stdlib.h>

static /*__attribute__ ((noinline))*/ int Sub3(int a, int b, int c) {
  const int pb = b - c;
  const int pa = a - c;
  return abs(pb) - abs(pa);
}

static unsigned Select(unsigned a, unsigned b, unsigned c) {
  const int pa_minus_pb =
      Sub3((a >> 24) & 0xff, (b >> 24) & 0xff, (c >> 24) & 0xff) +
      Sub3((a >> 16) & 0xff, (b >> 16) & 0xff, (c >> 16) & 0xff) +
      Sub3((a >>  8) & 0xff, (b >>  8) & 0xff, (c >>  8) & 0xff) +
      Sub3((a >>  0) & 0xff, (b >>  0) & 0xff, (c >>  0) & 0xff);
  return (pa_minus_pb <= 0) ? a : b;
}

static unsigned Predictor11(unsigned left, const unsigned* const top) {
  const unsigned pred = Select(top[0], left, top[-1]);
  return pred;
}

typedef unsigned (*VP8LPredictorFunc)(unsigned left, const unsigned* const top);

const VP8LPredictorFunc kPredictorsC[] = {
  Predictor11,
};
