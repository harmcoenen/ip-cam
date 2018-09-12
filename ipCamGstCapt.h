#ifndef IPCAMGSTCAPT_H
#define IPCAMGSTCAPT_H

char *working_dir = NULL;
const char *capture_subdir = "/rec";
const char *uploads_subdir = "/upl";
char capture_dir[PATH_MAX];
char capture_file[PATH_MAX];
char uploads_dir[PATH_MAX];
char upload_file[PATH_MAX];
char openedfilename[PATH_MAX];
char closedfilename[PATH_MAX];
char username_passwd[25];
static gboolean user_interrupt = FALSE;

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    GMainLoop *loop;
    gboolean is_live;
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

static void handle_pad_added (GstElement *src, GstPad *new_pad, CustomData *data);
static gboolean handle_keyboard (GIOChannel *channel, GIOCondition cond, CustomData *data);
static void handle_bus_message (GstBus *bus, GstMessage *msg, CustomData *data);
static GstPadProbeReturn pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data);
static void what_time_is_it (char *newtime);
static gboolean timer_expired (CustomData *data);
static gboolean watch_mainloop_timer_expired (CustomData *data);
static void handle_interrupt_signal (int signal);

static gboolean move_to_upload_directory (CustomData *data);
static int is_dir (const char *dir_to_test);
static int prepare_dir (const char *dir_to_create);

#endif /* IPCAMGSTCAPT_H */
