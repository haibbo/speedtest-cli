#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <float.h>
#include <math.h>

#include <curl/curl.h>

#define URL_LENGTH_MAX       255
#define THREAD_NUM_MAX       10
#define UPLOAD_EXT_LENGTH_MAX 5
#define SPEEDTEST_TIME_MAX   10
#define UPLOAD_EXTENSION_TAG "upload_extension"
#define LATENCY_TXT_URL "/speedtest/latency.txt"

#define INIT_DOWNLOAD_FILE_RESOLUTION 750
#define FILE_350_SIZE                 245388

#define OK  0
#define NOK 1

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))


struct thread_para
{
    pthread_t    tid;
    char         url[URL_LENGTH_MAX + 32];
    double       result;
    long         upload_size;
};

struct web_buffer
{
    char *data;
    int   size;
};

static int show_usage(char* argv[])
{
    printf("%s:\n\t\t -p number of threads\n" \
                "\t\t -l list all valid server\n" \
                "\t\t -s specify mini speedtest server url\n", argv[0]);
    exit(0);
}

static int calc_past_time(struct timeval* start, struct timeval* end)
{
    return (end->tv_sec - start->tv_sec) * 1000 + (end->tv_usec - start->tv_usec)/1000;
}

static size_t write_data(void* ptr, size_t size, size_t nmemb, void *stream)
{
    return size * nmemb;
}

static size_t write_web_buf(void* ptr, size_t size, size_t nmemb, void *data)
{
    struct web_buffer* buf = (struct web_buffer*)data;
    char *p = NULL;
    int length = buf->size + size*nmemb + 1;
    //printf("le = %p %p %d %d %d\n", p, buf->data, length,  size*nmemb, buf->size);
    p = (char*)realloc(buf->data, buf->size + size*nmemb + 1);
    if (p == NULL) {
        printf("realloc failed\n");
        return 0;
    }
    buf->data = p;
    memcpy(&buf->data[buf->size], ptr, size*nmemb);
    buf->size += size * nmemb;
    buf->data[buf->size] = 0;
    return size * nmemb;
}

static int do_latency(char *p_url)
{
    char latency_url[URL_LENGTH_MAX + sizeof(LATENCY_TXT_URL)] = {0};
    CURL *curl;
    CURLcode res;
    long response_code;
    
    curl = curl_easy_init();

    sprintf(latency_url, "%s%s", p_url, LATENCY_TXT_URL);
    
    curl_easy_setopt(curl, CURLOPT_URL, latency_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || response_code != 200) {

        printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)); 
        return NOK;
    }
    return OK;
}

static double test_latency(char *p_url)
{
    struct timeval s_time, e_time;
    double latency;

    gettimeofday(&s_time, NULL);
    if (do_latency(p_url) != OK)
        return DBL_MAX;
    gettimeofday(&e_time, NULL);

    latency = calc_past_time(&s_time, &e_time);
    return latency;
}

static void* do_download(void* data)
{
    CURL *curl;
    CURLcode res;
    struct thread_para* p_para = (struct thread_para*)data;
    double length = 0;
    double time = 0, time1 = 0, time2;
    
    curl = curl_easy_init();
    
    //printf("image url = %s\n", p_para->url);
    curl_easy_setopt(curl, CURLOPT_URL, p_para->url);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)); 
        curl_easy_cleanup(curl);
        return;
    }
    curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &length);
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &time);
    curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &time1);
    curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &time2);
    //printf("Length is %lf %lf %lf %lf\n", length, time, time1, time2);
    p_para->result = length;
    curl_easy_cleanup(curl);
    return;
}

