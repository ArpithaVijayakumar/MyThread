#include <stdio.h>
#include "mythread.h"

void t0(void * dummy)
{
  printf("t0 start\n");
  //MyThreadExit();
}

int main()
{
  printf("from main");
  MyThreadInit(t0, NULL);
  printf("Hello from main");
}
