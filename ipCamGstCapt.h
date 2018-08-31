#include <gst/gst.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h> // sigaction(), sigsuspend(), sig*()
#include <unistd.h> // alarm()

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    GMainLoop *loop;
    gboolean is_live;
    gboolean timer_expired;
    GstElement *pipeline;
    GstElement *source;
    GstElement *depay;
    GstElement *decoder;
    GstElement *convert;
    GstElement *scale;
    GstElement *encoder;
    GstElement *parser;
    GstElement *muxer;
    GstElement *sink;
    GstElement *splitsink;
} CustomData;

static gboolean stopping = FALSE;
