/***************************************************************************************************
*                                    FSTR - FireStreamer
*                                    www.firestreamer.rs
***************************************************************************************************/

/**
* \file     firestreamer.c
* \ingroup  g_applspec
* \brief    Implementation of the FireSteamer application.
* \author   Milos Ladicorbic
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE                                                 /* See feature_test_macros(7) */
#endif

#include "firestreamer.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/gstbuffer.h>
#include <gst/gstmemory.h>
#include <pthread.h>

#define CHAR_PARAM      128   /* NOTE: we want to simplify application, do not use dynamic memory */

/* the FireStreamer object's data structure */
typedef struct FireStreamerTag {
    /* video parameters */
    uint32_t        width;
    uint32_t        height;
    uint8_t         fps;
    uint8_t         quality;
    bool_t          grayscale;                                      /* convert video to grayscale */
    /* stream parameters */
    char            url[CHAR_PARAM];
    char            username[CHAR_PARAM];
    char            password[CHAR_PARAM];
    /* gst - GStreamer */
    GMainLoop      *pMainLoop;                 /* gstreamer main loop needed for gst messages bus */
    GstPipeline    *pipeline;                                                    /* main pipeline */
    GstState        state;                                       /* current state of the pipeline */
    GstBus         *bus;                                                             /* bus watch */
    guint           bus_watch_id;                                                 /* bus watch ID */
    GstMessage     *msg;
    GstAppSrc      *appsrc;                                              /* application feed data */
    GstElement     *sourceFilter;
    GstElement     *h264Enc;
    GstElement     *encFilter;
    GstElement     *videoqueue;                                                    /* video queue */
    GstElement     *rtspClientSink;
    pthread_t       gstThreadId;                               /* ID returned by pthread_create() */
    GstElement     *vcYuvToGs;
    GstElement     *vcYuvToGsCaps;
    GstElement     *vcGsToYuv;
    GstElement     *vcGsToYuvCaps;
    /* helper */
    GstElement     *fakesink;                                     /* fake sink for testing stream */
    GstElement     *identity;                                           /* helper identity plugin */
    uint32_t        debugCounter;
} FireStreamer_t;

static FireStreamer_t l_fireSteramer;               /* single instance of the FireStreamer object */
FireStreamer_t *pThis = &l_fireSteramer;                                 /* global object pointer */

/* private function declarations */
static gboolean FireStreamer_gst_busCall__(GstBus *bus, GstMessage *msg, FireStreamer_t *pPipeline);
static void* FireStreamer_gst_mainLoop__(void *pArgument);
static void FireStreamer_gst_free__(void);


