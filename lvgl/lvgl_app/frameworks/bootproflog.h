#include <math.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

static void writeBootProf(const char* text)
{
    int fd = open("/proc/bootprof", O_CREAT | O_RDWR | O_APPEND);
    if (fd < 0) {
        return;
    }
    write(fd, text, strlen(text));
    close(fd);
}
