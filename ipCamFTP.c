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
#include "ipCamFTP.h"

/* ftp.familiecoenen.nl port 21, /public/www/recordings */
const char *remote_url = "ftp://ftp.familiecoenen.nl/";

/* NOTE: if you want this example to work on Windows with libcurl as a
   DLL, you MUST also provide a read callback with CURLOPT_READFUNCTION.
   Failing to do so will give you a crash since a DLL may not use the
   variable's memory when passed in to it from an app like this. */ 
static size_t read_callback (void *ptr, size_t size, size_t nmemb, void *stream) {
  curl_off_t nread;
  /* in real-world cases, this would probably get this data differently
     as this fread() stuff is exactly what the library already would do
     by default internally */ 
  size_t retcode = fread (ptr, size, nmemb, stream);
 
  nread = (curl_off_t)retcode;
 
  fprintf (stderr, "\nRead %" CURL_FORMAT_CURL_OFF_T " bytes from file.", nread);
  return retcode;
}
 
int ftp_upload_file (const char *pathfilename, const char *filename, const char *usrpwd) {
    CURL *curl;
    CURLcode res;
    FILE *hd_src;
    struct stat file_info;
    curl_off_t fsize;

    struct curl_slist *headerlist = NULL;
    static const char buf_1 [] = "RNFR uploadfile.txt";
    static const char buf_2 [] = "RNTO renamedfile.txt";
    static char remote_url_and_file[60];

    strcpy (remote_url_and_file, remote_url); strcat (remote_url_and_file, filename);

    /* get the file size of the local file */ 
    if (stat (pathfilename, &file_info)) {
        printf ("\nCouldn't open '%s': %s", pathfilename, strerror(errno));
        return 1;
    }
    fsize = (curl_off_t)file_info.st_size;

    printf ("\nLocal file size: %" CURL_FORMAT_CURL_OFF_T " bytes.", fsize);

    /* get a FILE * of the same file */ 
    hd_src = fopen(pathfilename, "rb");

    /* In windows, this will init the winsock stuff */ 
    curl_global_init (CURL_GLOBAL_ALL);

    /* get a curl handle */ 
    curl = curl_easy_init ();
    if (curl) {
        /* build a list of commands to pass to libcurl */ 
        headerlist = curl_slist_append (headerlist, buf_1);
        headerlist = curl_slist_append (headerlist, buf_2);

        /* we want to use our own read function */ 
        curl_easy_setopt (curl, CURLOPT_READFUNCTION, read_callback);

        /* enable uploading */ 
        curl_easy_setopt (curl, CURLOPT_UPLOAD, 1L);

        /* specify target */ 
        curl_easy_setopt (curl, CURLOPT_URL, remote_url_and_file);
        curl_easy_setopt (curl, CURLOPT_USERPWD, usrpwd);

        /* pass in that last of FTP commands to run after the transfer */ 
        //curl_easy_setopt (curl, CURLOPT_POSTQUOTE, headerlist);

        /* now specify which file to upload */ 
        curl_easy_setopt (curl, CURLOPT_READDATA, hd_src);

        /* We activate SSL and we require it for both control and data */
        // curl_easy_setopt (curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

        /* Set the size of the file to upload (optional).  If you give a *_LARGE
           option you MUST make sure that the type of the passed-in argument is a
           curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
           make sure that to pass in a type 'long' argument. */ 
        curl_easy_setopt (curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);

        /* Enable verbose logging */
        //curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt (curl, CURLOPT_VERBOSE, 0);

        /* Now run off and do what you've been told! */ 
        res = curl_easy_perform (curl);
        /* Check for errors */ 
        if (res != CURLE_OK) fprintf (stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror (res));

        /* clean up the FTP commands list */ 
        curl_slist_free_all (headerlist);

        /* always cleanup */ 
        curl_easy_cleanup (curl);
    }

    fclose (hd_src); /* close the local file */ 
    curl_global_cleanup ();
    return 0;
}