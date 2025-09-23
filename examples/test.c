#include <stdio.h>

int main(void) {
   int i;
   int x;
   i = 4;
   x = 3;
   while (i < 7) {
      x = x + i;
      i = i + 2;
   }
   return 0;
}
