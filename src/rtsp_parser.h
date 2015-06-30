/*********************************************************************
 * File Name    : parser.h
 * Description  : Parse the RTSP requeset message.
 * Author       : Hu Lizhen
 * Create Date  : 2012-11-23
 ********************************************************************/

#ifndef __PARSER_H__
#define __PARSER_H__


#include "rtsp_sess.h"


void parse_rtsp_req(struct rtsp_sess *sessp);


#endif /* __PARSER_H__ */
