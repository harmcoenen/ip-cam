#ifndef IPCAMGSTCAPT_H
#define IPCAMGSTCAPT_H

const char *CAPTURE_DIR = "rec";
const char *UPLOAD_DIR = "upl";
char openenfilename[150];
char closedfilename[150];

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
    gboolean file_is_open;
    gchar *current_opened_file;
    gchar *current_closed_file;
} CustomData;

static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data);
static gboolean handle_keyboard (GIOChannel *channel, GIOCondition cond, CustomData *data);
static void cb_message (GstBus *bus, GstMessage *msg, CustomData *data);
static GstPadProbeReturn pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data);
static gboolean timer_expired (CustomData *data);
static gboolean watch_mainloop_timer_expired (CustomData *data);
void handle_interrupt_signal(int signal);

#endif /* IPCAMGSTCAPT_H */
