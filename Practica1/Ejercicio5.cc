#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include <iostream>

/*
*   ./echo_client <dir> <puerto>
*/

int main(int argc, char** argv){
    if(argc != 3){
        return -1;
    }

    char buffer[80];

    struct addrinfo hints;
    struct addrinfo * res;

    bool exit = false;

    memset((void *) &hints, 0, sizeof(struct addrinfo));
    
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

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

    int c = connect(sd, res->ai_addr, res->ai_addrlen);

    if(c == -1){
        return -1;
    }

    freeaddrinfo(res);

    while(true){
        std::cin>>buffer;
        if(buffer[0] == 'Q' && buffer[1] == '\0')
        {
            break;
        }
        else{
            send(sd,buffer,79,0);
            int bytes = recv(sd,buffer,79,0);
            buffer[bytes]= '\0';
            std::cout << buffer << std::endl;
        }
    }

    close(sd);

    return 0;    

}