#include "ipCamGstCapt.h"
#include "functions.h"

/* Functions below print the Capabilities in a human-friendly format */
static gboolean print_field (GQuark field, const GValue * value, gpointer pfx) {
    gchar *str = gst_value_serialize (value);

    g_print ("%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string (field), str);
    g_free (str);
    return TRUE;
}

static void print_caps (const GstCaps * caps, const gchar * pfx) {
    guint i;

    g_return_if_fail (caps != NULL);

    if (gst_caps_is_any (caps)) {
        g_print ("%sANY\n", pfx);
        return;
    }
    if (gst_caps_is_empty (caps)) {
        g_print ("%sEMPTY\n", pfx);
        return;
    }

    for (i = 0; i < gst_caps_get_size (caps); i++) {
        GstStructure *structure = gst_caps_get_structure (caps, i);

        g_print ("%s%s\n", pfx, gst_structure_get_name (structure));
        gst_structure_foreach (structure, print_field, (gpointer) pfx);
    }
}

/* Shows the CURRENT capabilities of the requested pad in the given element */
static void print_pad_capabilities (GstElement *element, gchar *pad_name) {
    GstPad *pad = NULL;
    GstCaps *caps = NULL;

    /* Retrieve pad */
    pad = gst_element_get_static_pad (element, pad_name);
    if (!pad) {
        g_printerr ("Could not retrieve pad '%s'\n", pad_name);
        return;
    }

    /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
    caps = gst_pad_get_current_caps (pad);
    if (!caps) caps = gst_pad_query_caps (pad, NULL);

    /* Print and free */
    g_print ("Caps for the %s pad:\n", pad_name);
    print_caps (caps, "      ");
    gst_caps_unref (caps);
    gst_object_unref (pad);
}

/* This function will be called by the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data) {
    GstPad *sink_pad = gst_element_get_static_pad (data->depay, "sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked (sink_pad)) {
        g_print ("We are already linked. Ignoring.\n");
        goto exit;
    }

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps (new_pad);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);
    if (!g_str_has_prefix (new_pad_type, "application/x-rtp")) {
        g_print ("It has type '%s' which is not application/x-rtp. Ignoring.\n", new_pad_type);
        goto exit;
    }

    /* Attempt the link */
    ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
        g_print ("Type is '%s' but link failed.\n", new_pad_type);
    } else {
        g_print ("Link succeeded (type '%s').\n", new_pad_type);
    }

exit:
    if (new_pad_caps != NULL) gst_caps_unref (new_pad_caps);
    gst_object_unref (sink_pad);
}

static void print_tag (const GstTagList * list, const gchar * tag, gpointer unused)
{
    gint i, count;
    count = gst_tag_list_get_tag_size (list, tag);
    for (i = 0; i < count; i++) {
        gchar *str;
        if (gst_tag_get_type (tag) == G_TYPE_STRING) {
            if (!gst_tag_list_get_string_index (list, tag, i, &str)) g_assert_not_reached ();
        } else {
            str = g_strdup_value_contents (gst_tag_list_get_value_index (list, tag, i));
        }
        if (i == 0) {
            g_print ("  %15s: %s\n", gst_tag_get_nick (tag), str);
        } else {
            g_print ("                 : %s\n", str);
        }
        g_free (str); 
    }
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
                exit(-3);
            }
            break;
        case G_IO_STATUS_NORMAL: {
            int index = g_ascii_strtoull (retString, NULL, 0);
            if (index < 0 || index >= 8) {
              g_printerr ("Index out of bounds\n");
            } else {
              /* If the input was a valid audio stream index, set the current audio stream */
              g_print ("You presses %d, adding 10 becomes %d\n", index, Summy(index, 10));
            }
            break;
        }
        default:
            break;
    }

    g_free (retString);
    return TRUE;
}

