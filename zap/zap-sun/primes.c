main()
{
  int i,j,bad;

  for (i = 500; i < 1000; i++) {
    for (j = 2,bad = 0; j < i / 2; j++)
      if (i % j == 0) {
	bad = 1;
	break;
      }
    if (bad == 0)
      printf("%d\n",i);
  }
}
