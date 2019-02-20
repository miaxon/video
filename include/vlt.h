
#ifndef VLANTEXT_H
#define VLANTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        const char *file;
        const char *stream;
		const char *title;
		const char *url;
		int        port;
		int        loop;
		
		
    } param_t;

    int
    vlt_start(param_t *param);
    
#ifdef __cplusplus
}
#endif

#endif /* VLANTEXT_H */

