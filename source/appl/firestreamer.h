/***************************************************************************************************
*                                    FSTR - FireStreamer
*                                    www.firestreamer.rs
***************************************************************************************************/
#ifndef FIRE_STREAMER_H
#define FIRE_STREAMER_H

/**
* \file     firestreamer.h
* \ingroup  g_applspec
* \brief    API for the FireStreamer class.
* \author   Milos Ladicorbic
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* define helper things */
typedef enum {
    FALSE = 0,
    TRUE  = 1
} bool_t;

#define UNUSED_ARGUMENT(x_) (void)(x_)

/* Fire Streamer - API */
bool_t FireStreamer_initialize(char *url, char *username, char * password, uint32_t width,
                               uint32_t height, bool_t grayscale);
uint32_t FireStreamer_pushFrame(void *pData, uint32_t size);


#endif                                                                         /* FIRE_STREAMER_H */

#ifdef __cplusplus
}
#endif
