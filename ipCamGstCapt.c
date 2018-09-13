#include <gst/gst.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include "ipCamGstCapt.h"
#include "ipCamPrinting.h"
#include "ipCamFTP.h"

/* This function will be called by the pad-added signal */
static void handle_pad_added (GstElement *src, GstPad *new_pad, CustomData *data) {
    GstPad *sink_pad = gst_element_get_static_pad (data->depay, "sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print ("\nReceived new pad '%s' from '%s'", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked (sink_pad)) {
        g_print ("\nWe are already linked. Ignoring.");
        goto exit;
    }

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps (new_pad);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);
    if (!g_str_has_prefix (new_pad_type, "application/x-rtp")) {
        g_print ("\nIt has type '%s' which is not application/x-rtp. Ignoring.", new_pad_type);
        goto exit;
    }

    /* Attempt the link */
    ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
        g_print ("\nType is '%s' but link failed.", new_pad_type);
    } else {
        g_print ("\nLink succeeded (type '%s').", new_pad_type);
    }

exit:
    if (new_pad_caps != NULL) gst_caps_unref (new_pad_caps);
    gst_object_unref (sink_pad);
}

/* Process keyboard input */
static gboolean handle_keyboard (GIOChannel *channel, GIOCondition cond, CustomData *data) {
    gchar *retString;
    gsize length;
    gsize terminator_pos;
    GError *error = NULL;
    GIOStatus retStatus;

    retStatus = g_io_channel_read_line (channel, &retString, &length, &terminator_pos, &error);
    switch (retStatus) {
        case G_IO_STATUS_ERROR:
            if (error != NULL) {
                //g_printf (error->message);
                exit(-10);
            }
            break;
        case G_IO_STATUS_NORMAL: {
            if (strcmp ("quit\n", retString) == 0) {
                user_interrupt = TRUE;
                gst_element_set_state (data->pipeline, GST_STATE_READY);
                g_main_loop_quit (data->loop);
            } else {
                int index = g_ascii_strtoull (retString, NULL, 0);
                if (index < 0 || index >= 8) {
                  g_printerr ("Index out of bounds\n");
                } else {
                  g_print ("You presses %d\n", index);
                }
            }
            break;
        }
        default:
            break;
    }

    g_free (retString);
    return TRUE;
}

