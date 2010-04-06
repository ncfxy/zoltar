#include <stdio.h>
#include <stdlib.h>

int RationalGT(int num1, int den1, int num2, int den2) {
  return ((float)num1/(float)den1) > ((float)num2/(float)den2);
}

void RationalSort(int n, int *num, int *den) {
  int i, j, temp;
  
  for (i=n-1; i>=0; i--) {
    for (j=0; j<i; j++) {
      if(RationalGT(num[j], den[j], num[j+1], den[j+1])) {
        /* bug: forgot to swap denominators */
        temp = num[j];
        num[j] = num[j+1];
        num[j+1] = temp;
      }
    }
  }
}
