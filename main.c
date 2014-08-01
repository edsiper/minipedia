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

struct bundle {
    duda_dthread_channel_t *chan;
    duda_request_t *dr;
};

int latest_id;
char *store_path;
pthread_mutex_t mutex_article_id;

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

void consumer(void *data)
{
    int id;
    int header_size = 64;
    char *doc;
    char *header_version;

    struct bundle *bdl = data;
    duda_dthread_channel_t *chan = bdl->chan;
    duda_request_t *dr = bdl->dr;

    while (!dthread->chan_done(chan)) {
        int *n = dthread->chan_recv(chan);
        //response->printf(dr, "%d\n", *n);
        mem->free(n);
    }

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

void producer(void *data)
{
    duda_dthread_channel_t *chan = data;
    int num1 = 1;
    int num2 = 1;
    int i;

    for (i = 0; i < 34; ++i) {
        int *n = mem->alloc(sizeof(int));
        if (i == 0) {
            *n = num1;
        } else if (i == 1) {
            *n = num2;
        } else {
            *n = num1 + num2;
            num1 = num2;
            num2 = *n;
        }
        dthread->chan_send(chan, n);
    }
    dthread->chan_end(chan);
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

/*
 * This callback will take care to prepare the duda-threads (co-routines)
 * for Fibonacci calculation and then start processing the page content.
 */
void cb_latest_plane_crash(duda_request_t *dr)
{
    int cid;
    int pid;
    duda_dthread_channel_t *chan = dthread->chan_create(0, monkey->mem_free);
    struct bundle *bdl = monkey->mem_alloc(sizeof(*bdl));

    /* Calculate Fibonacci using new duda-threads */
    bdl->chan = chan;
    bdl->dr = dr;
    cid = dthread->create(consumer, bdl);
    pid = dthread->create(producer, chan);

    dthread->chan_set_sender(chan, pid);
    dthread->chan_set_receiver(chan, cid);
    dthread->resume(cid);
    dthread->chan_free(chan);

    monkey->mem_free(bdl);
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