static void handle_bus_message (GstBus *bus, GstMessage *msg, CustomData *data) {

    // https://gstreamer.freedesktop.org/documentation/design/messages.html
    // https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstMessage.html
    switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_EOS:
            /* end-of-stream */
            g_print ("\nEnd-Of-Stream message [%s][%s] received.", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg));
            gst_element_set_state (data->pipeline, GST_STATE_READY);
            g_main_loop_quit (data->loop);
            break;

        case GST_MESSAGE_ERROR: {
            GError *err = NULL;
            gchar *dbg_info = NULL;

            gst_message_parse_error (msg, &err, &dbg_info);
            g_printerr ("\nERROR from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
            g_printerr ("\nDebugging info: %s", (dbg_info) ? dbg_info : "none");
            g_error_free (err);
            g_free (dbg_info);
            gst_pad_add_probe (data->blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, pad_probe_cb, data->decoder, NULL);
            //gst_element_set_state (data->pipeline, GST_STATE_READY);
            //g_main_loop_quit (data->loop);
            break;
        }

        case GST_MESSAGE_WARNING: {
            GError *err = NULL;
            gchar *dbg_info = NULL;

            gst_message_parse_warning (msg, &err, &dbg_info);
            g_print ("\nWarning from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
            g_print ("\nDebugging info: %s", (dbg_info) ? dbg_info : "none");
            g_error_free (err);
            g_free (dbg_info);
            break;
        }

        case GST_MESSAGE_INFO: {
            GError *err = NULL;
            gchar *dbg_info = NULL;

            gst_message_parse_info (msg, &err, &dbg_info);
            g_print ("\nInfo from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
            g_print ("\nDebugging info: %s", (dbg_info) ? dbg_info : "none");
            g_error_free (err);
            g_free (dbg_info);
            break;
        }

        case GST_MESSAGE_TAG: {
            GstTagList *tags = NULL;

            gst_message_parse_tag (msg, &tags);
            g_print ("\nTags from element %s", GST_OBJECT_NAME (msg->src));
            gst_tag_list_foreach (tags, print_tag, NULL);
            gst_tag_list_unref (tags);
            break;
        }

        case GST_MESSAGE_PROGRESS: {
            GstProgressType type;
            gchar *category, *text;

            gst_message_parse_progress (msg, &type, &category, &text);
            switch (type) {
                case GST_PROGRESS_TYPE_START:
                    g_print ("\nProgress Start [%s][%s]", category, text);
                    break;
                case GST_PROGRESS_TYPE_CONTINUE:
                    g_print ("\nProgress Continue [%s][%s]", category, text);
                    break;
                case GST_PROGRESS_TYPE_COMPLETE:
                    g_print ("\nProgress Complete [%s][%s]", category, text);
                    break;
                case GST_PROGRESS_TYPE_CANCELED:
                    g_print ("\nProgress Cancelled [%s][%s]", category, text);
                    break;
                case GST_PROGRESS_TYPE_ERROR:
                    g_print ("\nProgress Error [%s][%s]", category, text);
                    break;
                default:
                    g_print ("\nUndefined progress type.");
                    break;
            }
            g_free (category);
            g_free (text);
            break;
        }

        case GST_MESSAGE_BUFFERING: {
            gint percent = 0;

            /* If the stream is live, we do not care about buffering. */
            if (data->is_live) break;

            gst_message_parse_buffering (msg, &percent);
            g_print (" - Buffering (%3d%%) - \r", percent);
            /* Wait until buffering is complete before start/resume playing */
            if (percent < 100)
                gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
            else
                gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
            break;
        }

        case GST_MESSAGE_CLOCK_LOST:
            /* Get a new clock */
            g_print ("\nClock lost message [%s][%s] received.", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg));
            gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
            gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
            break;

        case GST_MESSAGE_STREAM_STATUS: {
            GstStreamStatusType type;
            GstElement *owner;
            gchar *path_source, *path_owner;
            const GValue *val;
            GstTask *task = NULL;

            //g_print ("\nStream Status message [%s][%s] received.", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg));
            gst_message_parse_stream_status (msg, &type, &owner);
            path_source = gst_object_get_path_string (GST_MESSAGE_SRC (msg));
            path_owner = gst_object_get_path_string (GST_OBJECT (owner));
            //g_print ("\ntype: %d    source: %s", type, path_source);
            //g_print ("\n           owner:  %s", path_owner);
            g_free (path_source);
            g_free (path_owner);
            val = gst_message_get_stream_status_object (msg);
            //g_print ("\nobject: type %s, value %p", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
            /* see if we know how to deal with this object */
            if (G_VALUE_TYPE (val) == GST_TYPE_TASK) task = g_value_get_object (val);
            switch (type) {
                case GST_STREAM_STATUS_TYPE_CREATE:
                    //g_print ("\nStream status CREATE, created task %p, type %s, value %p", task, G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                case GST_STREAM_STATUS_TYPE_ENTER:
                    //g_print ("\nStream status ENTER, type %s, value %p", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    /* setpriority (PRIO_PROCESS, 0, -10); */
                    break;
                case GST_STREAM_STATUS_TYPE_LEAVE:
                    //g_print ("\nStream status LEAVE, type %s, value %p", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                case GST_STREAM_STATUS_TYPE_DESTROY:
                    //g_print ("\nStream status DESTROY, type %s, value %p", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                case GST_STREAM_STATUS_TYPE_START:
                    //g_print ("\nStream status START, type %s, value %p", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                case GST_STREAM_STATUS_TYPE_PAUSE:
                    //g_print ("\nStream status PAUSE, type %s, value %p", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                case GST_STREAM_STATUS_TYPE_STOP:
                    //g_print ("\nStream status STOP, type %s, value %p", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                default:
                    //g_print ("\nStream status unknown as default is hit.");
                    break;
            }
            break;
        }

        case GST_MESSAGE_STREAM_START:
            g_print ("\nStream Start message [%s][%s] received.", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg));
            break;

        case GST_MESSAGE_ELEMENT:
            //g_print ("\nElement message [%s][%s][%s] received.", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg), gst_structure_get_name (gst_message_get_structure (msg)));
            if (strcmp ("splitmuxsink-fragment-opened", gst_structure_get_name (gst_message_get_structure (msg))) == 0){
                strcpy (openedfilename, gst_structure_get_string (gst_message_get_structure (msg), "location"));
                g_print ("\nCapture filename just opened is: [%s]", openedfilename);
            }
            if (strcmp ("splitmuxsink-fragment-closed", gst_structure_get_name (gst_message_get_structure (msg))) == 0){
                strcpy (closedfilename, gst_structure_get_string (gst_message_get_structure (msg), "location"));
                g_print ("\nCapture filename just closed is: [%s]", closedfilename);
                /* move it to the ftp upload directory to keep it safe */
                move_to_upload_directory(data);
            }
            break;

        case GST_MESSAGE_STATE_CHANGED:
            /* We are only interested in state-changed messages from the pipeline */
            if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                g_print ("\nPipeline state changed from %s to %s", gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
                /* Print the current capabilities of the sink element */
                //print_pad_capabilities (data->convert, "sink");
                //print_pad_capabilities (data->encoder, "sink");
                //print_pad_capabilities (data->parser, "sink");
                //print_pad_capabilities (data->splitsink, "video");
            }
            break;

        case GST_MESSAGE_LATENCY:
            g_print ("\nRecalculate latency");
            gst_bin_recalculate_latency ( GST_BIN (data->pipeline));
            break;

        case GST_MESSAGE_QOS: {
            gboolean live;
            guint64 running_time, stream_time, timestamp, duration;

            gst_message_parse_qos (msg, &live, &running_time, &stream_time, &timestamp, &duration);
            g_print ("\nQoS info: [%s]", (live) ? "LIVE" : "Not live");
            g_print (", rt [%" G_GUINT64_FORMAT "]", running_time);
            g_print (", st [%" G_GUINT64_FORMAT "]", stream_time);
            g_print (", ts [%" G_GUINT64_FORMAT "]", timestamp);
            g_print (", dur [%" G_GUINT64_FORMAT "]", duration);
            break;
        }

        default:
            /* Unhandled message */
            g_printerr ("\nUnhandled message [%s][%s] received.", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg));
            break;
    }
}

