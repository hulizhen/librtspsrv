/*********************************************************************
 * File Name    : librtspsrv.c
 * Description  : 
 * Author       : Hu Lizhen
 * Create Date  : 2012-11-15
 ********************************************************************/

#include <stdio.h>
#include "librtspsrv.h"
#include "rtsp_srv.h"
#include "util.h"
#include "log.h"


/*
 * NOTE:
 * If you've made big changes or added new features,
 *      then bump the MAJOR number up.
 * Else if you've fix big bugs or add features based on existed part,
 *      then bump the MINOR number up.
 * Else if you've changed little things or just fixed some small bugs,
 *      then bump the BUILD number up is enough.
*/
#define LIBRARY_VERSION_MAJOR 0
#define LIBRARY_VERSION_MINOR 1
#define LIBRARY_VERSION_BUILD 0


/* RTSP stream relative callback functions. */
static start_strm_t start_strm;
static stop_strm_t stop_strm;
static get_frm_t get_frm;


static void print_lib_info(void)
{
    printf("\n    \033[1;32m****************************************************\033[0m\n"
           "    \033[1;32m*   RTSP library version: [V%d.%d.%d]                 *\033[0m\n"
           "    \033[1;32m*   Build date and time:  [%s, %s]  *\033[0m\n"
           "    \033[1;32m****************************************************\033[0m\n\n\n",
           LIBRARY_VERSION_MAJOR, LIBRARY_VERSION_MINOR, LIBRARY_VERSION_BUILD,
           __DATE__, __TIME__);
    return;
}

void set_strm_cb(start_strm_t start, stop_strm_t stop, get_frm_t get)
{
    start_strm = start;
    stop_strm = stop;
    get_frm = get;
    return;
}

void get_strm_cb(start_strm_t *start, stop_strm_t *stop, get_frm_t *get)
{
    if (start) {
        *start = start_strm;
    }
    if (stop) {
        *stop = stop_strm;
    }
    if (get) {
        *get = get_frm;
    }
    return;
}

int rtsp_init_lib(void)
{
    print_lib_info();

    if (!start_strm || !stop_strm || !get_frm) {
        printf("\033[31m    *** RTSP stream callback MUST be set properly! ***\033[0m\n");
        return -1;
    }
    start_rtsp_srv();
    return 0;
}

int rtsp_deinit_lib(void)
{
    return 0;
}
