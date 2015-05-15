int ga[5];

main()
{
    int a, b, c, d;
    int arr[5];
    int *pi;
    char arrc[5];
    char *pic;
    int s1, s2;
    int z;
    int t;
    int *pip;
    int *picp;
    int e1, e2;

    ga[0] = 10;
    ga[1] = 20;
    ga[2] = 30;
    ga[3] = 40;
    ga[4] = 50;
    
    a = 21;
    b = 31;
    c = 71;
    d = 82;

    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    arr[4] = 50;
    pi = &arr[0];

    arrc[0] = 13;
    arrc[1] = 23;
    arrc[2] = 33;
    arrc[3] = 43;
    arrc[4] = 53;
    pic = &arrc[0];

    printf("          21 + 31 = %d (52)\n",   a + b);
    printf("          21 - 31 = %d (-10)\n",  a - b);
    printf("          21 & 71 = %d (5)\n",    a & c);
    printf("          21 | 82 = %d (87)\n",   a | d);
    printf("          21 ^ 82 = %d (71)\n",   a ^ d);
    printf("          21 * 82 = %d (1722)\n", a * d);
    printf("          82 % 21 = %d (19)\n",   d % a);
    printf("          82 / 21 = %d (3)\n",    d / a);
    printf("              *pi = %d (10)\n",   *pi);
    printf("          *pi + 1 = %d (11)\n",   *pi + 1);
    printf("        *(pi + 1) = %d (20)\n",   *(pi + 1));
    printf("&arr[3] - &arr[0] = %d (3)\n",    &arr[3] - &arr[0]);
    printf("    arr[3]-arr[0] = %d (30)\n",   arr[3] - arr[0]);
    printf("    arr[3]+arr[0] = %d (50)\n",   arr[3] + arr[0]);
    printf("  &ga[3] - &ga[0] = %d (3)\n",    &ga[3] - &ga[0]);
    printf("      ga[3]-ga[0] = %d (30)\n",   ga[3] - ga[0]);
    printf("      ga[3]+ga[0] = %d (50)\n",   ga[3] + ga[0]);
    printf("\n");

    printf("               *pic = %d (13)\n", *pic);
    printf("           *pic + 1 = %d (14)\n", *pic+1);
    printf("         *(pic + 1) = %d (23)\n", *(pic+1));
    printf("&arrc[3] - &arrc[0] = %d (3)\n",  &arrc[3]-&arrc[0]);
    printf("\n");

    s1 = 3;
    s2 = -200;
    printf("  82 << 3 = %d (656)\n",   d << s1);
    printf("  82 >> 3 = %d (10)\n",    d >> s1);
    printf("-200 >> 3 = %d (-25)\n",   s2 >> s1);
    printf("-200 << 3 = %d (-1600)\n", s2 << s1);
    printf("\n");

    printf("-s1 = %d (-3)\n",  -s1);
    printf("-s2 = %d (200)\n", -s2);
    printf("\n");

    printf("~82 = %d (-83)\n", ~d);
    printf("\n");

    z = 0;
    printf("!82 = %d (0)\n", !d);
    printf(" !0 = %d (1)\n", !z);
    printf("\n");

    printf(" 0 && 0  = %d (0)\n", z && z);
    printf(" 0 && 21 = %d (0)\n", z && a);
    printf(" 3 && 21 = %d (1)\n", s1 && a);
    printf("21 && 3  = %d (1)\n", a && s1);
    printf("\n");

    printf(" 0 || 0  = %d (0)\n", z || z);
    printf(" 0 || 21 = %d (1)\n", z || a);
    printf(" 3 || 21 = %d (1)\n", s1 || a);
    printf("21 || 3  = %d (1)\n", a || s1);
    printf("\n");

    pi = 4;
    printf("pi++ = %d (4)\n",  pi++);
    printf("  pi = %d (8)\n",  pi);
    printf("++pi = %d (12)\n", ++pi);
    printf("pi-- = %d (12)\n", pi--);
    printf("  pi = %d (8)\n",  pi);
    printf("--pi = %d (4)\n",  --pi);
    printf("\n");

    pic = 4;
    printf("pic++ = %d (4)\n", pic++);
    printf("  pic = %d (5)\n", pic);
    printf("++pic = %d (6)\n", ++pic);
    printf("pic-- = %d (6)\n", pic--);
    printf("  pic = %d (5)\n", pic);
    printf("--pic = %d (4)\n", --pic);
    printf("\n");

    t = 4;
    printf("t++ = %d (4)\n", t++);
    printf("  t = %d (5)\n", t);
    printf("++t = %d (6)\n", ++t);
    printf("t-- = %d (6)\n", t--);
    printf("  t = %d (5)\n", t);
    printf("--t = %d (4)\n", --t);
    printf("\n");

    t = 4;
    printf(" t==4 = %d (1)\n", t == 4);
    printf(" t==3 = %d (0)\n", t == 3);
    printf(" t==5 = %d (0)\n", t == 5);
    t = -4;
    printf("t==-4 = %d (1)\n", t == -4);
    printf("t==-3 = %d (0)\n", t == -3);
    printf("t==-5 = %d (0)\n", t == -5);
    printf(" t==4 = %d (0)\n", t == 4);
    printf(" t==3 = %d (0)\n", t == 3);
    printf(" t==5 = %d (0)\n", t == 5);
    printf("\n");

    t = 4;
    printf(" t!=4 = %d (0)\n", t != 4);
    printf(" t!=3 = %d (1)\n", t != 3);
    printf(" t!=5 = %d (1)\n", t != 5);
    t = -4;
    printf("t!=-4 = %d (0)\n", t != -4);
    printf("t!=-3 = %d (1)\n", t != -3);
    printf("t!=-5 = %d (1)\n", t != -5);
    printf(" t!=4 = %d (1)\n", t != 4);
    printf(" t!=3 = %d (1)\n", t != 3);
    printf(" t!=5 = %d (1)\n", t != 5);
    printf("\n");

    t = 4;
    printf(" t<4 = %d (0)\n", t < 4);
    printf(" t<3 = %d (0)\n", t < 3);
    printf(" t<5 = %d (1)\n", t < 5);
    printf("t<-1 = %d (0)\n", t < -1);
    printf("\n");

    printf(" t<=4 = %d (1)\n", t <= 4);
    printf(" t<=3 = %d (0)\n", t <= 3);
    printf(" t<=5 = %d (1)\n", t <= 5);
    printf("t<=-1 = %d (0)\n", t <= -1);
    printf("\n");

    t = 4;
    printf(" t>4 = %d (0)\n", t > 4);
    printf(" t>3 = %d (1)\n", t > 3);
    printf(" t>5 = %d (0)\n", t > 5);
    printf("t>-1 = %d (1)\n", t > -1);
    printf("\n");

    printf(" t>=4 = %d (1)\n", t >= 4);
    printf(" t>=3 = %d (1)\n", t >= 3);
    printf(" t>=5 = %d (0)\n", t >= 5);
    printf("t>=-1 = %d (1)\n", t >= -1);
    printf("\n");

    pi = -100;
    printf("   pi<4 = %d (0)\n", pi < 4);
    printf("   pi<3 = %d (0)\n", pi < 3);
    printf("pi<-100 = %d (0)\n", pi < -100);
    printf("  pi<-1 = %d (1)\n", pi < -1);
    printf("\n");

    printf("   pi<=4 = %d (0)\n", pi <= 4);
    printf("   pi<=3 = %d (0)\n", pi <= 3);
    printf("pi<=-100 = %d (1)\n", pi <= -100);
    printf("  pi<=-1 = %d (1)\n", pi <= -1);
    printf("\n");

    pi = -100;
    printf("   pi>4 = %d (1)\n", pi > 4);
    printf("   pi>3 = %d (1)\n", pi > 3);
    printf("pi>-100 = %d (0)\n", pi > -100);
    printf("  pi>-1 = %d (0)\n", pi > -1);
    printf("\n");

    printf("   pi>=4 = %d (1)\n", pi >= 4);
    printf("   pi>=3 = %d (1)\n", pi >= 3);
    printf("pi>=-100 = %d (1)\n", pi >= -100);
    printf("  pi>=-1 = %d (0)\n", pi >= -1);
    printf("\n");

    pi = &arr[0];
    pip = &arr[3];
    printf("    *pip - *pi: %d 30\n", *pip - *pi);
    printf("      pip - pi: %d 3\n",  pip - pi);
    printf("          *pip: %d 40\n", *pip);
    printf("    *(pip - 3): %d 10\n", *(pip - 3));
    printf("      *&arr[3]: %d 40\n", *&arr[3]);
    printf("*(&arr[3] - 3): %d 10\n", *(&arr[3]-3));
}

printt (t, str)
    int t;
    char *str;
{
    printf("bool test on value %d %s\n", t, str);
}
