/*
 * Null cleanup routine to resolve reference in exit()
 * if not using stdio.
 */
void __attribute__((weak)) _cleanup()
{
}
