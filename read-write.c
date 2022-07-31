
#include <stdio.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdbool.h>

#include "stream.h"
#include "config.h"
#include "utils.h"

#define PORT 8080
#define BUFFER_SIZE 1 << 10
#define LAST_BUFFER_INDEX (BUFFER_SIZE - 1)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define HTTP_SUCCESS "HTTP/1.1 200 OK\n"                 \
                     "Server: Harrison-Ho\n"             \
                     "Content-Type: text/html\n"         \
                     "Content-Length: %ld\n"             \
                     "Accept-Ranges: bytes\n"            \
                     "Connection: keep-alive\n"          \
                     "Keep-Alive: timeout=5, max=1000\n" \
                     "\n"

#define FILE_PATH "index.html"

int main(int argc, char *argv[])
{
    // socket server use pipe FD
    run_socket();
    return 0;
}

void error(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

__off_t get_file_size(int fd)
{
    __off_t n = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET); // set cursor to the beginning of the file
    return n;
}

char *itoa(size_t i)
{
    char buf[(i % 10) + 1];
    sprintf(buf, "%ld", i);
    return strdup(buf);
}

void stream_file(FILE *in, FILE *out, char *buffer)
{
    if (in)
    {
        int nread = 0;
        while ((nread = fread(buffer, 1, sizeof(buffer), in)) > 0)
            fwrite(buffer, 1, nread, out);
        if (ferror(in))
        {
            error("Error reading file");
        }
    }
}

void stream_fd(int in, int out, char *buffer, size_t buff_size)
{
    if (in)
    {
        int nread = 0;
        while ((nread = read(in, buffer, buff_size)) > 0)
            write(out, buffer, nread);
        if (nread < 0)
        {
            error("read");
        }
    }
}

void read_request_header(int client_fd, int out)
{
    char buffer[BUFFER_SIZE];

    if (client_fd)
    {
        int nread = 0;
        while ((nread = read(client_fd, buffer, sizeof(buffer) - 1)) > 0)
        {
            write(out, buffer, nread);
            if (nread < 0)
            {
                error("read");
            }
        }
    }
}

void close_client_fd(int client_fd)
{
    // shutdown read-write socket descriptor
    sleep(3);
    shutdown(client_fd, SHUT_WR);
    close(client_fd);
}

void run_socket()
{
    int socket_fd, n = 0;
    int portno = PORT;

    // client
    struct sockaddr_in client_addr;
    socklen_t client_len = 0;

    // A sockaddr_in is a structure containing an internet address
    struct sockaddr_in server_addr;

    // buffer
    char buffer[BUFFER_SIZE];

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    int option = 1; // for close bind address

    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    server_addr.sin_family = AF_INET;

    server_addr.sin_port = htons(portno); // convert port number to network byte order

    server_addr.sin_addr.s_addr = INADDR_ANY;

    // bind server to port & host
    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        error("ERROR on binding");
    }

    // listen & set queue for connections
    listen(socket_fd, SOMAXCONN);

    // accept connection
    client_len = sizeof(client_addr);

    printf("Server is started  on port %d , Waiting for connection...\n", portno);

    while (true)
    {
        int client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            error("ERROR on accept");
        }
        int html_fd = open(FILE_PATH, O_RDONLY);
        // compute HTML file size
        __off_t html_file_size = get_file_size(html_fd);
        // malloc enough header memory
        size_t header_len = sizeof(char) * ((strlen(itoa(html_file_size)) + strlen(HTTP_SUCCESS)) - 3);
        char *header = malloc(header_len);
        // format header
        sprintf(header, HTTP_SUCCESS, html_file_size);
        // write header to client
        write(client_fd, header, header_len);
        // stream file to client
        stream_fd(html_fd, client_fd, buffer, BUFFER_SIZE);

        // clean up
        close(html_fd);
        close_client_fd(client_fd);
        free(header);
    }
}
