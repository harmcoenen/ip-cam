#ifndef IPCAMCAPTURE_H
#define IPCAMCAPTURE_H

#define DAY_IN_SECONDS (60 * 60 * 24)
#define RETENTION_PERIOD (DAY_IN_SECONDS * 14)

static const char *capture_subdir = "/cap";
static const char *uploads_subdir = "/upl";
static const char *appl_video = "video";
static const char *appl_photo = "photo";
static const char *reboot_filename="reboot.force";
static const char *recording_filename="rec%03d";
static const char *snapshot_filename="snapshot%d";
static const char *extension_video = ".mp4";
static const char *extension_photo = ".jpeg";
static const char *delimiter = "\n";
char src_video_padname[50];
char capture_dir[PATH_MAX];
char capture_file[PATH_MAX];
char uploads_dir[PATH_MAX];
char preupl_file[PATH_MAX]; /* Prepared for upload with timestamp */
char upload_file[PATH_MAX];
char openedfilename[PATH_MAX];
char closedfilename[PATH_MAX];
char username_passwd[25];
static gboolean motion = FALSE;
static gboolean motion_detected = FALSE;
static gboolean user_interrupt = FALSE;
static gboolean error_occurred = FALSE;
static gboolean force_reboot = FALSE;
static int n_warnings_as_errors = 0;
static int n_user_interrupts = 0;
guint mainloop_timer_id;
guint snapshot_timer_id;
guint upload_timer_id;
guint cleanup_timer_id;
pthread_t ftp_upload_thread_id;
pthread_t ftp_cleanup_thread_id;
pthread_mutex_t ftp_upload_mutex;
pthread_mutex_t ftp_cleanup_mutex;

/* Argument options for this application */
static const gchar *camera_uri = NULL;
static const gchar *rtsp_user = NULL;
static const gchar *rtsp_pass = NULL;
static const gchar *application = "video";
static gint timing = 5;
static gboolean motion_detection = FALSE;
static gboolean scale_down = FALSE;
static gboolean take_snapshot = FALSE;
static GOptionEntry options [] =
{
    { "camera-uri", 'l', 0, G_OPTION_ARG_STRING, &camera_uri, "Camera URI in the form of rtsp://<ip>:<port>/videoMain", NULL },
    { "rtsp-user", 'u', 0, G_OPTION_ARG_STRING, &rtsp_user, "Username for camera access", NULL },
    { "rtsp-pass", 'p', 0, G_OPTION_ARG_STRING, &rtsp_pass, "Password for camera access", NULL },
    { "application", 'a', 0, G_OPTION_ARG_STRING, &application, "Application mode; video or photo(default: video)", NULL },
    { "timing", 't', 0, G_OPTION_ARG_INT, &timing, "Minutes of video recording or interval seconds between each snapshot(default: 5)", NULL },
    { "motion-detection", 'm', 0, G_OPTION_ARG_NONE, &motion_detection, "Enable motion detection(default: disabled)", NULL},
    { "scale-down", 's', 0, G_OPTION_ARG_NONE, &scale_down, "Scale down the input stream to a lower resolution(default: disabled)", NULL },
    { NULL }
};

enum Application {VIDEO = 1, PHOTO = 2};
enum Application appl;

struct MemoryStruct {
    char *memory;
    size_t size;
};

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    GMainLoop *loop;
    gboolean is_live;
    gboolean pipeline_playing;
    GstElement *pipeline;
    GstElement *source;
    GstElement *depay;
    GstElement *decoder;
    GstElement *convert;
    GstElement *motioncells;
    GstElement *convert_b;
    GstElement *scale;
    GstElement *encoder;
    GstElement *parser;
    GstElement *muxer;
    GstElement *photosink;
    GstElement *splitsink;
    GstPad *blockpad;
} CustomData;

static void handle_pad_added(GstElement *src, GstPad *new_pad, CustomData *data);
static gboolean handle_keyboard(GIOChannel *channel, GIOCondition cond, CustomData *data);
static void handle_bus_message(GstBus *bus, GstMessage *msg, CustomData *data);
static GstPadProbeReturn pad_probe_cb(GstPad * pad, GstPadProbeInfo * info, gpointer user_data);
int get_list_of_files_to_upload(void);
static void what_time_is_it(char *newtime);
static void what_hour_is_it(char *newhour);
static gboolean retention_period_expired(const char *remote_dir_name);
static void setRebootFile(void);
static void resetRebootFile(void);
static void cleanup_remote_site(void);
static void *ftp_upload(void *arg);
static void *ftp_cleanup(void *arg);
static gboolean upload_timer(CustomData *data);
static gboolean cleanup_timer(CustomData *data);
static gboolean snapshot_timer(CustomData *data);
static gboolean mainloop_timer(CustomData *data);
static void handle_interrupt_signal(int signal);
static gboolean move_to_upload_directory(CustomData *data);
static int is_dir(const char *dir_to_test);
static int prepare_dir(const char *dir_to_create);
static int prepare_work_environment(CustomData *data);
static void initialize(CustomData *data);
static void print_help(void);
static int handle_arguments(CustomData *data);
static int create_video_pipeline(int argc, char *argv[], CustomData *data);
static int create_photo_pipeline(int argc, char *argv[], CustomData *data);
static int save_snapshot(CustomData *data);

#endif /* IPCAMCAPTURE_H */