bool_t FireStreamer_initialize (char *url, char *username, char * password, uint32_t width,
                                uint32_t height, bool_t grayscale) {
    static uint8_t nFireStreamers = 0;         /* number of FireStreamer objects allocated so far */
    unsigned int major, minor, micro, nano;
    bool_t success = TRUE;
    int retVal;
    GstStateChangeReturn gstRet;

    assert(nFireStreamers < 1);                  /* only one FireStreamer object can be allocated */
    memset(&l_fireSteramer, 0, sizeof(l_fireSteramer));

    /* check input parameters */
    assert(url != NULL);
    assert(strlen(url) < sizeof(pThis->url));
    assert(width >= 32 && width <= 1920);
    assert(height >= 32 && height <= 1080);

    /* save stream parameters */
    snprintf(pThis->url, sizeof(pThis->url), "%s", url);
    if (username != NULL) {
        assert(strlen(username) < sizeof(pThis->username));
        snprintf(pThis->username, sizeof(pThis->username), "%s", username);
    }
    if (password != NULL) {
        assert(strlen(password) < sizeof(pThis->password));
        snprintf(pThis->password, sizeof(pThis->password), "%s", password);
    }
    pThis->width = width;
    pThis->height = height;
    pThis->grayscale = grayscale;

    /* Initialize GStreamer */
    gst_init(NULL, NULL);
    pThis->pMainLoop = g_main_loop_new (NULL, FALSE);
    gst_version(&major, &minor, &micro, &nano);
    gst_update_registry();

    /* start main loop thread to get message bus working */
    retVal = pthread_create(&pThis->gstThreadId, NULL, &FireStreamer_gst_mainLoop__,
                            pThis);
    assert(retVal == 0);                             /* pthread_create() must return with success */
    pthread_setname_np(pThis->gstThreadId, "gstMsgBus");

    /* Create gstreamer elements */
    pThis->pipeline = (GstPipeline*)gst_pipeline_new ("firestreamer");
    pThis->appsrc   = (GstAppSrc*)gst_element_factory_make("appsrc", "videoSource");
    pThis->sourceFilter = gst_element_factory_make("capsfilter", "sourceFilter");
    pThis->h264Enc = gst_element_factory_make("v4l2h264enc", "h264Encoder");
    pThis->encFilter = gst_element_factory_make("capsfilter", "encoderFilter");
    pThis->videoqueue = gst_element_factory_make("queue", "videoqueue");
    pThis->rtspClientSink = gst_element_factory_make("rtspclientsink", "videosink");
    if (pThis->grayscale == TRUE) {
        pThis->vcYuvToGs = gst_element_factory_make("videoconvert", "vcYuvToGs");
        pThis->vcYuvToGsCaps = gst_element_factory_make("capsfilter", "vcYuvToGsCaps");
        pThis->vcGsToYuv = gst_element_factory_make("videoconvert", "vcGsToYuv");
        pThis->vcGsToYuvCaps = gst_element_factory_make("capsfilter", "vcGsToYuvCaps");
    }

    if (!pThis->pipeline) {
        g_printerr ("ERROR: 'pipeline' main could be created.\n");
        success = FALSE;
    }
    if (!pThis->appsrc) {
        g_printerr ("ERROR: 'appsrc' element could be created.\n");
        success = FALSE;
    }
    if (!pThis->sourceFilter || !pThis->encFilter) {
        g_printerr ("ERROR: 'capsfilter' element could be created.\n");
        success = FALSE;
    }
    if (!pThis->h264Enc) {
        g_printerr ("ERROR: 'v4l2h264enc' element could be created.\n");
        success = FALSE;
    }
    if (!pThis->videoqueue) {
        g_printerr ("ERROR: 'queue' element could be created.\n");
        success = FALSE;
    }
    if (!pThis->rtspClientSink) {
        g_printerr ("ERROR: 'rtspclientsink' element could be created.\n");
        success = FALSE;
    }
    if ((pThis->grayscale == TRUE) && (!pThis->vcYuvToGs || !pThis->vcGsToYuv)) {
        g_printerr ("ERROR: 'videoconvert' element could be created.\n");
        success = FALSE;
    }

    if (success != TRUE) {
        g_printerr ("ERROR: Not all elements could be created.\n");
        FireStreamer_gst_free__();
        return FALSE;
    }

    /* set element properties */
    g_object_set(G_OBJECT(pThis->appsrc), "do-timestamp", TRUE, NULL);
    g_object_set(G_OBJECT(pThis->appsrc), "is_live", TRUE, NULL);
    g_object_set(G_OBJECT(pThis->appsrc), "format", GST_FORMAT_TIME, NULL);

    gchar       *capsstr;
    GstCaps     *caps;

    capsstr = g_strdup_printf("video/x-raw,width=%d, height=%d, framerate=30/1, format=(string)YUY2,"
                              "interlace-mode=(string)progressive, colorimetry=(string)bt601",
                               pThis->width, pThis->height);
    caps = gst_caps_from_string(capsstr);
    g_object_set(G_OBJECT(pThis->sourceFilter), "caps", caps, NULL);     /* caps for sourceFilter */
    g_free(capsstr);
    gst_caps_unref(caps);
    capsstr = g_strdup_printf("video/x-h264, profile=high, level=(string)4");
    caps = gst_caps_from_string(capsstr);
    g_object_set(G_OBJECT(pThis->encFilter), "caps", caps, NULL);       /* caps for h.264 encoder */
    g_free(capsstr);
    gst_caps_unref(caps);
    if (pThis->grayscale) {
        capsstr = g_strdup_printf("video/x-raw,format=(string)GRAY8");
        caps = gst_caps_from_string(capsstr);
        g_object_set(G_OBJECT(pThis->vcYuvToGsCaps), "caps", caps, NULL);
        g_free(capsstr);
        gst_caps_unref(caps);
        capsstr = g_strdup_printf("video/x-raw,format=(string)YUY2");
        caps = gst_caps_from_string(capsstr);
        g_object_set(G_OBJECT(pThis->vcGsToYuvCaps), "caps", caps, NULL);
        g_free(capsstr);
        gst_caps_unref(caps);
    }

    g_object_set(G_OBJECT(pThis->rtspClientSink), "location", pThis->url, NULL);
    g_object_set(G_OBJECT(pThis->rtspClientSink), "user-id", pThis->username, NULL);
    g_object_set(G_OBJECT(pThis->rtspClientSink), "user-pw", pThis->password, NULL);
    g_object_set(G_OBJECT(pThis->rtspClientSink), "protocols", 0x00000024, NULL);
    g_object_set(G_OBJECT(pThis->rtspClientSink), "tls-validation-flags", 0, NULL);

    /* add list of elements to a bin */
    if (pThis->grayscale) {
        gst_bin_add_many(GST_BIN(pThis->pipeline), (GstElement*)pThis->appsrc,
                         pThis->sourceFilter, pThis->vcYuvToGs, pThis->vcYuvToGsCaps,
                         pThis->vcGsToYuv, pThis->vcGsToYuvCaps,
                         pThis->h264Enc, pThis->encFilter, pThis->videoqueue,
                         pThis->rtspClientSink, NULL);
    } else {
        gst_bin_add_many(GST_BIN(pThis->pipeline), (GstElement*)pThis->appsrc,
                         pThis->sourceFilter, pThis->h264Enc, pThis->encFilter, pThis->videoqueue,
                         pThis->rtspClientSink, NULL);
    }

    if (pThis->grayscale) {
        if(!gst_element_link_many((GstElement*)pThis->appsrc, pThis->sourceFilter,
                pThis->vcYuvToGs, pThis->vcYuvToGsCaps, pThis->vcGsToYuv, pThis->vcGsToYuvCaps,
                pThis->h264Enc, pThis->encFilter, pThis->videoqueue, pThis->rtspClientSink, NULL)) {
            g_printerr ("ERROR: Elements could not be linked.\n");
            FireStreamer_gst_free__();
            return FALSE;
        }
    } else {
        if(!gst_element_link_many((GstElement*)pThis->appsrc, pThis->sourceFilter, pThis->h264Enc,
                                   pThis->encFilter, pThis->videoqueue, pThis->rtspClientSink, NULL)) {
            g_printerr ("ERROR: Elements could not be linked.\n");
            FireStreamer_gst_free__();
            return FALSE;
        }
    }

    /* add a BUS message handler to HTTP pipeline */
    pThis->bus = gst_pipeline_get_bus (GST_PIPELINE (pThis->pipeline));
    pThis->bus_watch_id = gst_bus_add_watch (pThis->bus, (GstBusFunc)FireStreamer_gst_busCall__,
                                             (gpointer)pThis);

    /* start streamer */
    gstRet = gst_element_set_state ((GstElement*)pThis->pipeline, GST_STATE_PLAYING);
    if (gstRet == GST_STATE_CHANGE_FAILURE) {
         g_printerr ("ERROR: Unable to set the pipeline to the playing state.\n");
         FireStreamer_gst_free__();
         return FALSE;
    }
    nFireStreamers++;                                 /* number of allocated FireStreamer objects */

    g_usleep(1000000);         /* give a chance to gstreamer start pipeline before we push frames */
    return TRUE;
}