static void cb_message (GstBus *bus, GstMessage *msg, CustomData *data) {

    // https://gstreamer.freedesktop.org/documentation/design/messages.html
    // https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstMessage.html
    switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_EOS:
            /* end-of-stream */
            g_print ("End-Of-Stream message [%s][%s] received.\n", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg));
            gst_element_set_state (data->pipeline, GST_STATE_READY);
            g_main_loop_quit (data->loop);
            break;

        case GST_MESSAGE_ERROR: {
            GError *err = NULL;
            gchar *dbg_info = NULL;

            gst_message_parse_error (msg, &err, &dbg_info);
            g_printerr ("ERROR from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_printerr ("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
            g_error_free (err);
            g_free (dbg_info);
            gst_element_set_state (data->pipeline, GST_STATE_READY);
            g_main_loop_quit (data->loop);
            break;
        }

        case GST_MESSAGE_WARNING: {
            GError *err = NULL;
            gchar *dbg_info = NULL;

            gst_message_parse_warning (msg, &err, &dbg_info);
            g_print ("Warning from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_print ("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
            g_error_free (err);
            g_free (dbg_info);
            break;
        }

        case GST_MESSAGE_INFO: {
            GError *err = NULL;
            gchar *dbg_info = NULL;

            gst_message_parse_info (msg, &err, &dbg_info);
            g_print ("Info from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_print ("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
            g_error_free (err);
            g_free (dbg_info);
            break;
        }

        case GST_MESSAGE_TAG: {
            GstTagList *tags = NULL;

            gst_message_parse_tag (msg, &tags);
            g_print ("Tags from element %s\n", GST_OBJECT_NAME (msg->src));
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
                    g_print ("Progress Start [%s][%s]\n", category, text);
                    break;
                case GST_PROGRESS_TYPE_CONTINUE:
                    g_print ("Progress Continue [%s][%s]\n", category, text);
                    break;
                case GST_PROGRESS_TYPE_COMPLETE:
                    g_print ("Progress Complete [%s][%s]\n", category, text);
                    break;
                case GST_PROGRESS_TYPE_CANCELED:
                    g_print ("Progress Cancelled [%s][%s]\n", category, text);
                    break;
                case GST_PROGRESS_TYPE_ERROR:
                    g_print ("Progress Error [%s][%s]\n", category, text);
                    break;
                default:
                    g_print ("Undefined progress type.\n");
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
            g_print ("Buffering (%3d%%)\r", percent);
            /* Wait until buffering is complete before start/resume playing */
            if (percent < 100)
                gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
            else
                gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
            break;
        }

        case GST_MESSAGE_CLOCK_LOST:
            /* Get a new clock */
            g_print ("Clock lost message [%s][%s] received.\n", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg));
            gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
            gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
            break;

        case GST_MESSAGE_STREAM_STATUS: {
            GstStreamStatusType type;
            GstElement *owner;
            gchar *path_source, *path_owner;
            const GValue *val;
            GstTask *task = NULL;

            //g_print ("Stream Status message [%s][%s] received.\n", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg));
            gst_message_parse_stream_status (msg, &type, &owner);
            path_source = gst_object_get_path_string (GST_MESSAGE_SRC (msg));
            path_owner = gst_object_get_path_string (GST_OBJECT (owner));
            g_print ("type: %d    source: %s\n", type, path_source);
            g_print ("           owner:  %s\n", path_owner);
            g_free (path_source);
            g_free (path_owner);
            val = gst_message_get_stream_status_object (msg);
            //g_print ("object: type %s, value %p\n", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
            /* see if we know how to deal with this object */
            if (G_VALUE_TYPE (val) == GST_TYPE_TASK) task = g_value_get_object (val);
            switch (type) {
                case GST_STREAM_STATUS_TYPE_CREATE:
                    g_print ("Stream status CREATE, created task %p, type %s, value %p\n", task, G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                case GST_STREAM_STATUS_TYPE_ENTER:
                    g_print ("Stream status ENTER, type %s, value %p\n", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    /* setpriority (PRIO_PROCESS, 0, -10); */
                    break;
                case GST_STREAM_STATUS_TYPE_LEAVE:
                    g_print ("Stream status LEAVE, type %s, value %p\n", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                case GST_STREAM_STATUS_TYPE_DESTROY:
                    g_print ("Stream status DESTROY, type %s, value %p\n", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                case GST_STREAM_STATUS_TYPE_START:
                    g_print ("Stream status START, type %s, value %p\n", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                case GST_STREAM_STATUS_TYPE_PAUSE:
                    g_print ("Stream status PAUSE, type %s, value %p\n", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                case GST_STREAM_STATUS_TYPE_STOP:
                    g_print ("Stream status STOP, type %s, value %p\n", G_VALUE_TYPE_NAME (val), g_value_get_object (val));
                    break;
                default:
                    g_print ("Stream status unknown as default is hit.\n");
                    break;
            }
            break;
        }

        case GST_MESSAGE_STREAM_START:
            g_print ("Stream Start message [%s][%s] received.\n", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg));
            break;

        case GST_MESSAGE_ELEMENT:
            g_print ("Element message [%s][%s][%s] received.\n", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg), gst_structure_get_name (gst_message_get_structure (msg)));
            if (strcmp("splitmuxsink-fragment-opened", gst_structure_get_name (gst_message_get_structure (msg))) == 0){
                //GST_LOG ("structure is %" GST_PTR_FORMAT, gst_message_get_structure (msg));
                //g_print ("closed file message: [%s]\n", gst_structure_to_string (gst_message_get_structure (msg)));
                g_print ("Capture filename just opened is: [%s]\n", gst_structure_get_string (gst_message_get_structure (msg), "location"));
            }
            if (strcmp("splitmuxsink-fragment-closed", gst_structure_get_name (gst_message_get_structure (msg))) == 0){
                //GST_LOG ("structure is %" GST_PTR_FORMAT, gst_message_get_structure (msg));
                //g_print ("closed file message: [%s]\n", gst_structure_to_string (gst_message_get_structure (msg)));
                g_print ("Capture filename just closed is: [%s]\n", gst_structure_get_string (gst_message_get_structure (msg), "location"));
            }
            break;

        case GST_MESSAGE_STATE_CHANGED:
            /* We are only interested in state-changed messages from the pipeline */
            if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                g_print ("\nPipeline state changed from %s to %s:\n", gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
                /* Print the current capabilities of the sink element */
                //print_pad_capabilities (data->convert, "sink");
                //print_pad_capabilities (data->encoder, "sink");
                //print_pad_capabilities (data->parser, "sink");
                //print_pad_capabilities (data->splitsink, "video");
            }
            break;

        case GST_MESSAGE_LATENCY:
            g_print ("Recalculate latency\n");
            gst_bin_recalculate_latency ( GST_BIN (data->pipeline));
            break;

        case GST_MESSAGE_QOS: {
            gboolean live;
            guint64 running_time, stream_time, timestamp, duration;

            gst_message_parse_qos (msg, &live, &running_time, &stream_time, &timestamp, &duration);
            g_print ("QoS info: [%s]", (live) ? "LIVE" : "Not live");
            g_print (", rt [%" G_GUINT64_FORMAT "]", running_time);
            g_print (", st [%" G_GUINT64_FORMAT "]", stream_time);
            g_print (", ts [%" G_GUINT64_FORMAT "]", timestamp);
            g_print (", dur [%" G_GUINT64_FORMAT "]\n", duration);
            break;
        }

        default:
            /* Unhandled message */
            g_printerr ("Unhandled message [%s][%s] received.\n", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_SRC_NAME (msg));
            break;
    }
}

static GstPadProbeReturn pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
    GstPad *sinkpad;

    g_print ("--- pad is blocked now ---\n");

    /* remove the probe first */
    //gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

    /* Send the EOS signal to decently close the capture files */
    sinkpad = gst_element_get_static_pad (GST_ELEMENT (user_data), "sink");
    gst_pad_send_event (sinkpad, gst_event_new_eos ());
    gst_object_unref (sinkpad);

    //return GST_PAD_PROBE_OK;
    return GST_PAD_PROBE_REMOVE;
}

/* This function is called periodically */
static gboolean timer_expired (CustomData *data) {
    data->timer_expired = TRUE;
    gst_pad_add_probe (data->blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, pad_probe_cb, data->decoder, NULL);
    return FALSE; /* Otherwise the callback will continue */
}

/* This function is called periodically */
static gboolean watch_mainloop_timer_expired (CustomData *data) {
    if (user_interrupt) {
        /* Add a pad probe to the src pad and block the downstream */
        /* Once the stream is blocked send a EOS signal to decently close the capture files */
        gst_pad_add_probe (data->blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, pad_probe_cb, data->decoder, NULL);
    }
    return TRUE; /* Otherwise the callback will be cancelled */
}

void handle_interrupt_signal(int signal) {
    switch (signal) {
        case SIGHUP:
            g_print ("--- Caught SIGHUP, ---\n");
            break;
        case SIGUSR1:
            g_print ("--- Caught SIGUSR1, ---\n");
            break;
        case SIGINT:
            g_print ("--- Caught SIGINT, ---\n");
            user_interrupt = TRUE;
            break;
        default:
            g_print ("Caught signal: %d\n", signal);
            break;
    }
}

int main(int argc, char *argv[]) {
    CustomData data;
    GstBus *bus;
    GstCaps *caps;
    GstStateChangeReturn ret;
    GIOChannel *io_channel;
    struct sigaction sa;
    int retries = 0;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Initialize our data structure */
    memset (&data, 0, sizeof (data));

    if (argc > 3){
        g_print("URI [%s]\n", argv[1]);
        g_print("user-name [%s]\n", argv[2]);
        g_print("password [%s]\n", argv[3]);
    } else {
        g_print("To little parameters. Please supply URI, user name and password.\n");
        exit(-2);
    }

    // Setup the signal handler
    sa.sa_handler = &handle_interrupt_signal;
    sa.sa_flags = SA_RESTART;                    // Restart the system call, if at all possible
    sigfillset(&sa.sa_mask);                     // Block every signal during the handler
    if (sigaction(SIGINT, &sa, NULL) == -1) {    // Intercept SIGINT
        g_printerr ("Error: cannot handle SIGINT");
    }
    if (sigaction(SIGHUP, &sa, NULL) == -1) {    // Intercept SIGHUP
        g_printerr("Error: cannot handle SIGHUP");
    }
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {   // Intercept SIGUSR1
        g_printerr("Error: cannot handle SIGUSR1");
    }

    /* Register a function that GLib will call every x seconds */
    g_timeout_add_seconds (1, (GSourceFunc)watch_mainloop_timer_expired, &data);

    while ((!user_interrupt) && (retries < maxRetries)) {
        data.timer_expired = FALSE;

        /* Create the elements */
        data.source = gst_element_factory_make ("rtspsrc", "source");
        data.depay = gst_element_factory_make ("rtph264depay", "depay");
        data.decoder = gst_element_factory_make ("avdec_h264", "decoder");
        data.convert = gst_element_factory_make ("videoconvert", "convert");
        data.scale = gst_element_factory_make ("videoscale", "scale");
        data.encoder = gst_element_factory_make("x264enc", "encoder");
        data.parser = gst_element_factory_make("h264parse", "parser");
        data.muxer = gst_element_factory_make("mp4mux", "muxer"); // matroskamux or avimux or mp4mux
        data.sink = gst_element_factory_make ("autovideosink", "sink");
        data.splitsink = gst_element_factory_make ("splitmuxsink", "splitsink");

        /* Attach a blockpad to this element to be able to stop the pipeline dataflow decently */
        data.blockpad = gst_element_get_static_pad (data.depay, "src");

        /* Create the empty pipeline */
        data.pipeline = gst_pipeline_new ("my-pipeline");

        if (!data.pipeline || !data.source || !data.depay || !data.decoder || !data.convert || !data.scale || !data.encoder || !data.parser || !data.muxer || !data.sink || !data.splitsink) {
            g_printerr ("Not all elements could be created.\n");
            return -1;
        }

        /* Build the pipeline. */
        gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.depay, data.decoder, data.convert, data.scale, data.encoder, data.parser, data.splitsink, NULL);

        /* Note that we are NOT linking the source at this point. We will do it later. */
        if (!gst_element_link_many (data.depay, data.decoder, data.convert, data.scale, NULL)) {
            g_printerr ("Element link problem.\n");
            gst_object_unref (data.pipeline);
            return -1;
        }

        /* Negotiate caps */
        //caps = gst_caps_from_string("video/x-raw, format=(string)RGB, width=320, height=240, framerate=(fraction)30/1");
        // https://en.wikipedia.org/wiki/Display_resolution
        // https://en.wikipedia.org/wiki/Display_resolution#/media/File:Vector_Video_Standards8.svg
        //caps = gst_caps_from_string("video/x-raw, width=1280, height=720"); // Change scale from HD 1080 to HD 720
        caps = gst_caps_from_string("video/x-raw, width=1024, height=576"); // Change scale from HD 1080 to PAL
        if (!gst_element_link_filtered (data.scale, data.encoder, caps)) {
            g_printerr ("Element scale could not be linked to element tee.\n");
            gst_object_unref (data.pipeline);
            return -1;
        }
        gst_caps_unref (caps);

        /* Link the rest of the pipeline */
        if (!gst_element_link_many (data.encoder, data.parser, data.splitsink, NULL)) {
            g_printerr ("Rest of elements link problem.\n");
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

        g_object_set (data.splitsink, "location", "/home/harm/github/ip-cam/rec%03d.mp4", NULL);
        g_object_set (data.splitsink, "max-size-time", (60 * GST_SECOND), NULL); // in nanoseconds so 10 seconds
        g_object_set (data.splitsink, "max-size-bytes", (50 * 10485760), NULL); // in bytes so 50 MBytes
        g_object_set (data.splitsink, "max-files", 25, NULL);
        g_object_set (data.splitsink, "muxer", data.muxer, NULL);

        /* Connect to the pad-added signal */
        g_signal_connect (data.source, "pad-added", G_CALLBACK (pad_added_handler), &data);

        /* Start playing the pipeline */
        g_print ("---> Go to PLAYING, retries is [%d/%d]\n", retries, maxRetries);
        ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr ("Unable to set the pipeline to the playing state.\n");
            gst_object_unref (data.pipeline);
            return -1;
        } else if (ret == GST_STATE_CHANGE_NO_PREROLL) {
            data.is_live = TRUE;
            g_print ("Live stream.....\n");
        } else {
            g_print ("Not a live stream.....\n");
        }

        g_timeout_add_seconds (75, (GSourceFunc)timer_expired, &data);

        /* Listen to the bus */
        bus = gst_element_get_bus (data.pipeline);
        gst_bus_add_signal_watch (bus);
        g_signal_connect (bus, "message", G_CALLBACK (cb_message), &data);

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

        retries++;
        if (!user_interrupt)
        {
            g_print ("Sleepy.....\n");
            sleep (40);
        }
    }

    return 0;
}