static int test_download(char *p_url, int num_thread, int dsize, char init)
{
    struct timeval s_time, e_time;
    int time, i, error;
    struct thread_para paras[THREAD_NUM_MAX];
    double sum = 0;

    gettimeofday(&s_time, NULL);

    for ( i = 0; i < num_thread; i++) {

        sprintf(paras[i].url, "%s/speedtest/random%dx%d.jpg", p_url, dsize, dsize);
        paras[i].result = 0;
        error = pthread_create(&paras[i].tid, NULL, do_download, (void*)&paras[i]);

        if ( error != 0)
            printf("Can't Run thread num %d, error %d\n", i, error);
    }
    if (init != 0)
    {
        char running = 0;

        do {
            usleep(400*1000);
            running = 0;
            for (i = 0;i < num_thread; i++) {
                if (paras[i].result == 0)
                    running = 1;
            }
            fprintf(stderr, ".");
        }while(running);
       fprintf(stderr, "\n");
    }
    for (i = 0;i < num_thread; i++) {
        pthread_join(paras[i].tid, NULL);
        sum += paras[i].result;
    }
    gettimeofday(&e_time, NULL);

    time = calc_past_time(&s_time, &e_time);

    //printf("msec = %d speed = %0.2fMbps\n", time, ((sum*8*1000/time))/(1024*1024));
    return (sum*1000)/time;
}

static size_t read_data(void* ptr, size_t size, size_t nmemb, void *userp)
{
    struct  thread_para* para = (struct thread_para*)userp;
    int     length;
    char    data[16284] = {0};

    if (size * nmemb < 1 && para->upload_size)
        return 0;
#if 0
    if (para->data == NULL) {
        int i;

        para->data = (char*)malloc(10240);
        para->data[0] = 'A';
        for ( i = 1;i < 10240; i++) {

            if (para->data[i - 1] + 1 > 'Z')
                para->data[i - 1] = 'A';
            para->data[i] = para->data[i - 1] + 1;
         
        }
    }
#endif
    if (para->upload_size > size * nmemb) {
        
        length = size * nmemb < 16284 ? size*nmemb : 16284;
    }
    else
        length = para->upload_size < 16284 ? para->upload_size : 16284;
    memcpy(ptr, data, length);

    para->upload_size -= length;

    return  length;
}

static int do_upload(struct thread_para* para)
{
    char upload_url[URL_LENGTH_MAX + sizeof(LATENCY_TXT_URL)] = {0};
    CURL *curl;
    CURLcode res;
    int i;
    double size_upload;

    curl = curl_easy_init();

    
    curl_easy_setopt(curl, CURLOPT_URL, para->url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_data);
    curl_easy_setopt(curl, CURLOPT_READDATA, para);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE , (curl_off_t)para->upload_size);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)); 
        curl_easy_cleanup(curl);
        return NOK;
    }
    curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, &size_upload);
    para->result = size_upload;
    //printf("size upload = %lf\n", size_upload);
    curl_easy_cleanup(curl);
    return OK;
}

static int test_upload(char *p_url, int num_thread, long speed, char *p_ext, char init)
{
    struct timeval s_time, e_time;
    int time;
    int i, error;
    struct thread_para paras[THREAD_NUM_MAX];
    double sum = 0;

    gettimeofday(&s_time, NULL);

    for ( i = 0; i < num_thread; i++) {
        sprintf(paras[i].url, "%s/speedtest/upload.%s", p_url, p_ext);
        paras[i].result = 0;
        paras[i].upload_size = speed/num_thread;
        //printf("szeleft = %ld\n", paras[i].upload_size);
        error = pthread_create(&paras[i].tid, NULL, (void*)do_upload, (void*)&paras[i]);

        if ( error != 0)
            printf("Can't Run thread num %d, error %d\n", i, error);
    }
    if (init != 0)
    {
        char running = 0;

        do {
            usleep(400*1000);
            running = 0;
            for (i = 0;i < num_thread; i++) {
                if (paras[i].result == 0)
                    running = 1;
            }
            fprintf(stderr, ".");
        }while(running);
        fprintf(stderr, "\n");
    }
    for (i = 0;i < num_thread; i++) {
        pthread_join(paras[i].tid, NULL);
        sum += paras[i].result;
    }
    gettimeofday(&e_time, NULL);

    time = calc_past_time(&s_time, &e_time);

    //printf("msec = %d speed = %0.2fMbps\n", time, ((sum*8*1000/time))/(1024*1024));
    return (sum*1000)/time;
}