static GstPadProbeReturn pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data) {
    GstPad *sinkpad;

    g_print ("\nPad is blocked now");

    /* remove the probe first */
    //gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

    /* Send the EOS signal to decently close the capture files */
    sinkpad = gst_element_get_static_pad (GST_ELEMENT (user_data), "sink");
    gst_pad_send_event (sinkpad, gst_event_new_eos ());
    gst_object_unref (sinkpad);

    //return GST_PAD_PROBE_OK;
    return GST_PAD_PROBE_REMOVE;
}


int get_list_of_files_to_upload (CustomData *data) {
    struct dirent *de;
    int n_files = 0;
    gboolean file_found = FALSE;

    DIR *dr = opendir(uploads_dir); 
    if (dr == NULL) {
        g_print ("\nCould not open current directory");
        return 0;
    }
    while ((de = readdir(dr)) != NULL) {
        if ((strcmp (".", de->d_name) != 0) && (strcmp ("..", de->d_name) != 0)) {
            if (file_found == FALSE){
                strcpy (upload_file, de->d_name);
                file_found = TRUE;
                g_print ("\nFound file to upload: [%s]", upload_file);
            }
            n_files++;
            g_print ("\nFound another file to upload later: [%s]", de->d_name);
        }
    }
    closedir (dr);
    return (n_files);
}

static void what_time_is_it (char *newtime) {
    time_t seconds_since_epoch;
    struct tm *time_info;
    char time_string[20];

    /* The function time_t time(time_t *seconds) returns the time since the Epoch (00:00:00 UTC, January 1, 1970), measured in seconds. */
    /* If seconds is not NULL, the return value is also stored in variable seconds. */
    time (&seconds_since_epoch); /* See https://www.tutorialspoint.com/c_standard_library/c_function_time.htm */
    time_info = localtime (&seconds_since_epoch); /* See https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm */
    strftime (time_string, 20, "%Y_%m_%d_%H_%M_%S", time_info); /* See https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm */
    strcpy (newtime, time_string);
}

