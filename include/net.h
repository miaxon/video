#ifndef NET_H
#define NET_H

#ifdef __cplusplus
extern "C" {
#endif
#include "vlt.h"

	int net_init(char *title, char *url, char *resource);

	char*
	net_subtitle(void);
	void
	net_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_H */