static int get_download_filename(double speed, int num_thread)
{
    int i;
    int filelist[] = {350, 500, 750, 1000, 1500, 2000, 3000, 3500, 4000};
    int num_file = ARRAY_SIZE(filelist);

    for (i = 1; i < num_file; i++) {

        int time;
        float times = (float)filelist[i]/350;
        //printf("time %f speed %lf\n", times, speed);
        times = (times*times);
        time = (num_thread*times*FILE_350_SIZE)/speed;
        //printf("%d %d %f\n", filelist[i], time, times);
        if (time > SPEEDTEST_TIME_MAX)
            break;
    }
    if (i < num_file)
        return filelist[i - 1];
    return filelist[num_file - 1];
}

static int get_upload_extension(char *server, char *p_ext)
{
    CURL *curl;
    CURLcode res;
    struct web_buffer web;
    char* p = NULL;
    int i;

    memset(&web, 0, sizeof(web));
    
    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, server);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_web_buf);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &web);
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)); 
        return NOK;
    }
    p = strstr(web.data, UPLOAD_EXTENSION_TAG);
    if (p == NULL || 
        sscanf(p + strlen(UPLOAD_EXTENSION_TAG), "%*[^a-zA-Z]%[a-zA-Z]", p_ext) <= 0) {
        fprintf(stderr, "Upload extension not found\n");
        return NOK;
    }
    printf("Upload extension: %s\n", p_ext);
    return OK;
}

int main(int argc, char *argv[])
{
    int     opt, num_thread;
    char    server_url[URL_LENGTH_MAX], ext[UPLOAD_EXT_LENGTH_MAX];
    double  latency, speed, download_speed, upload_speed;
    int     dsize;

    num_thread  = 4;
    dsize       = INIT_DOWNLOAD_FILE_RESOLUTION;
    memset(server_url, 0, sizeof(server_url));
    memset(ext, 0, sizeof(ext));

    while ( (opt = getopt(argc, argv, "p:s:lh")) > 0) {

        switch (opt) {
            case 'p':
                if (atoi(optarg) > THREAD_NUM_MAX) {

                    fprintf(stderr, "Only support %d threads meanwhile", THREAD_NUM_MAX);
                    exit(-1);
                }
                num_thread = atoi(optarg);
                break;
            case 's':
                strncpy(server_url, optarg, URL_LENGTH_MAX);
                break;
            case 'l':
                break;
            case 'h':
                show_usage(argv);
                break;
        }
    }

    if (server_url[0] == 0) {
         show_usage(argv);
         exit(-1);
    }

    /* Must initialize libcurl before any threads are started */
    curl_global_init(CURL_GLOBAL_ALL);

    latency = test_latency(server_url);
    printf("Server latency is %lfms\n", latency);
    
    speed = test_download(server_url, num_thread, INIT_DOWNLOAD_FILE_RESOLUTION, 0);
    
    dsize = get_download_filename(speed, num_thread);
    fprintf(stderr, "Testing download speed");
    download_speed = test_download(server_url, num_thread, dsize, 1);
    
    printf("Download speed: %0.2fMbps\n", ((download_speed*8)/(1024*1024)));

    if (get_upload_extension(server_url, ext) != OK)
        exit(-1);

    speed = test_upload(server_url, num_thread, speed, ext, 0);

    fprintf(stderr, "Testing uoload speed");
    upload_speed = test_upload(server_url, num_thread, speed*SPEEDTEST_TIME_MAX, ext, 1);

    printf("Upload speed: %0.2fMbps\n", ((upload_speed*8)/(1024*1024)));
    
}



