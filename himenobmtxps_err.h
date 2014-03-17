#ifndef HIME_ERR_H
#define HIME_ERR_H

void hime_err_init(int r);
void hime_err(const char* fmt, ...);
void hime_alert(const char* fmt, ...);
void hime_dbg(const char* fmt, ...);
void hime_print(const char* fmt, ...);
void hime_debug(const char* fmt, ...);
void hime_btrace();

void hime_exit(int no);

#endif
