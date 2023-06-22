#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#ifdef __linux__
    #include <sys/ioctl.h>
#elif __APPLE__
    #include <fcntl.h>
#else
    #error Platform not supported
#endif

int parse_tcp_url(const char *url, struct sockaddr_in *addr) {
    char *temp_url, *ip, *port_str;
    int port;

    // Make a copy of the url because strtok() modifies the string it processes
    temp_url = strdup(url);
    if (temp_url == NULL) {
        return -1;
    }

    // Remove the "tcp://" prefix
    if (strncmp(temp_url, "tcp://", 6) == 0) {
        ip = temp_url + 6;
    } else {
        free(temp_url);
        return -1;
    }

    // Find the colon separating the IP and port
    port_str = strchr(ip, ':');
    if (port_str == NULL) {
        free(temp_url);
        return -1;
    }

    // Replace the colon with a null terminator to separate the IP and port strings
    *port_str = '\0';
    port_str++;

    // Convert the port string to an integer
    port = atoi(port_str);

    // Fill in the sockaddr_in struct
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &(addr->sin_addr)) <= 0) {
        free(temp_url);
        return -1;
    }

    free(temp_url);
    return 0;
}


int connect_tcp(const char *url) {
    int sockfd;
    int flags;
    int ret;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;


    bzero((char *) &serv_addr, sizeof(serv_addr));
    if (parse_tcp_url(url, &serv_addr) < 0) 
        return -1;

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        return -1;
    }

#ifdef __linux__
    flags = 1;
    if ((ret = ioctl(sockfd, FIONBIO, &flags) < 0)) {
        close(sockfd);
        return ret;
    }
#elif __APPLE__
    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        close(sockfd);
        return flags;
    }
    flags |= O_NONBLOCK;
    if ((ret = fcntl(sockfd, F_SETFL, flags)) < 0) {
        close(sockfd);
        return ret;
    }
#else
    #error Platform not supported
#endif

    return sockfd;
}
