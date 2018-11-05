#include <gst/gst.h>
#include "ipCamPrinting.h"

/* Functions below print the Capabilities in a human-friendly format */
gboolean print_field (GQuark field, const GValue * value, gpointer pfx) {
    gchar *str = gst_value_serialize (value);

    GST_INFO ("%s  %15s: %s", (gchar *) pfx, g_quark_to_string (field), str);
    g_free (str);
    return TRUE;
}

void print_caps (const GstCaps * caps, const gchar * pfx) {
    guint i;

    g_return_if_fail (caps != NULL);

    if (gst_caps_is_any (caps)) {
        GST_INFO ("%sANY", pfx);
        return;
    }
    if (gst_caps_is_empty (caps)) {
        GST_INFO ("%sEMPTY", pfx);
        return;
    }

    for (i = 0; i < gst_caps_get_size (caps); i++) {
        GstStructure *structure = gst_caps_get_structure (caps, i);

        GST_INFO ("%s%s", pfx, gst_structure_get_name (structure));
        gst_structure_foreach (structure, print_field, (gpointer) pfx);
    }
}

/* Shows the CURRENT capabilities of the requested pad in the given element */
void print_pad_capabilities (GstElement *element, gchar *pad_name) {
    GstPad *pad = NULL;
    GstCaps *caps = NULL;

    /* Retrieve pad */
    pad = gst_element_get_static_pad (element, pad_name);
    if (!pad) {
        GST_ERROR ("Could not retrieve pad '%s'", pad_name);
        return;
    }

    /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
    caps = gst_pad_get_current_caps (pad);
    if (!caps) caps = gst_pad_query_caps (pad, NULL);

    /* Print and free */
    GST_INFO ("Caps for the %s pad:", pad_name);
    print_caps (caps, "      ");
    gst_caps_unref (caps);
    gst_object_unref (pad);
}

void print_tag (const GstTagList * list, const gchar * tag, gpointer unused)
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
            GST_INFO ("  %15s: %s", gst_tag_get_nick (tag), str);
        } else {
            GST_INFO ("                 : %s", str);
        }
        g_free (str); 
    }
}
