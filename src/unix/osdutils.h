#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define strcmpi        strcasecmp
#define strncmpi       strncasecmp

#define osd_mkdir(dir) mkdir(dir, 0)

#define PATH_SEPARATOR '/'
#define EOLN           "\n"
