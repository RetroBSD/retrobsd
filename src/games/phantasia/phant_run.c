/*
	This is a program to run phantasia

	David Wells, May, 1986
*/

main(argc, argv)
	int argc;
	char **argv;
{
	char tmp[160];
	strcat(tmp,"exec nice /usr/games/lib/phantasia/phantasia ");
	if (argc > 1)
		strcat(tmp,argv[1]);
	system(tmp);
}
