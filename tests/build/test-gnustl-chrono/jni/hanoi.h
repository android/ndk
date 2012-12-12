#ifndef HANOI_H
#define HANOI_H

#if defined(__cplusplus)
extern "C" {
#endif

extern void hanoi(int from, int to, int mid, int n,
                  void (*callback)(int, int));

#if defined(__cplusplus)
}
#endif

#endif /* HANOI_H */
