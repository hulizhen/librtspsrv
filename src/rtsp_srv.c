/*********************************************************************
 * File Name    : rtsp_srv.c
 * Description  : Implementaion of a RTSP server.
 * Author       : Hu Lizhen
 * Create Date  : 2012-11-16
 ********************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include "list.h"
#include "log.h"
#include "librtspsrv.h"
#include "rtsp_srv.h"
#include "util.h"
#include "rtsp_parser.h"
#include "rtsp_sess.h"
#include "rtp.h"
#include "sd_handler.h"
#include "delay_task.h"


struct rtsp_srv rtsp_srv;

/**
 * This will add specified @ev to the epoll events.
 * 
 * @ev: Generally, we provide EPOLLIN, EPOLLOUT, EPOLLET for the caller.
 */
int monitor_sd_event(int fd, unsigned int ev)
{
    struct epoll_event ee;

    ee.events = ev;
    ee.data.fd = fd;
    if (epoll_ctl(rtsp_srv.epoll_fd, EPOLL_CTL_ADD, fd, &ee) < 0) {
        perrord(ERR "Add fd to epoll events error");
        return -1;
    }
    return 0;
}

/**
 * This will modify specified @ev to the epoll events.
 * 
 * @ev: Generally, we provide EPOLLIN, EPOLLOUT, EPOLLET for the caller.
 */
int update_sd_event(int fd, unsigned int ev)
{
    struct epoll_event ee;

    ee.events = ev;
    ee.data.fd = fd;
    if (epoll_ctl(rtsp_srv.epoll_fd, EPOLL_CTL_MOD, fd, &ee) < 0) {
        perrord(ERR "Add fd to epoll events error");
        return -1;
    }
    return 0;
}

static void *rtsp_srv_thrd(void *arg)
{
    pthread_detach(pthread_self());
    entering_thread();
    int i = 0;
    struct rtsp_srv *srvp = (struct rtsp_srv *)arg;
    int rtsp_lsn_sd = -1;           /* RTSP listen socket. */
    int epfd = -1;
    struct epoll_event *ees = NULL;
    int nfds = 0;
    int timeout = 0;
    struct rlimit rl;
    int maxfd = MAX_FD_NUM;
    int min_delay_time = 0;

    /* Set resource limit. */
    rl.rlim_cur = maxfd;
    rl.rlim_max = maxfd;
    if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
        if (errno == EPERM || errno == EINVAL) {
            if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
                perrord(WARNING "Get resource limit error");
                goto rtn;
            } else {
                maxfd = rl.rlim_cur;
                rl.rlim_cur = rl.rlim_max;
                if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
                    perrord(WARNING "Set resource limit error");
                } else {
                    maxfd = rl.rlim_cur;
                }
            }
        } else {
            perrord(WARNING "Set resource limit error");
        }
        return NULL;
    }

    /* Create epoll file descriptor. */
    epfd = epoll_create(maxfd);
    if (epfd < 0) {
        perrord(EMERG "Create epoll file descriptor error");
        goto rtn;
    }
    srvp->epoll_fd = epfd;
    
    /* Create listen socket. */
    rtsp_lsn_sd = create_and_bind(srvp->rtsp_port);
    if (rtsp_lsn_sd < 0) {
        goto rtn;
    }
    if (listen(rtsp_lsn_sd, BACKLOG) < 0) {
        perrord(EMERG "Start listening connections error");
        goto rtn;
    }
    srvp->rtsp_lsn_sd = rtsp_lsn_sd;
    if (monitor_sd_event(srvp->rtsp_lsn_sd, RTSP_LSN_SD_DFL_EV) < 0) {
        goto rtn;
    }

    /* Register sd handler. */
    if (register_sd_handler(rtsp_lsn_sd, handle_lsn_sd, srvp) < 0) {
        printd(ERR "register rtsp_lsn_sd handler failed!\n");
        goto rtn;
    }

    ees = calloc(EPOLL_MAX_EVENTS, sizeof(struct epoll_event));
    if (ees == NULL) {
        printd("calloc for epoll events failed!\n");
        goto rtn;
    }

    /* Accept connections, then create a new thread for it. */
    printd(INFO "Start listening RTSP connections ...\n");
    while (srvp->thrd_running) {
        /* Check send buffer queue in each RTSP session,
         * then update the event of socket descriptors. */
        if (check_send_queue() < 0) {
            continue;
        }
        /* Reset the value of epoll_wait timeout. */
        min_delay_time = get_min_delay_time();
        timeout = (min_delay_time < 0) ? -1 : (min_delay_time / THOUSAND);

        /* Wait event notifications. */
        do {
            nfds = epoll_wait(epfd, ees, EPOLL_MAX_EVENTS, timeout);
        } while (nfds < 0 && errno == EINTR);
        if (nfds < 0) {
            perrord(ERR "epoll_wait for listen socket error");
            goto rtn;
        }

        /* Handle the readable sockets. */
        for (i = 0; i < nfds; i++) {
            if ((ees[i].events & EPOLLERR) ||
#ifdef EPOLLRDHUP
                (ees[i].events & EPOLLRDHUP) ||
#endif
                (ees[i].events & EPOLLHUP)) {
                /* Error occured on this descriptor. */
                printd(WARNING "epoll_wait error occured on this fd[%d]\n",
                       ees[i].events);
                destroy_rtsp_sess_contain_sd(ees[i].data.fd);
                continue;
            }

            do_sd_handler(ees[i].data.fd, ees[i].events);
        }

        /* Process the delayed task whose time is up. */
        do_delay_task();
    }

rtn:
    deregister_sd_handler(srvp->rtsp_lsn_sd);
    close(srvp->rtsp_lsn_sd);
    close(srvp->http_lsn_sd);
    close(srvp->epoll_fd);
    srvp->rtsp_lsn_sd = -1;
    srvp->http_lsn_sd = -1;
    freez(ees);
    leaving_thread();
    return NULL;
}

int start_rtsp_srv(void)
{
    int ret = 0;

    rtsp_srv.rtsp_lsn_sd = -1;
    rtsp_srv.http_lsn_sd = -1;
    rtsp_srv.rtsp_port = DFL_RTSP_PORT;
    rtsp_srv.http_port = DFL_HTTP_PORT;
    rtsp_srv.thrd_running = 1;
    init_rtsp_sess_list();
    init_sd_handler_list();
    init_delay_task_queue();

    if ((ret = pthread_create(&rtsp_srv.rtsp_srv_tid, NULL,
                              rtsp_srv_thrd, &rtsp_srv)) != 0) {
        printd(EMERG "Create thread rtsp_srv_thrd error: %s\n", strerror(ret));
        return -1;
    }
    usleep(100 * 1000);
    return 0;
}

int stop_rtsp_srv(void)
{
    rtsp_srv.thrd_running = 0;
    deinit_delay_task_queue();
    deinit_sd_handler_list();
    deinit_rtsp_sess_list();

    return 0;
}