/* This function is called periodically */
static gboolean upload_timer (CustomData *data) {
    char time_string[20];
    char upload_file_fullname[PATH_MAX];

    what_time_is_it (time_string); g_print ("\nUpload_timer expired at time: [%s]", time_string);
    if (get_list_of_files_to_upload (data) > 0) {
        what_time_is_it (time_string); g_print ("\nFTP start time [%s]", time_string);
        strcpy (upload_file_fullname, uploads_dir); strcat (upload_file_fullname, "/"); strcat (upload_file_fullname, upload_file);
        if (ftp_upload_file (upload_file_fullname, upload_file, username_passwd) == 0) {
            g_print ("\nFile [%s] uploaded successfully", upload_file);
            /* remove file if upload successfull */
            if (remove (upload_file_fullname) == 0) {
                g_print ("\nFile [%s] deleted successfully", upload_file_fullname);
            } else {
                g_print ("\nError: unable to delete file [%s]", upload_file_fullname);
            }
        }
        what_time_is_it (time_string); g_print ("\nFTP stop time [%s]", time_string);
    }

    return TRUE; /* Otherwise the callback will be cancelled */
}

/* This function is called periodically */
static gboolean mainloop_timer (CustomData *data) {
    if (user_interrupt) {
        /* Add a pad probe to the src pad and block the downstream */
        /* Once the stream is blocked send a EOS signal to decently close the capture files */
        gst_pad_add_probe (data->blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, pad_probe_cb, data->decoder, NULL);
    }
    return TRUE; /* Otherwise the callback will be cancelled */
}

static void handle_interrupt_signal (int signal) {
    switch (signal) {
        case SIGINT:
            g_print ("\nCaught SIGINT");
            user_interrupt = TRUE;
            break;
        case SIGUSR1:
            g_print ("\nCaught SIGUSR1");
            break;
        default:
            g_print ("\nCaught signal: %d", signal);
            break;
    }
}

static gboolean move_to_upload_directory (CustomData *data) {
    char time_string[20];

    what_time_is_it (time_string);

    /* Concatenate the upload file name */
    strcpy (preupl_file, uploads_dir); strcat (preupl_file, "/"); strcat (preupl_file, time_string); strcat (preupl_file, ".mp4");
    if (rename (closedfilename, preupl_file) == -1) {
        g_printerr ("\nrename returned error [%s]", strerror (errno));
    } else {
        g_print("\nSuccesfully moved file [%s] to [%s]", closedfilename, preupl_file);
    }
    return TRUE;
}

/* test if directory exists (1 success, -1 does not exist, -2 not dir) */
static int is_dir (const char *dir_to_test) {
    DIR *dirptr;

    if (access (dir_to_test, F_OK) != -1 ) {
        // file exists
        if ((dirptr = opendir (dir_to_test)) != NULL) {
            closedir (dirptr);
        } else {
            return -2; /* dir_to_test exists, but not dir */
        }
    } else {
        return -1;     /* dir_to_test does not exist */
    }
    return 1;
}

/* Prepare a directory (0 success, -1 error upon creation, -2 not dir) */
static int prepare_dir (const char *dir_to_create) {
    int retval = 0;

    switch (is_dir (dir_to_create)) {
        case -2: { // not a dir
            g_printerr ("\n[%s] is not a valid directory.", dir_to_create);
            retval = -2;
            break;
        }
       case -1: { // does not exist
            if (mkdir (dir_to_create, 0777) == -1) {
                g_printerr ("\nmkdir returned error [%s].", strerror (errno));
                retval = -1;
            } else {
                g_print("\nSuccesfully created directory [%s]", dir_to_create);
                retval = 0;
            }
            break;
       }
        case 1: { // exists
                g_print("\nDirectory [%s] already exists.", dir_to_create);
                retval = 0;
            break;
        }
        default:
            break;
    }
    return (retval);
}

