main()
{
    int t;

    t = 0;
    if (t) printt(t, "failure"); else printt(t, "success");
    t = 1;
    if (t) printt(t, "success"); else printt(t, "failure");
    t = 8;
    if (t) printt(t, "success"); else printt(t, "failure");
    t = -2;
    if (t) printt(t, "success"); else printt(t, "failure");
    printf("\n");

    t = 4;
    printf("switch test: ");
    switch (t) {
    case 3:
        printf("failure");
        break;
    case 4:
        printf("success");
        break;
    case 5:
        printf("failure");
        break;
    }
    printf("\n");

    printf("switch fallthrough test: ");
    switch (t) {
    case 3:
        printf("failure");
        break;
    case 4:
        printf("OKSOFAR: ");
    case 5:
        printf("success if oksofar printed before this in caps");
        break;
    }
    printf("\n");
}

printt (t, str)
    int t;
    char *str;
{
    printf("bool test on value %d %s\n", t, str);
}
