#ifndef IPCAMPRINTING_H
#define IPCAMPRINTING_H

/* Functions below print the Capabilities in a human-friendly format */
gboolean print_field(GQuark field, const GValue * value, gpointer pfx);
void print_caps(const GstCaps * caps, const gchar * pfx);
void print_pad_capabilities(GstElement *element, gchar *pad_name);
void print_tag(const GstTagList * list, const gchar * tag, gpointer unused);

#endif /* IPCAMPRINTING_H */