#include <stdio.h>
#include "mythread.h"

int n;

void t1(void * who)
{
  int i;

  printf("t%d start\n", (long)who);
  for (i = 0; i < n; i++) {
    printf("t%d yield\n", (long)who);
    MyThreadYield();
  }
  printf("t%d end\n", (long)who);
  MyThreadExit();
}

void t0(void * dummy)
{
  MyThreadCreate(t1, (void *)1);
  t1(0);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
    return -1;
  n = atoi(argv[1]);
  MyThreadInit(t0, 0);
}
