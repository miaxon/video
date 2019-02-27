
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
        int port;
        int loop;
        int debug;
        int audio;
        int net;
    } param_t;

    int
    vlt_start(param_t *param);

#ifdef __cplusplus
}
#endif

#endif /* VLANTEXT_H */