uint32_t FireStreamer_pushFrame (void *pData, uint32_t size) {

    GstBuffer      *buffer;
    uint32_t        nWritten = 0;
    GstFlowReturn   ret;

    assert(pThis->appsrc != NULL);

    buffer = gst_buffer_new_and_alloc(size);
    nWritten = gst_buffer_fill(buffer, 0, pData, size);

    /* push data to appsrc */
    ret = gst_app_src_push_buffer(GST_APP_SRC(pThis->appsrc), buffer);
    if (ret != GST_FLOW_OK) {
        g_printerr ("ERROR: -EINVAL GST_FLOW!\n");
    }

    return nWritten;
}


/* private function definition */
static gboolean FireStreamer_gst_busCall__ (GstBus *bus, GstMessage *msg,
                                            FireStreamer_t *pPipeline) {
    FireStreamer_t *pThis = pPipeline;
    UNUSED_ARGUMENT(bus);
    UNUSED_ARGUMENT(pThis);

    switch (GST_MESSAGE_TYPE(msg)) {

        case GST_MESSAGE_EOS: {
            printf("GST_MESSAGE_EOS\n");
            //TODO handle this properly
            break;
        }
        case GST_MESSAGE_ERROR: {
            gchar  *debug;
            GError *error;

            if (pPipeline->state == GST_STATE_NULL) {
                return TRUE;
            }

            printf("GST_MESSAGE_ERROR!\n");
            gst_message_parse_error(msg, &error, &debug);
            printf("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), error->message);
            printf("Debugging information: %s\n", debug ? debug : "none");

            if ((strcmp(GST_OBJECT_NAME(msg->src), "videoSource") == 0) ||
                    (strcmp(GST_OBJECT_NAME(msg->src), "h264Encoder") == 0) ||
                    (strcmp(GST_OBJECT_NAME(msg->src), "videosink") == 0)) {
                if ((strcmp(error->message, "Could not open resource for reading and writing.") == 0) ||
                        strcmp(error->message, "Could not establish connection to server.") == 0) {
//                    CriticalSection_enter(pThis->pCriticalSection);
//                    if (pPipeline->fed != TRUE) {
//                        pPipeline->fed = TRUE;
//                        pPipeline->statusCode = URLSNAPSHOT_STATUS_400_ERROR__;
//                        strcpy(pPipeline->statusMessage, "Bad Request");
//                        CriticalSection_exit(pThis->pCriticalSection);
//                        UrlSnapshot_passEvent_catchFinishing__(pPipeline);
//                    } else {
//                        CriticalSection_exit(pThis->pCriticalSection);
//                    }
                } else if ((strcmp(error->message, "Not authorized to access resource.") == 0) ||
                        (strcmp(error->message, "Unauthorized") == 0)) {
//                    CriticalSection_enter(pThis->pCriticalSection);
//                    if (pPipeline->fed != TRUE) {
//                        pPipeline->fed = TRUE;
//                        pPipeline->statusCode = URLSNAPSHOT_STATUS_401_ERROR__;
//                        strcpy(pPipeline->statusMessage, "Unauthorized");
//                        CriticalSection_exit(pThis->pCriticalSection);
//                        UrlSnapshot_passEvent_catchFinishing__(pPipeline);
//                    } else {
//                        CriticalSection_exit(pThis->pCriticalSection);
//                    }
                } else if (strcmp(error->message, "Could not open resource for reading.") == 0) {
//                    CriticalSection_enter(pThis->pCriticalSection);
//                    if (pPipeline->fed != TRUE) {
//                        pPipeline->fed = TRUE;
//                        pPipeline->statusCode = URLSNAPSHOT_STATUS_404_ERROR__;
//                        strcpy(pPipeline->statusMessage, "Not Found");
//                        CriticalSection_exit(pThis->pCriticalSection);
//                        UrlSnapshot_passEvent_catchFinishing__(pPipeline);
//                    } else {
//                        CriticalSection_exit(pThis->pCriticalSection);
//                    }
                } else {
//                    CriticalSection_enter(pThis->pCriticalSection);
//                    if (pPipeline->fed != TRUE) {
//                        pPipeline->fed = TRUE;
//                        pPipeline->statusCode = URLSNAPSHOT_STATUS_404_ERROR__;
//                        strcpy(pPipeline->statusMessage, "Not Found");
//                        CriticalSection_exit(pThis->pCriticalSection);
//                        UrlSnapshot_passEvent_catchFinishing__(pPipeline);
//                    } else {
//                        CriticalSection_exit(pThis->pCriticalSection);
//                    }
                }
            }
            g_free (debug);
            g_error_free (error);
            break;
        }
        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state, pending_state;

            gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pPipeline->pipeline)) {
                pPipeline->state = new_state;
                printf("State set from %s to %s\n",
                gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
            }
            break;
        }
        default: {
            break;
        }
    }

    return TRUE;
}

