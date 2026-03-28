#ifndef _TIME_H_
#define _TIME_H_

typedef long time_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
    long tm_gmtoff;
    char *tm_zone;
};

time_t time(time_t *timer);
struct tm *localtime(const time_t *timer);
size_t strftime(char *restrict s, size_t maxsize,
                const char *restrict format,
                const struct tm *restrict timeptr);
char *ctime(const time_t *timer);

#endif
