#include <gst/gst.h>
#include <gst/video/video.h>
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

    /* Only try to link if the sink pad is not yet linked */
    if (!gst_pad_is_linked (sink_pad)) {
        /* Check the new pad's type */
        new_pad_caps = gst_pad_get_current_caps (new_pad);
        new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
        new_pad_type = gst_structure_get_name (new_pad_struct);
        if (g_str_has_prefix (new_pad_type, "application/x-rtp")) {
            /* Attempt the link */
            ret = gst_pad_link (new_pad, sink_pad);
            if (GST_PAD_LINK_SUCCESSFUL (ret)) {
                g_print ("\nLink for video succeeded.");
                strcpy (src_video_padname, GST_PAD_NAME (new_pad));
                print_pad_capabilities (data->source, src_video_padname);
            } else {
                g_print ("\nLink for video failed.");
            }
        } else {
            g_print ("\nNew added pad has type '%s' which is not application/x-rtp. Ignoring.", new_pad_type);
        }
    } else {
        g_print ("\nDepay sink pad is already linked to Source src pad. Ignore this new added pad.");
    }

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
                g_source_remove (upload_timer_id);
                user_interrupt = TRUE;
                gst_element_set_state (data->pipeline, GST_STATE_READY);
                g_main_loop_quit (data->loop);
            } else if (strcmp ("motion\n", retString) == 0){
                //g_print("\nMotion %s, motion detected is [%s]", data->motion ? "started" : "stopped", data->motion_detected ? "TRUE" : "FALSE");
                g_print("\nMotion [%d], motion detected is [%d]", data->motion, data->motion_detected);
            } else if (strcmp ("motion start\n", retString) == 0){
                data->motion = TRUE;
                data->motion_detected = TRUE;
            } else if (strcmp ("motion stop\n", retString) == 0){
                data->motion = FALSE;
            } else {
                int index = g_ascii_strtoull (retString, NULL, 0);
                if (index < 0 || index >= 8) {
                  g_printerr ("\nIndex out of bounds.");
                } else {
                  g_print ("\nYou presses %d\n", index);
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

        case GST_MESSAGE_ELEMENT: {
            const GstStructure *msg_struct = gst_message_get_structure (msg);
            if (gst_structure_has_name (msg_struct, "splitmuxsink-fragment-opened")) {
                strcpy (openedfilename, gst_structure_get_string (msg_struct, "location"));
                g_print ("\nCapture filename just opened is: [%s]", openedfilename);
            } else if (gst_structure_has_name (msg_struct, "splitmuxsink-fragment-closed")) {
                strcpy (closedfilename, gst_structure_get_string (msg_struct, "location"));
                g_print ("\nCapture filename just closed is: [%s]", closedfilename);
                if (data->motion_detected == TRUE) {
                    /* Move it to the ftp upload directory to keep it safe */
                    move_to_upload_directory(data);
                    /* When there is currently no motion reset the detected flag */
                    if (data->motion == FALSE) data->motion_detected = FALSE;
                }
            } else if (gst_structure_has_name (msg_struct, "motion")) {
                if (gst_structure_has_field (msg_struct, "motion_begin")) {
                    data->motion = TRUE;
                    data->motion_detected = TRUE;
                    g_print ("\nMotion detected in area(s): %s", gst_structure_get_string (msg_struct, "motion_cells_indices"));
                } else if (gst_structure_has_field (msg_struct, "motion_finished")) {
                    data->motion = FALSE;
                    g_print ("\nMotion end");
                }
            } else {
                g_print ("\nElement message [%s][%s][%s] received (unhandled).", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg), gst_structure_get_name (msg_struct));
            }
            break;
        }

        case GST_MESSAGE_STATE_CHANGED:
            /* We are only interested in state-changed messages from the pipeline */
            if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                g_print ("\nPipeline state changed from %s to %s", gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
                /* Print the current capabilities of the source pad */
                if (new_state == GST_STATE_PLAYING) {
                    print_pad_capabilities (data->source, src_video_padname);
                }
                /* Print the current capabilities of a sink pad */
                //print_pad_capabilities (data->depay, "sink");
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

    DIR *dr = opendir (uploads_dir); 
    if (dr == NULL) {
        g_print ("\nCould not open current directory");
        return 0;
    }
    while ((de = readdir (dr)) != NULL) {
        if ((strcmp (".", de->d_name) != 0) && (strcmp ("..", de->d_name) != 0)) {
            if (file_found == FALSE){
                strcpy (upload_file, de->d_name);
                file_found = TRUE;
                g_print ("\nFound file to upload: [%s]", upload_file);
            } else {
                g_print ("\nFound another file to upload later: [%s]", de->d_name);
            }
            n_files++;
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
    char upload_file_fullname[PATH_MAX];
    time_t start_time, end_time;

    time (&start_time);
    g_print ("\nUpload_timer expired at %s", asctime (localtime (&start_time)));

    if (data->appl == VIDEO) {
        if (get_list_of_files_to_upload (data) > 0) {
            strcpy (upload_file_fullname, uploads_dir); strcat (upload_file_fullname, "/"); strcat (upload_file_fullname, upload_file);
            if (ftp_upload_file (upload_file_fullname, upload_file, username_passwd) == 0) {
                g_print ("\nFile [%s] uploaded successfully", upload_file);
            }
        }
    } else if (data->appl == PHOTO) {
        if (ftp_upload_files (uploads_dir, username_passwd) == 0) {
            g_print ("\nUpload of files finished without problems");
        }
    }

    time (&end_time);
    g_print ("\nFTP upload execution time = %f", difftime (end_time, start_time));

    return TRUE; /* Otherwise the callback will be cancelled */
}

/* This function is called periodically */
static gboolean snapshot_timer (CustomData *data) {
    if (data->appl == PHOTO) {
        if (save_snapshot (data) == 0) {
            g_print ("\nSnapshot saved.");
            strcpy (closedfilename, capture_file);
            move_to_upload_directory(data);
        }
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
            g_print ("\nCaught SIGINT, please wait....");
            g_source_remove (upload_timer_id);
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
    strcpy (preupl_file, uploads_dir); strcat (preupl_file, "/"); strcat (preupl_file, time_string);
    if (data->appl == VIDEO) {
        strcat (preupl_file, extension_video);
    } else if (data->appl == PHOTO) {
        strcat (preupl_file, extension_photo);
    }
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
        return -1; /* dir_to_test does not exist */
    }
    return 1;
}

/* Prepare a directory (0 success, -1 error upon creation, -2 not dir) */
static int prepare_dir (const char *dir_to_create) {
    int retval = 0;

    switch (is_dir (dir_to_create)) {
        case -2: // not a dir
            g_printerr ("\n[%s] is not a valid directory.", dir_to_create);
            retval = -2;
            break;
       case -1: // does not exist
            if (mkdir (dir_to_create, 0777) == -1) {
                g_printerr ("\nmkdir returned error [%s].", strerror (errno));
                retval = -1;
            } else {
                g_print("\nSuccesfully created directory [%s]", dir_to_create);
                retval = 0;
            }
            break;
        case 1: // exists
            g_print("\nDirectory [%s] already exists.", dir_to_create);
            retval = 0;
            break;
        default:
            break;
    }

    return (retval);
}

/* Prepare working environment (0 success, -1 error) */
static int prepare_work_environment (CustomData *data) {
    char *working_dir = NULL;
    int retval = 0;

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
            retval = -1;
        }
        if (prepare_dir (uploads_dir) == 0) {
            g_print ("\nDirectory [%s] is available", uploads_dir);
        } else {
            g_printerr ("\nDirectory [%s] is NOT available", uploads_dir);
            retval = -1;
        }
        strcpy (capture_file, capture_dir); strcat (capture_file, "/");
        if (data->appl == VIDEO) {
            strcat (capture_file, recording_filename); strcat (capture_file, extension_video);
        } else if (data->appl == PHOTO) {
            strcat (capture_file, snapshot_filename); strcat (capture_file, extension_photo);
        }
        g_print ("\nCapture file is [%s]", capture_file);

        free (working_dir);
    } else {
        g_printerr ("\nCould not find the current working directory.");
        retval = -1;
    }

    return (retval);
}

/* Initialize data structures and arrays */
static void initialize (CustomData *data) {
    memset (&data, 0, sizeof (data));
    memset (src_video_padname, '\0', sizeof (src_video_padname));
    memset (capture_dir, '\0', sizeof (capture_dir));
    memset (capture_file, '\0', sizeof (capture_file));
    memset (uploads_dir, '\0', sizeof (uploads_dir));
    memset (preupl_file, '\0', sizeof (preupl_file));
    memset (upload_file, '\0', sizeof (upload_file));
    memset (openedfilename, '\0', sizeof (openedfilename));
    memset (closedfilename, '\0', sizeof (closedfilename));
    memset (username_passwd, '\0', sizeof (username_passwd));
}

static void print_help (void) {
    g_print ("\n\033[4;031mIncorrect parameters detected.\033[0m Please supply URI, user name, password, application and parameter.");
    g_print ("\nFormat:");
    g_print ("\n./ipCamGstCapt <rtsp uri> <username> <password> <application> <parameter>");
    g_print ("\n\nExample that will capture videoclips of 5 minutes each:");
    g_print ("\n./ipCamGstCapt rtsp://192.168.178.28:88/videoMain username 'password' video 5");
    g_print ("\n\nExample that will capture a photo each 3 seconds:");
    g_print ("\n./ipCamGstCapt rtsp://192.168.178.28:88/videoMain username 'password' photo 3");
    g_print ("\n\n");
}

/* Handle initial arguments (0 success, -1 error) */
static int handle_arguments (int argc, char *argv[], CustomData *data) {
    if (argc > 5){
        g_print ("\nURI [%s]", argv[1]);
        g_print ("\nuser-name [%s]", argv[2]);
        g_print ("\npassword [%s]", argv[3]);
        strcpy (username_passwd, argv[2]); strcat (username_passwd, ":"); strcat (username_passwd, argv[3]);
        g_print ("\ncombi is [%s]", username_passwd);
        data->appl_param = atoi (argv[5]);
        if (strcmp (argv[4], appl_video) == 0) {
            data->appl = VIDEO;
            g_print ("\napplication is video with parameter [%d]", data->appl_param);
        } else if (strcmp (argv[4], appl_photo) == 0) {
            data->appl = PHOTO;
            g_print ("\napplication is photo with parameter [%d]", data->appl_param);
        } else {
            g_printerr ("\napplication [%s] is not yet supported.\n", argv[4]);
            print_help();
            return -1;
        }
    } else {
        print_help();
        return -1;
    }

    return 0;
}

static int create_video_pipeline (int argc, char *argv[], CustomData *data) {
    int retval = 0;
    GstCaps *caps;

    g_print ("\nCreating video pipeline.");

    /* Create the elements */
    data->source = gst_element_factory_make ("rtspsrc", "source");
    data->depay = gst_element_factory_make ("rtph264depay", "depay");
    data->decoder = gst_element_factory_make ("avdec_h264", "decoder");
    data->convert_a = gst_element_factory_make ("videoconvert", "convert_a");
    data->motioncells = gst_element_factory_make ("motioncells", "motioncells");
    data->convert_b = gst_element_factory_make ("videoconvert", "convert_b");
    data->scale = gst_element_factory_make ("videoscale", "scale");
    data->encoder = gst_element_factory_make ("x264enc", "encoder");
    data->parser = gst_element_factory_make ("h264parse", "parser");
    data->muxer = gst_element_factory_make ("mp4mux", "muxer"); // matroskamux or avimux or mp4mux
    data->splitsink = gst_element_factory_make ("splitmuxsink", "splitsink");

    /* Attach a blockpad to this element to be able to stop the pipeline dataflow decently */
    data->blockpad = gst_element_get_static_pad (data->depay, "src");

    /* Create the empty pipeline */
    data->pipeline = gst_pipeline_new ("video-pipeline");

    if (!data->pipeline || !data->source || !data->depay || !data->decoder || !data->convert_a || !data->motioncells ||
        !data->convert_b || !data->scale || !data->encoder || !data->parser || !data->muxer || !data->splitsink) {
        g_printerr ("\nNot all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline. */
    gst_bin_add_many (GST_BIN (data->pipeline), data->source, data->depay, data->decoder, data->convert_a, data->motioncells,
                        data->convert_b, data->scale, data->encoder, data->parser, data->splitsink, NULL);

    /* Note that we are NOT linking the source at this point. We will do it later. */
    if (!gst_element_link_many (data->depay, data->decoder, data->convert_a, data->motioncells, data->convert_b, data->scale, NULL)) {
        g_printerr ("\nElements for first part of video path could not be linked.\n");
        return -1;
    }

    /* Negotiate caps */
    //caps = gst_caps_from_string("video/x-raw, format=(string)RGB, width=320, height=240, framerate=(fraction)30/1");
    // https://en.wikipedia.org/wiki/Display_resolution
    // https://en.wikipedia.org/wiki/Display_resolution#/media/File:Vector_Video_Standards8.svg
    //caps = gst_caps_from_string("video/x-raw, width=1280, height=720"); // Change scale from HD 1080 to HD 720
    caps = gst_caps_from_string ("video/x-raw, width=1024, height=576"); // Change scale from HD 1080 to PAL
    if (!gst_element_link_filtered (data->scale, data->encoder, caps)) {
        g_printerr ("\nElement scale could not be linked to element encoder.\n");
        return -1;
    }
    gst_caps_unref (caps);

    /* Link the rest of the pipeline */
    if (!gst_element_link_many (data->encoder, data->parser, data->splitsink, NULL)) {
        g_printerr ("\nElements for last part of video path could not be linked.\n");
        return -1;
    }

    /* Set the URI to play */
    //g_object_set (data.source, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);
    g_object_set (data->source, "location", argv[1], NULL);
    g_object_set (data->source, "user-id", argv[2], NULL);
    g_object_set (data->source, "user-pw", argv[3], NULL);

    g_object_set (data->motioncells, "display", 1, NULL);
    g_object_set (data->motioncells, "cellscolor", "255,0,255", NULL);
    g_object_set (data->motioncells, "gridx", 12, NULL);
    g_object_set (data->motioncells, "gridy", 12, NULL);
    g_object_set (data->motioncells, "sensitivity", 0.1, NULL);
    g_object_set (data->motioncells, "threshold", 0.1, NULL);
    g_object_set (data->motioncells, "minimummotionframes", 1, NULL);

    g_object_set (data->encoder, "tune", 4, NULL); /* important, the encoder usually takes 1-3 seconds to process this. Queue buffer is generally upto 1 second. Hence, set tune=zerolatency (0x4) */

    g_object_set (data->splitsink, "location", capture_file, NULL);
    g_object_set (data->splitsink, "max-size-bytes", (data->appl_param * 30 * 1048576), NULL); // in bytes. 0 = disable, default is 0. Maximum 30 MB per minute.
    g_object_set (data->splitsink, "max-size-time", (data->appl_param * 60 * GST_SECOND), NULL); // in nanoseconds. 0 = disable, default is 0
    g_object_set (data->splitsink, "max-files", 30, NULL); // default is 0
    g_object_set (data->splitsink, "muxer", data->muxer, NULL);

    return (retval);
}

static int create_photo_pipeline (int argc, char *argv[], CustomData *data) {
    int retval = 0;
    GstCaps *caps;

    g_print ("\nCreating photo pipeline.");

    /* Create the elements */
    data->source = gst_element_factory_make ("rtspsrc", "source");
    data->depay = gst_element_factory_make ("rtph264depay", "depay");
    data->decoder = gst_element_factory_make ("avdec_h264", "decoder");
    data->tee = gst_element_factory_make ("tee", "tee");
    data->queue = gst_element_factory_make ("queue", "queue");
    data->convert_a = gst_element_factory_make ("videoconvert", "convert_a");
    data->motioncells = gst_element_factory_make ("motioncells", "motioncells");
    data->convert_b = gst_element_factory_make ("videoconvert", "convert_b");
    data->scale = gst_element_factory_make ("videoscale", "scale");
    data->videosink = gst_element_factory_make ("fakesink", "videosink"); //autovideosink

    /* Attach a blockpad to this element to be able to stop the pipeline dataflow decently */
    data->blockpad = gst_element_get_static_pad (data->depay, "src");

    /* Create the empty pipeline */
    data->pipeline = gst_pipeline_new ("photo-pipeline");

    if (!data->pipeline || !data->source || !data->depay || !data->decoder || !data->convert_a || 
        !data->motioncells || !data->convert_b || !data->scale || !data->videosink) {
        g_printerr ("\nNot all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline. */
    gst_bin_add_many (GST_BIN (data->pipeline), data->source, data->depay, data->decoder, data->convert_a, 
                      data->motioncells, data->convert_b, data->scale, data->videosink, NULL);

    /* Note that we are NOT linking the source at this point. We will do it later. */
    if (!gst_element_link_many (data->depay, data->decoder, data->convert_a, 
                                data->motioncells, data->convert_b, data->scale, NULL)) {
        g_printerr ("\nElements for first part of photo path could not be linked.\n");
        return -1;
    }

    /* Negotiate caps */
    //caps = gst_caps_from_string("video/x-raw, format=(string)RGB, width=320, height=240, framerate=(fraction)30/1");
    // https://en.wikipedia.org/wiki/Display_resolution
    // https://en.wikipedia.org/wiki/Display_resolution#/media/File:Vector_Video_Standards8.svg
    //caps = gst_caps_from_string("video/x-raw, width=1280, height=720"); // Change scale from HD 1080 to HD 720
    caps = gst_caps_from_string ("video/x-raw, width=1024, height=576"); // Change scale from HD 1080 to PAL
    if (!gst_element_link_filtered (data->scale, data->videosink, caps)) {
        g_printerr ("\nElement scale could not be linked to element encoder.\n");
        return -1;
    }
    gst_caps_unref (caps);

    /* Set the URI to play */
    //g_object_set (data.source, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);
    g_object_set (data->source, "location", argv[1], NULL);
    g_object_set (data->source, "user-id", argv[2], NULL);
    g_object_set (data->source, "user-pw", argv[3], NULL);

    g_object_set (data->motioncells, "display", 1, NULL);
    g_object_set (data->motioncells, "cellscolor", "255,0,255", NULL);
    g_object_set (data->motioncells, "gridx", 12, NULL);
    g_object_set (data->motioncells, "gridy", 12, NULL);
    g_object_set (data->motioncells, "sensitivity", 0.1, NULL);
    g_object_set (data->motioncells, "threshold", 0.1, NULL);
    g_object_set (data->motioncells, "minimummotionframes", 1, NULL);

    return (retval);
}

static int save_snapshot (CustomData *data) {
    GError *err = NULL;
    GstCaps *caps;
    GstBuffer *buf;
    GstSample *last_sample, *converted_sample;
    GstMapInfo map_info;

    g_object_get (data->videosink, "last-sample", &last_sample, NULL);
    if (last_sample == NULL) {
        g_printerr ("\nError getting last sample from sink");
        return -1;
    }

    caps = gst_caps_from_string ("image/jpeg");
    converted_sample = gst_video_convert_sample (last_sample, caps, GST_CLOCK_TIME_NONE, &err);
    gst_caps_unref (caps);
    gst_sample_unref (last_sample);

    if (converted_sample == NULL && err) {
        g_printerr ("\nError converting sample: %s", err->message);
        g_error_free (err);
        return -1;
    }

    buf = gst_sample_get_buffer (converted_sample);
    if (gst_buffer_map (buf, &map_info, GST_MAP_READ)) {
        if (!g_file_set_contents (capture_file, (const char *) map_info.data, map_info.size, &err)) {
            g_printerr ("\nCould not save thumbnail: %s", err->message);
            g_error_free (err);
            return -1;
        }
    }

    gst_buffer_unmap (buf, &map_info);
    gst_sample_unref (converted_sample);

    return 0;
}

int main (int argc, char *argv[]) {
    CustomData data;
    GstBus *bus;
    GstStateChangeReturn ret;
    GIOChannel *io_channel;
    struct sigaction sa;
    static unsigned int runs = 0;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Initialize data structures and arrays */
    initialize (&data);

    if (handle_arguments (argc, argv, &data) != 0) {
        g_print ("\n");
        return -1;
    }

    if (prepare_work_environment (&data) != 0) {
        g_print ("\n");
        return -1;
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
    mainloop_timer_id = g_timeout_add_seconds (1, (GSourceFunc)mainloop_timer, &data);
    snapshot_timer_id = g_timeout_add_seconds (data.appl_param, (GSourceFunc)snapshot_timer, &data);
    upload_timer_id = g_timeout_add_seconds (60, (GSourceFunc)upload_timer, &data);

    while (!user_interrupt) {
        runs++;

        switch (data.appl) {
            case VIDEO:
                if (create_video_pipeline (argc, argv, &data) != 0) {
                    gst_object_unref (data.pipeline);
                    return -1;
                }
                break;
            case PHOTO:
                if (create_photo_pipeline (argc, argv, &data) != 0) {
                    gst_object_unref (data.pipeline);
                    return -1;
                }
                break;
            default:
                g_printerr ("\nThis application is not yet supported.\n");
                return -1;
                break;
        }

        /* Connect to the pad-added signal */
        g_signal_connect (data.source, "pad-added", G_CALLBACK (handle_pad_added), &data);

        /* Start playing the pipeline */
        g_print ("\nGo to PLAYING, run [%d]", runs);
        ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("\nUnable to set the pipeline to the playing state.\n");
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

    g_print ("\n\nHave a nice day.\n");
    return 0;
}
