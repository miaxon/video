#include <unistd.h>
#include <errno.h>

#include "log.h"
#include "err.h"
#include "utils.h"

int
utils_file_exists(const char *path)
{
	if (access(path, R_OK) == -1) {
		switch (errno) {
			case ENOENT:
				ERROR("'%s': file does not exists.", path);
				return ENOENT;
			case EACCES:
				ERROR("'%s': access denied.", path);
				return EACCES;
			default:
				ERR_SYS("%s", "access() failed");
		}
	} else {
		return 0;
	}
}