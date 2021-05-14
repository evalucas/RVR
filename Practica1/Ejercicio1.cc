#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include <iostream>

int main(int argc, char** argv)
{
    struct addrinfo hints;
    struct addrinfo * res;

    memset((void *) &hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(argv[1], NULL, &hints, &res);

    if(rc != 0){
        std::cerr << "[getaddrinfo]: " << gai_strerror(rc) << std::endl;
        return -1;
    }

    for (struct addrinfo *i = res; i != NULL; i = i->ai_next){
        char host[NI_MAXHOST];

        getnameinfo(i->ai_addr, i->ai_addrlen, host, NI_MAXHOST, 0, 0, NI_NUMERICHOST);

        std::cout << host << " " << i->ai_family << " " << i->ai_socktype << std::endl;
    }

    freeaddrinfo(res);

    return 0;
}