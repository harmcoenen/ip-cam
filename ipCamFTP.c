#include <gst/gst.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <dirent.h>
#include "ipCamFTP.h"
#include "ipCamCapture.h"

GST_DEBUG_CATEGORY_EXTERN (ipcam);
#define GST_CAT_DEFAULT ipcam

static size_t read_callback (void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t retcode = fread (ptr, size, nmemb, stream);
    return retcode;
}
 
int ftp_upload_file (const char *pathfilename, const char *filename, const char *usrpwd) {
    CURL *curl;
    CURLcode res = 1; /* By default expect curl to fail */
    FILE *hd_src;
    struct stat file_info;
    curl_off_t fsize;

    static char remote_url_and_file[PATH_MAX];

    strcpy (remote_url_and_file, remote_url); strcat (remote_url_and_file, filename);

    /* get the file size of the local file */ 
    if (stat (pathfilename, &file_info)) {
        GST_ERROR ("Couldn't open '%s': %s", pathfilename, strerror (errno));
        return 1;
    }
    fsize = (curl_off_t)file_info.st_size;

    GST_DEBUG ("Local file size: %" CURL_FORMAT_CURL_OFF_T " bytes.", fsize);

    /* get a FILE * of the same file */ 
    hd_src = fopen (pathfilename, "rb");

    /* In windows, this will init the winsock stuff */ 
    curl_global_init (CURL_GLOBAL_ALL);

    /* get a curl handle */ 
    curl = curl_easy_init ();
    if (curl) {
        /* we want to use our own read function */ 
        curl_easy_setopt (curl, CURLOPT_READFUNCTION, read_callback);

        /* enable uploading */ 
        curl_easy_setopt (curl, CURLOPT_UPLOAD, 1L);

        /* specify target */ 
        curl_easy_setopt (curl, CURLOPT_URL, remote_url_and_file);

        /* Set username and password */
        curl_easy_setopt (curl, CURLOPT_USERPWD, usrpwd);

        /* now specify which file to upload */ 
        curl_easy_setopt (curl, CURLOPT_READDATA, hd_src);

        /* Set the size of the file to upload (optional).  If you give a *_LARGE
           option you MUST make sure that the type of the passed-in argument is a
           curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
           make sure that to pass in a type 'long' argument. */ 
        curl_easy_setopt (curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);

        /* Enable verbose logging */
        curl_easy_setopt (curl, CURLOPT_VERBOSE, 0L);

        /* Enable progress meter */
        curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1L);

        curl_easy_setopt (curl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR);
        curl_easy_setopt (curl, CURLOPT_NEW_DIRECTORY_PERMS, 0755L); /* Default is 0755 */
        curl_easy_setopt (curl, CURLOPT_NEW_FILE_PERMS, 0644L); /* Default is 0644 */

        /* Perform the custom request */ 
        res = curl_easy_perform (curl);
        /* Check for errors */ 
        if (res == CURLE_OK) {
            /* remove file if upload successfull */
            if (remove (pathfilename) == 0) {
                GST_INFO ("File [%s] deleted successfully", pathfilename);
            } else {
                GST_ERROR ("Unable to delete file [%s]", pathfilename);
            }
        } else {
            GST_ERROR ("curl_easy_perform() failed: %s", curl_easy_strerror (res));
            //fprintf (stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror (res));
        }

        /* always cleanup */ 
        curl_easy_cleanup (curl);
    }

    fclose (hd_src); /* close the local file */ 
    curl_global_cleanup ();
    return (res);
}

