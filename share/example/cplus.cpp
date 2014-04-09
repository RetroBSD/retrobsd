extern "C" {
    #include <string.h>
    #include <unistd.h>
};

int main()
{
    const char *message = "Hello, C++ World!\n";
    write (1, message, strlen (message));
    return 0;
}
