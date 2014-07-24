/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * Minipedia - Web Service Task for MediaWiki application
 * ======================================================
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "webservice.h"

DUDA_REGISTER("Minipedia", "API Example");

int latest_id;
char *store_path;
pthread_mutex_t mutex_article_id;

int lazy_fibonacci(int n)
{
    if (n == 0) {
        return 0;
    }
    else if (n == 1) {
        return 1;
    }
    else {
        return (lazy_fibonacci(n-1) + lazy_fibonacci(n-2));
    }
}

/* It returns the latest plane crash report file path that can be served */
static char *get_latest_plane_crash(int *id)
{
    int max_path = 1024;
    char *path;

    path = mem->alloc(max_path);
    if (!path) {
        return NULL;
    }

    pthread_mutex_lock(&mutex_article_id);
    *id = latest_id;
    pthread_mutex_unlock(&mutex_article_id);

    snprintf(path, max_path - 1, "%s/%i", store_path, *id);
    return path;
}

/* Home page */
void cb_home(duda_request_t *dr)
{
    response->http_status(dr, 200);
    response->printf(dr, "MiniPedia Example\n");
    response->end(dr, NULL);
}

/* It store a new document on latest_plane_crash */
void cb_latest_plane_crash_update(duda_request_t *dr)
{
    int fd;
    int bytes;
    int new_id;
    int size = 1024;
    unsigned long data_length;
    char *path;
    char *content;
    char *header_version;

    /* Just allow requests with incoming data */
    if (request->is_data(dr) == MK_FALSE) {
        goto error;
    }

    /* Critical section, this needs a different approach */
    pthread_mutex_lock(&mutex_article_id);

    /* Compose path */
    new_id = latest_id + 1;

    path = mem->alloc(size);
    snprintf(path, size - 1, "%s/%i",
             store_path, new_id);

    /* Create new article file */
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    content = request->get_data(dr, &data_length);
    bytes = write(fd, content, data_length);
    if (bytes < 0) {
        perror("write");
    }

    close(fd);

    gc->add(dr, data);
    latest_id++;
    pthread_mutex_unlock(&mutex_article_id);

    /* Header */
    header_version = mem->alloc(size);
    snprintf(header_version, size - 1, "X-Minipedia-Version-Id: %i", new_id);
    gc->add(dr, header_version);

    /* Compose response */
    response->http_status(dr, 200);
    response->http_header(dr, header_version);
    response->printf(dr, "Document updated: %i bytes written\n", bytes);
    response->end(dr, NULL);

 error:
    response->http_status(dr, 500);
    response->printf(dr, "You are not using the API correctly\n");
    response->end(dr, NULL);
}


void cb_latest_plane_crash(duda_request_t *dr)
{
    int id;
    int header_size = 64;
    char *doc;
    char *header_version;

    lazy_fibonacci(34);

    /* Get the latest document that can be send */
    doc = get_latest_plane_crash(&id);

    /*
     * Just a fanzy HTTP header to let the client now which version of the
     * article is getting.
     */
    header_version = mem->alloc(header_size);
    snprintf(header_version, header_size - 1, "X-Minipedia-Version-Id: %i", id);

    /* Register the buffers into the garbage collector */
    gc->add(dr, doc);
    gc->add(dr, header_version);

    /* Compose the response and let the magic non-blocking work */
    response->http_status(dr, 200);
    response->http_header(dr, header_version);
    response->http_content_type(dr, "html");
    response->sendfile(dr, doc);
    response->end(dr, NULL);
}

/* Main function, lets configure our service */
int duda_main()
{
    store_path = mem->alloc(1024);
    snprintf(store_path, 1024, "%s/plane_crash/", data->get_path());

    /* Make the service be root */
    conf->service_root();

    latest_id = 1;

    /* Set API callbacks */
    map->static_root("cb_home");
    map->static_add("/Latest_plane_crash/update", "cb_latest_plane_crash_update");
    map->static_add("/Latest_plane_crash", "cb_latest_plane_crash");

    return 0;
}
