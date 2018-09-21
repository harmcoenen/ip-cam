#ifndef IPCAMGSTCAPT_H
#define IPCAMGSTCAPT_H

char *working_dir = NULL;
const char *capture_subdir = "/cap";
const char *uploads_subdir = "/upl";
const char *appl_video = "video";
const char *appl_photo = "photo";
char src_video_padname[50];
char capture_dir[PATH_MAX];
char capture_file[PATH_MAX];
char uploads_dir[PATH_MAX];
char preupl_file[PATH_MAX]; /* Prepared for upload with timestamp */
char upload_file[PATH_MAX];
char openedfilename[PATH_MAX];
char closedfilename[PATH_MAX];
char username_passwd[25];
static gboolean user_interrupt = FALSE;
guint mainloop_timer_id;
guint upload_timer_id;

enum Application {VIDEO = 1, PHOTO = 2};

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    enum Application appl;
    guint appl_param;
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
static gboolean upload_timer (CustomData *data);
static gboolean mainloop_timer (CustomData *data);
static void handle_interrupt_signal (int signal);

static gboolean move_to_upload_directory (CustomData *data);
static int is_dir (const char *dir_to_test);
static int prepare_dir (const char *dir_to_create);
static int prepare_work_environment (void);
static void initialize(CustomData *data);
static void print_help(void);
static int handle_arguments(int argc, char *argv[], CustomData *data);

#endif /* IPCAMGSTCAPT_H */