int main (int argc, char *argv[]) {
    CustomData data;
    GstBus *bus;
    GstCaps *caps;
    GstStateChangeReturn ret;
    GIOChannel *io_channel;
    struct sigaction sa;
    static unsigned int runs = 0;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Initialize our data structure */
    memset (&data, 0, sizeof (data));
    memset (capture_dir, '\0', sizeof (capture_dir));
    memset (capture_file, '\0', sizeof (capture_file));
    memset (uploads_dir, '\0', sizeof (uploads_dir));
    memset (preupl_file, '\0', sizeof (preupl_file));
    memset (upload_file, '\0', sizeof (upload_file));
    memset (openedfilename, '\0', sizeof (openedfilename));
    memset (closedfilename, '\0', sizeof (closedfilename));
    memset (username_passwd, '\0', sizeof (username_passwd));

    if (argc > 3){
        g_print ("\nURI [%s]", argv[1]);
        g_print ("\nuser-name [%s]", argv[2]);
        g_print ("\npassword [%s]", argv[3]);
        strcpy (username_passwd, argv[2]); strcat (username_passwd, ":"); strcat (username_passwd, argv[3]);
        g_print ("\ncombi is [%s]", username_passwd);
    } else {
        g_printerr ("\nToo little parameters. Please supply URI, user name and password.");
        exit (-2);
    }

    // Determine the current working directory and prepare it
    working_dir = getcwd (NULL, 0);
    if (working_dir != NULL) {
        g_print ("\nCurrent working dir: [%s][%lu]", working_dir, strlen (working_dir));
        strcpy (capture_dir, working_dir); strcat (capture_dir, capture_subdir);
        strcpy (uploads_dir, working_dir); strcat (uploads_dir, uploads_subdir);
        if (prepare_dir (capture_dir) == 0) {
            g_print ("\nDirectory [%s] is available", capture_dir);
        } else {
            g_printerr ("\nDirectory [%s] is NOT available", capture_dir);
            exit (-4);
        }
        if (prepare_dir (uploads_dir) == 0) {
            g_print ("\nDirectory [%s] is available", uploads_dir);
        } else {
            g_printerr ("\nDirectory [%s] is NOT available", uploads_dir);
            exit (-4);
        }
        // "/home/harm/github/ip-cam/rec%03d.mp4"
        strcpy (capture_file, capture_dir); strcat (capture_file, "/"); strcat (capture_file, "rec%03d.mp4");
        g_print ("\nCapture file is [%s]", capture_file);
    } else {
        g_printerr ("\nCould not find the current working directory.");
        exit (-3);
    }

    // Setup the signal handler
    sa.sa_handler = &handle_interrupt_signal;
    sa.sa_flags = SA_RESTART;                    // Restart the system call, if at all possible
    sigfillset (&sa.sa_mask);                     // Block every signal during the handler
    if (sigaction (SIGINT, &sa, NULL) == -1) {    // Intercept SIGINT
        g_printerr ("\nError: cannot handle SIGINT");
    }
    if (sigaction (SIGUSR1, &sa, NULL) == -1) {   // Intercept SIGUSR1
        g_printerr ("\nError: cannot handle SIGUSR1");
    }

    /* Register a function that GLib will call every x seconds */
    g_timeout_add_seconds (1, (GSourceFunc)mainloop_timer, &data);
    g_timeout_add_seconds ((10 * 60), (GSourceFunc)upload_timer, &data);

    while (!user_interrupt) {
        runs++;

        /* Create the elements */
        data.source = gst_element_factory_make ("rtspsrc", "source");
        data.depay = gst_element_factory_make ("rtph264depay", "depay");
        data.decoder = gst_element_factory_make ("avdec_h264", "decoder");
        data.convert = gst_element_factory_make ("videoconvert", "convert");
        data.scale = gst_element_factory_make ("videoscale", "scale");
        data.encoder = gst_element_factory_make ("x264enc", "encoder");
        data.parser = gst_element_factory_make ("h264parse", "parser");
        data.muxer = gst_element_factory_make ("mp4mux", "muxer"); // matroskamux or avimux or mp4mux
        data.sink = gst_element_factory_make ("autovideosink", "sink");
        data.splitsink = gst_element_factory_make ("splitmuxsink", "splitsink");

        /* Attach a blockpad to this element to be able to stop the pipeline dataflow decently */
        data.blockpad = gst_element_get_static_pad (data.depay, "src");

        /* Create the empty pipeline */
        data.pipeline = gst_pipeline_new ("my-pipeline");

        if (!data.pipeline || !data.source || !data.depay || !data.decoder || !data.convert || !data.scale || !data.encoder || !data.parser || !data.muxer || !data.sink || !data.splitsink) {
            g_printerr ("\nNot all elements could be created.");
            return -1;
        }

        /* Build the pipeline. */
        gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.depay, data.decoder, data.convert, data.scale, data.encoder, data.parser, data.splitsink, NULL);

        /* Note that we are NOT linking the source at this point. We will do it later. */
        if (!gst_element_link_many (data.depay, data.decoder, data.convert, data.scale, NULL)) {
            g_printerr ("\nElement link problem.");
            gst_object_unref (data.pipeline);
            return -1;
        }

        /* Negotiate caps */
        //caps = gst_caps_from_string("video/x-raw, format=(string)RGB, width=320, height=240, framerate=(fraction)30/1");
        // https://en.wikipedia.org/wiki/Display_resolution
        // https://en.wikipedia.org/wiki/Display_resolution#/media/File:Vector_Video_Standards8.svg
        //caps = gst_caps_from_string("video/x-raw, width=1280, height=720"); // Change scale from HD 1080 to HD 720
        caps = gst_caps_from_string ("video/x-raw, width=1024, height=576"); // Change scale from HD 1080 to PAL
        if (!gst_element_link_filtered (data.scale, data.encoder, caps)) {
            g_printerr ("\nElement scale could not be linked to element tee.");
            gst_object_unref (data.pipeline);
            return -1;
        }
        gst_caps_unref (caps);

        /* Link the rest of the pipeline */
        if (!gst_element_link_many (data.encoder, data.parser, data.splitsink, NULL)) {
            g_printerr ("\nRest of elements link problem.");
            gst_object_unref (data.pipeline);
            return -1;
        }

        /* Set the URI to play */
        //g_object_set (data.source, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);
        g_object_set (data.source, "location", argv[1], NULL);
        g_object_set (data.source, "user-id", argv[2], NULL);
        g_object_set (data.source, "user-pw", argv[3], NULL);

        g_object_set (data.encoder, "tune", 4, NULL); /* important, the encoder usually takes 1-3 seconds to process this. Queue buffer is generally upto 1 second. Hence, set tune=zerolatency (0x4) */
        //g_object_set (data.encoder, "pass", 5, NULL);
        //g_object_set (data.encoder, "qp-min", 18, NULL);

        //g_object_set (data.parser, "split-packetized", TRUE, NULL);
        //g_object_set (data.parser, "access-unit", TRUE, NULL);
        //g_object_set (data.parser, "output-format", 1, NULL);
        //g_object_set (data.parser, "config-interval", -1, NULL);

        g_object_set (data.splitsink, "location", capture_file, NULL);
        g_object_set (data.splitsink, "max-size-bytes", (150 * 1048576), NULL); // in bytes. 0 = disable, default is 0
        g_object_set (data.splitsink, "max-size-time", (5 * 60 * GST_SECOND), NULL); // in nanoseconds. 0 = disable, default is 0
        g_object_set (data.splitsink, "max-files", 30, NULL); // default is 0
        g_object_set (data.splitsink, "muxer", data.muxer, NULL);

        /* Connect to the pad-added signal */
        g_signal_connect (data.source, "pad-added", G_CALLBACK (handle_pad_added), &data);

        /* Start playing the pipeline */
        g_print ("\nGo to PLAYING, run [%d]", runs);
        ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("\nUnable to set the pipeline to the playing state.");
            gst_object_unref (data.pipeline);
            return -1;
        } else if (ret == GST_STATE_CHANGE_NO_PREROLL) {
            data.is_live = TRUE;
            g_print ("\nLive stream");
        } else {
            g_print ("\nNot a live stream");
        }

        /* Listen to the bus */
        bus = gst_element_get_bus (data.pipeline);
        gst_bus_add_signal_watch (bus);
        g_signal_connect (bus, "message", G_CALLBACK (handle_bus_message), &data);

        /* Add a keyboard watch so we get notified of keystrokes */
#ifdef G_OS_WIN32
        io_channel = g_io_channel_win32_new_fd (fileno (stdin));
#else
        io_channel = g_io_channel_unix_new (fileno (stdin));
#endif
        g_io_add_watch (io_channel, G_IO_IN, (GIOFunc)handle_keyboard, &data);

        /* Run main loop */
        data.loop = g_main_loop_new (NULL, FALSE);
        g_main_loop_run (data.loop);

        /* Free resources */
        g_main_loop_unref (data.loop);
        gst_object_unref (bus);
        gst_element_set_state (data.pipeline, GST_STATE_NULL);
        gst_object_unref (data.pipeline);
    }

    /* Free resources */
    free (working_dir);

    g_print ("\n\nHave a nice day.\n");
    return 0;
}
