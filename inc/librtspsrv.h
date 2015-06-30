/*********************************************************************
 * File Name    : librtspsrv.h
 * Description  : 
 * Author       : Hu Lizhen
 * Create Date  : 2012-11-15
 ********************************************************************/

#ifndef __LIBRTSPSRV_H__
#define __LIBRTSPSRV_H__


#if defined (__cplusplus) || defined (_cplusplus)
extern "C" {
#endif

#define DFL_RTSP_PORT   10554   /* Default RTSP port. */
#define DFL_HTTP_PORT   0       /* Not support RTSP over HTTP yet. */

#define MAX_CHN_NUM 8                 /* Max channel number. */
#define MAX_FRM_SZ  (1 * 1024 * 1024) /* Max frame size bytes. */

/* Frame type. */
enum frm_type {
    FRM_TYPE_I = 1,
    FRM_TYPE_B,
    FRM_TYPE_P,
    FRM_TYPE_A,
};

enum chn_type {
    CHN_TYPE_MAIN,
    CHN_TYPE_MINOR,
};

/* Stream resource information. */
struct strm_info {
    int chn_no;                 /* Channel number. */
    enum chn_type chn_type;
    char *file_name;            /* Local AV file name. */
    void *usr_data;
};

/* Frame information used for getting frame. */
struct frm_info {
    unsigned int frm_sz;        /* Frame size. */
    enum frm_type frm_type;     /* Frame type. */
    char *frm_buf;              /* Frame buffer: store pure av data. */
};

/*
 * Callback function to start, stop and get media resource.
 */
typedef int (*start_strm_t)(struct strm_info *);
typedef void (*stop_strm_t)(struct strm_info *);
typedef int (*get_frm_t)(struct strm_info *, struct frm_info *);

void set_strm_cb(start_strm_t start, stop_strm_t stop, get_frm_t get);
void get_strm_cb(start_strm_t *start, stop_strm_t *stop, get_frm_t *get);
int rtsp_init_lib(void);
int rtsp_deinit_lib(void);

#if defined (__cplusplus) || defined (_cplusplus)
}
#endif

#endif /* __LIBRTSPSRV_H__ */

