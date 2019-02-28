
#ifndef VLANTEXT_H
#define VLANTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        char *file;
        char *stream;
        char *title;
        char *url;		
        char *res;
        int loop;
        int debug;
        int audio;
    } param_t;

    int
    vlt_start(param_t *param);

#ifdef __cplusplus
}
#endif

#endif /* VLANTEXT_H */

