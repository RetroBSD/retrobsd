BEGIN	{ 
	FS="|" ; 
	sum=0.00; 
	n=0;
	printf("\n                     INDEX VALUES            \n");
	printf("TEST%40sBASELINE     RESULT      INDEX\n\n","");

	}

	{ # process all lines
	sum += log($10/$5)
	++n;
	printf("%-40s  %10.1f %10.1f %10.1f\n", $1, $5, $10, 10*$10/$5);
	}

END	{
	printf("              %30s                     =========\n", "");
	printf("     FINAL SCORE      %30s  %20.1f\n","",(10*exp(sum/n)));
	}
