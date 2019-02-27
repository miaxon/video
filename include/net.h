#ifndef NET_H
#define NET_H

#ifdef __cplusplus
extern "C" {
#endif
#include "vlt.h"

	int net_init(char *title, int port, char *url);
	
	char*
	net_get_title(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_H */

