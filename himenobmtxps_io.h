int hime_open(const char* file, int flags, mode_t  mode);
int hiem_close(const char* file, int fd);
ssize_t hiem_write(const char* file, int fd, const void* buf, size_t size);
ssize_t hiem_read(const char* file, int fd, void* buf, size_t size);
off_t hiem_lseek(int fd, off_t offset, int whence);
