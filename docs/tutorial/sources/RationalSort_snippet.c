void RationalSort(int n, int *num, int *den) {
  /* block 1 */
  int i, j, temp;
  
  for (i=n-1; i>=0; i--) {
    /* block 2 */
    for (j=0; j<i; j++) {
      /* block 3 */
      if(RationalGT(num[j], den[j], num[j+1], den[j+1])) {
        /* block 4 */
        temp = num[j];
        num[j] = num[j+1];
        num[j+1] = temp;
      }
    }
  }
}
