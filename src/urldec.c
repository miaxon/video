#include <ctype.h>

#include "urldec.h"
#include "log.h"
#include "err.h"

static long long
url_strtonum(const char *str, int base, long long min, long long max, int *err) {
	long long	l;
	char		*ep;

	if (min > max) {
		*err = -1;
		return (0);
	}

	errno = 0;
	l = strtoll(str, &ep, base);
	if (errno != 0 || str == ep || *ep != '\0') {
		*err = -1;
		return (0);
	}

	if (l < min) {
		*err = -1;
		return (0);
	}

	if (l > max) {
		*err = -1;
		return (0);
	}

	*err = 0;
	return (l);
}

int
url_decode(char *arg) {
	u_int8_t v;
	int		err;
	size_t	len;
	char	*p, *in, h[5];

	p = arg;
	in = arg;
	len = strlen(arg);

	while (*p != '\0' && p < (arg + len)) {
		if (*p == '+')
			*p = ' ';
		if (*p != '%') {
			*in++ = *p++;
			continue;
		}

		if ((p + 2) >= (arg + len)) {
			ERROR("overflow in '%s'", arg);
			return -1;
		}

		if (!isxdigit(*(p + 1)) || !isxdigit(*(p + 2))) {
			*in++ = *p++;
			continue;
		}

		h[0] = '0';
		h[1] = 'x';
		h[2] = *(p + 1);
		h[3] = *(p + 2);
		h[4] = '\0';
		
		v = url_strtonum(h, 16, 0x0, 0xff, &err);
		if (err != 0)
			return -1;
		
		*in++ = (char) v;
		p += 3;
	}

	*in = '\0';
	return 0;
}
