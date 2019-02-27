#include <net.h>
static char *current_title;

int
net_init(char *title, int port, char *url) {
	current_title = title;
}

char*
net_get_title(void) {
	return current_title;
}

