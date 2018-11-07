#ifndef IPCAMFTP_H
#define IPCAMFTP_H

/* ftp.familiecoenen.nl port 21, /public/www/recordings */
static const char *remote_url = "ftp://ftp.familiecoenen.nl/";

static size_t read_callback (void *ptr, size_t size, size_t nmemb, void *stream);
int ftp_upload_file (const char *pathfilename, const char *filename, const char *usrpwd);
int ftp_upload_files (const char *path_with_uploads, const char *remote_dir, const char *usrpwd);
int ftp_list_directory (const char *remote_dir, const char *usrpwd); /* check out https://curl.haxx.se/libcurl/c/getinmemory.html */
int ftp_remove_directory (const char *remote_dir, const char *usrpwd);

#endif /* IPCAMFTP_H */