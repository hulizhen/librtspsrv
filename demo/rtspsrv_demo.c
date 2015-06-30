/*********************************************************************
 * File Name    : rtspDemo.c
 * Description  : Demostrate how to use this library.
 * Author       : Hu Lizhen
 * Create Date  : 2012-11-13
 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "librtspsrv.h"

#define FILE_HDR_SZ 512

/* AV file frame flags. */
#define FRM_FLG_I       0x63643030 /* I frame <Ascii: 00dc>. */
#define FRM_FLG_P       0x63643130 /* P frame <Ascii: 01dc>. */
#define FRM_FLG_A       0x62773130 /* A frame <Ascii: 01wb>. */
#define FRM_FLG_INFO    0x30317762 /* Frmae info. */

/* Frame header */
struct frm_hdr {
	unsigned int    frm_flg;    /* Frame type<Ascii - type>: <00dc - I>, <01dc - P>, <01wb - A>. */
	unsigned int    frm_sz;   /* Size of frame. */
	unsigned char   hour;
	unsigned char   min;
	unsigned char   sec;
	unsigned char   ext;
	unsigned int    i_last_offset;
	long long       pts;        /* Timestamp. */
	unsigned int    alarm_info;
	unsigned int    pad;
};

int start_strm(struct strm_info *strmp)
{
    if (strmp->file_name) {
        FILE *fp = fopen(strmp->file_name, "rb");
        if (!fp) {
            printf("fopen local file[%s] error: %s\n",
                   strmp->file_name, strerror(errno));
            return -1;
        }
        strmp->usr_data = fp;

        /* Skip over the file header. */
        if (fseek(fp, FILE_HDR_SZ, SEEK_SET) < 0) {
            perror("fseek av file error");
            return -1;
        }
    }
    return 0;
}

void stop_strm(struct strm_info *strmp)
{
    if (strmp->file_name) {
        FILE *fp = (FILE *)strmp->usr_data;
        if (strmp->usr_data) {
            fclose(fp);
            strmp->usr_data = 0;
        }
    }
    return;
}

int get_frm(struct strm_info *strmp, struct frm_info *frmp)
{
    FILE *fp = (FILE *)strmp->usr_data;
    size_t nr = 0;              /* Bytes read. */
    struct frm_hdr frm_hdr;

retry:
    /* Read frame head. */
    nr = fread(&frm_hdr, 1, sizeof(struct frm_hdr), fp);
    if (nr < 0) {
        perror("fread frame header error");
        return -1;
    } else if (nr == 0) {
        fseek(fp, FILE_HDR_SZ, SEEK_SET);
        goto retry;
    } else if (nr != sizeof(struct frm_hdr)) {
        printf("Bytes read[%d] != sizeof(struct frm_hdr) T_T\n", (int)nr);
        return -1;
    }

    if (frm_hdr.frm_sz >= MAX_FRM_SZ) {
        printf("The size of frame is too large: %x!\n", frm_hdr.frm_sz);
        return -1;
    }

    /* Read frame data. */
    nr = fread(frmp->frm_buf, 1, frm_hdr.frm_sz, fp);
    if (nr < 0) {
        perror("fread frame data error");
        return -1;
    } else if (nr == 0) {
        fseek(fp, FILE_HDR_SZ, SEEK_SET);
        goto retry;
    } else if (nr != frm_hdr.frm_sz) {
        printf("Bytes read[%d] != frm_hdr.frm_sz T_T\n", (int)nr);
        return -1;
    }

    frmp->frm_sz = frm_hdr.frm_sz;
    switch (frm_hdr.frm_flg) {
    case FRM_FLG_I:
        frmp->frm_type = FRM_TYPE_I;
        break;
    case FRM_FLG_P:
        frmp->frm_type = FRM_TYPE_P;
        break;
    case FRM_FLG_A:
        frmp->frm_type = FRM_TYPE_A;
        break;
    default:
        printf("Undefined frame type, flag = %d\n", frm_hdr.frm_flg);
        return -1;
    }
    
    return 0;
}

static void signal_handler(int signo)
{
    switch (signo) {
    case SIGINT:
        exit(0);
        break;
    case SIGPIPE:
        break;
    default:
        break;
    }
    return;
}

static void init_signals(void)
{
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, signal_handler);
    return;
}

int main(int argc, char *argv[])
{
    init_signals();

    set_strm_cb(start_strm, stop_strm, get_frm);
    rtsp_init_lib();

    while (1) {
        sleep(3);
    }
    return 0;
}
