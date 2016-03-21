#ifndef STRERR_H
#define STRERR_H

void strerr_warn(const char *fmt, ...) __attribute__ ((format(printf,1,2)));
void strerr_warnsys(const char *fmt, ...) __attribute__ ((format(printf,1,2)));
void strerr_die(int status, const char *fmt, ...) __attribute__ ((format(printf,2,3)));
void strerr_diesys(int status, const char *fmt, ...) __attribute__ ((format(printf,2,3)));

#endif /* STRERR_H */
