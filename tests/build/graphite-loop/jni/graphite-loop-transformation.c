#define  N 10000
int A[N][N];
int main()
{
   int i, j, sum = 0;
   for(i=0; i<N; i++)
     for(j=0; j<N; j++)
       sum += A[j][i];
   return sum;
}
