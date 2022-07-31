#include <fcntl.h>

void stream_fd(int in, int out, char *buffer, size_t buff_size);

void stream_file(FILE *in, FILE *out, char *buffer);

void run_socket();