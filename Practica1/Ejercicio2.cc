
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#include <iostream>

/*
*   ./time_server <dir, escucha> <puerto>
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

    while(!exit){

        char buffer[80];

        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];

        struct sockaddr cliente;
        socklen_t clientelen = sizeof(struct sockaddr);
        
        int bytes = recvfrom(sd, (void *) &buffer, 79, 0, 
        &cliente, &clientelen);

        buffer[bytes] = '\0';

        if(bytes == -1){
            return -1;
        }   

        getnameinfo(&cliente, clientelen, host, NI_MAXHOST,
        serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

        std::cout << bytes << "bytes de "<< host << " : " << serv << std::endl;


        if(bytes == 2){
            time_t t;
            struct tm* tInfo;

            time(&t);
            tInfo = localtime(&t);

            int len;

            switch (buffer[0])
            {
                case 't':
                    len = strftime(buffer, 79,"%r", tInfo);
                    sendto(sd,buffer,len,0,&cliente,clientelen);
                    break;
                case 'd':
                    len = strftime(buffer, 79,"%F", tInfo);
                    sendto(sd,buffer,len,0,&cliente,clientelen);
                    break;
                case 'q':
                    exit= true;                
                    break;           
                default:
                    std::cout << "Comando no soportado " << buffer[0] << std::endl;
                    break;
            }
        }

    }
    close(sd);


    return 0;
}