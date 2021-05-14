
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#include <iostream>

/*
*   ./miudp <dir, escucha> <puerto>
*/
int main(int argc, char** argv)
{
    struct addrinfo hints;
    struct addrinfo * res;

    bool exit = false;

    memset((void *) &hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int rc = getaddrinfo(argv[1], argv[2], &hints, &res);

    if(rc != 0){
        std::cerr << "[getaddrinfo]: " << gai_strerror(rc) << std::endl;
        return -1;
    }

    int sd = socket(res->ai_family, res->ai_socktype,0);

    if(sd == -1){
        std::cerr << "[socket]: creacion socket\n";
        return -1;
    }

    bind(sd, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    while(!false){

        char buffer[80];

        char host[NI_MAXHOST];
        char host[NI_MAXSERV];

        struct sockaddr cliente;
        socklen_t clientelen = sizeof(struct sockaddr);
        
        int bytes = recvfrom(sd, (void *) &buffer, 79, 0, 
        &cliente, &clientelen);

        buffer[bytes] = '\0';

        if(bytes == -1){
            return -1;
        }

        

        getnameinfo(i->ai_addr, i->ai_addrlen, host, NI_MAXHOST,
        serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

        std::cout << "Host: " host << " Port: " << serv << std::endl;

        sendto(sd, buffer, bytes , 0, &cliente, clientelen);
        
    }
    close(sd);


    return 0;
}