static void* FireStreamer_gst_mainLoop__ (void *pArgument) {
    FireStreamer_t *pThis = (FireStreamer_t*)pArgument;

    g_main_loop_run (pThis->pMainLoop);

    printf("%s EXIT\n", __func__);                                    /* this should never happen */
    return (void*) 0;
}

static void FireStreamer_gst_free__ (void) {

    if (pThis->gstThreadId != 0) {
        pthread_cancel(pThis->gstThreadId);
        pThis->gstThreadId = 0;
    }
    if (pThis->pipeline != NULL) {
        gst_object_unref(pThis->pipeline);
    }
    if (pThis->msg != NULL) {
        gst_message_unref (pThis->msg);
    }
    if (pThis->bus != NULL) {
        gst_object_unref (pThis->bus);
    }
    if (pThis->bus_watch_id != 0) {
        g_source_remove(pThis->bus_watch_id);
        pThis->bus_watch_id = 0;
    }
    if (pThis->appsrc != NULL) {
        gst_object_unref(pThis->appsrc);
    }
    if (pThis->sourceFilter != NULL) {
        gst_object_unref(pThis->sourceFilter);
    }
    if (pThis->h264Enc != NULL) {
        gst_object_unref(pThis->h264Enc);
        pThis->h264Enc = NULL;
    }
    if (pThis->encFilter != NULL) {
        gst_object_unref(pThis->encFilter);
        pThis->encFilter = NULL;
    }
    if (pThis->videoqueue != NULL) {
        gst_object_unref(pThis->videoqueue);
        pThis->videoqueue = NULL;
    }
    if (pThis->rtspClientSink != NULL) {
        gst_object_unref(pThis->rtspClientSink);
        pThis->rtspClientSink = NULL;
    }
}

