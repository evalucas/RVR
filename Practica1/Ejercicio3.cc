#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include <iostream>

/*
*   ./time_client <dir> <puerto> <comando>
*/

int main(int argc, char** argv){
    if(argc != 4){
        return -1;
    }

    char buffer[80];

    struct addrinfo hints;
    struct addrinfo * res;

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

    int s = sendto(sd, argv[3], 2, 0, res->ai_addr, res->ai_addrlen);

    if(s == -1){
        return -1;
    }

    if(argv[3][0] == 't' || argv[3][0] == 'd'){
        int bytes = recvfrom(sd, (void *) &buffer, 79, 0 ,res->ai_addr, &res->ai_addrlen);
        if(bytes == -1) return -1;
        buffer[bytes] = '\0';
        std::cout << buffer << std::endl;
    }


    freeaddrinfo(res);
    close(sd);

    return 0;    

}