int ftp_upload_files (const char *path_with_uploads, const char *remote_dir, const char *usrpwd) {
    CURL *curl;
    CURLcode res;
    FILE *hd_src;
    struct stat file_info;
    curl_off_t fsize;
    DIR *dr = NULL;
    struct dirent *de;
    int n_uploaded_files = 0;

    static char remote_url_and_file[PATH_MAX];
    static char local_path_and_file[PATH_MAX];

    dr = opendir (path_with_uploads); 
    if (dr == NULL) {
        GST_ERROR ("Could not open directory of files to upload.");
        return -1;
    }

    /* In windows, this will init the winsock stuff */ 
    curl_global_init (CURL_GLOBAL_ALL);

    /* get a curl handle */ 
    curl = curl_easy_init ();
    if (curl) {
        curl_easy_setopt (curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt (curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt (curl, CURLOPT_USERPWD, usrpwd);
        curl_easy_setopt (curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt (curl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR);
        curl_easy_setopt (curl, CURLOPT_NEW_DIRECTORY_PERMS, 0755L); /* Default is 0755 */
        curl_easy_setopt (curl, CURLOPT_NEW_FILE_PERMS, 0644L); /* Default is 0644 */

        while ((de = readdir (dr)) != NULL) {
            if ((strcmp (".", de->d_name) != 0) && (strcmp ("..", de->d_name) != 0)) {
                strcpy (remote_url_and_file, remote_url);
                strcat (remote_url_and_file, remote_dir);
                strcat (remote_url_and_file, "/");
                strcat (remote_url_and_file, de->d_name);
                strcpy (local_path_and_file, path_with_uploads);
                strcat (local_path_and_file, "/");
                strcat (local_path_and_file, de->d_name);
                GST_INFO ("Going to upload [%s] to [%s]", local_path_and_file, remote_url_and_file);
                /* specify target */ 
                curl_easy_setopt (curl, CURLOPT_URL, remote_url_and_file);
                /* get the file size of the local file */ 
                if (stat (local_path_and_file, &file_info)) {
                    GST_ERROR ("Couldn't open '%s': %s", local_path_and_file, strerror (errno));
                    curl_easy_cleanup (curl); curl_global_cleanup (); closedir (dr); /* always cleanup */
                    return -1;
                }
                fsize = (curl_off_t)file_info.st_size;
                GST_DEBUG ("Local file size: %" CURL_FORMAT_CURL_OFF_T " bytes.", fsize);
                /* get a FILE * of the same file */ 
                hd_src = fopen (local_path_and_file, "rb");
                /* now specify which file to upload */ 
                curl_easy_setopt (curl, CURLOPT_READDATA, hd_src);
                curl_easy_setopt (curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);

                /* Perform the custom request */ 
                res = curl_easy_perform (curl);
                /* Check for errors */ 
                if (res == CURLE_OK) {
                    n_uploaded_files++;
                    GST_INFO ("File [%s] uploaded successfully", local_path_and_file);
                    /* remove file if upload successfull */
                    if (remove (local_path_and_file) == 0) {
                        GST_INFO ("File [%s] deleted successfully", local_path_and_file);
                    } else {
                        GST_ERROR ("Unable to delete file [%s]", local_path_and_file);
                    }
                } else {
                    GST_ERROR ("curl_easy_perform() failed: %s", curl_easy_strerror (res));
                    //fprintf (stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror (res));
                }
                fclose (hd_src); /* close the local file */ 
            }
        }

        /* always cleanup */ 
        curl_easy_cleanup (curl);
    }

    curl_global_cleanup ();
    closedir (dr);
    return (n_uploaded_files);
}

int ftp_list_directory (const char *remote_dir, const char *usrpwd) {
    CURL *curl;
    CURLcode res = CURLE_OK;
    static char remote_url_and_file[PATH_MAX];

    /* In windows, this will init the winsock stuff */
    curl_global_init (CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init ();
    if (curl) {
        /* Set username and password */
        curl_easy_setopt (curl, CURLOPT_USERPWD, usrpwd);
         /* specify target */
        strcpy (remote_url_and_file, remote_url);
        strcat (remote_url_and_file, remote_dir);
        curl_easy_setopt (curl, CURLOPT_URL, remote_url_and_file);
        /* Set the custom request command */
        curl_easy_setopt (curl, CURLOPT_CUSTOMREQUEST, "NLST"); /* NLST or LIST */

        /* Perform the custom request */
        res = curl_easy_perform (curl);
        /* Check for errors */
        if (res == CURLE_OK) {
            GST_INFO ("List remote directory successful");
        } else {
            GST_ERROR ("curl_easy_perform() failed: %s", curl_easy_strerror (res));
            //fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror (res));
        }
 
        /* Always cleanup */
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup ();
    return (res);
}

int ftp_remove_directory (const char *remote_dir, const char *usrpwd) {
    CURL *curl;
    CURLcode res = CURLE_OK;
    static char remote_url_and_file[PATH_MAX];
    static char remove_cmd[50];

    /* In windows, this will init the winsock stuff */
    curl_global_init (CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init ();
    if (curl) {
        /* Set username and password */
        curl_easy_setopt (curl, CURLOPT_USERPWD, usrpwd);
         /* specify target */ 
        strcpy (remote_url_and_file, remote_url);
        strcpy (remove_cmd, "RMD ");
        strcat (remove_cmd, remote_dir);
        GST_WARNING ("Delete remote directory command: %s", remove_cmd);
        curl_easy_setopt (curl, CURLOPT_URL, remote_url_and_file);
        /* Set the DELETE command specifying the existing folder */
        curl_easy_setopt (curl, CURLOPT_CUSTOMREQUEST, remove_cmd);

        /* Perform the custom request */
        res = curl_easy_perform (curl);
        /* Check for errors */
        if (res == CURLE_OK) {
            GST_WARNING ("Delete remote directory '%s' successful", remote_dir);
        } else {
            GST_ERROR ("curl_easy_perform() failed: %s", curl_easy_strerror (res));
            //fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror (res));
        }
 
        /* Always cleanup */
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup ();
    return (res);
}