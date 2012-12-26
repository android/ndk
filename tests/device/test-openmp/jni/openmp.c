#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char *argv[])
{
    int iam = 0, np = 1;

    if (!getenv("OMP_NUM_THREADS"))
        omp_set_num_threads(4);

  #pragma omp parallel default(shared) private(iam, np)
    {
      #if defined(_OPENMP)
        np = omp_get_num_threads();
        iam = omp_get_thread_num();
      #endif
        printf("Hello from thread %d out of %d\n", iam, np);
    }

    return 0;
}
