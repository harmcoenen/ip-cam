#ifndef IPCAMFTP_H
#define IPCAMFTP_H

static size_t read_callback (void *ptr, size_t size, size_t nmemb, void *stream);
int ftp_upload_file (const char *pathfilename, const char *filename, const char *usrpwd);
int ftp_upload_files (const char *path_with_uploads, const char *remote_dir, const char *usrpwd);

#endif /* IPCAMFTP_H */