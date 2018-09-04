#include <gst/gst.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h> // sigaction(), sigsuspend(), sig*()
#include <unistd.h> // alarm()

const int maxRetries = 4;
const char *CAPTURE_DIR = "rec";
const char *UPLOAD_DIR = "upl";

static gboolean user_interrupt = FALSE;

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
    GstPad *blockpad;
} CustomData;

