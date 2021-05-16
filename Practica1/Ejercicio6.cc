#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#include <iostream>
#include <thread>
#include <stdio.h>

#define MAX_THREAD 5
/*
* ./time_server_t <dir> <puerto>
*/

class Thread{
private:
    int _sd;
public:
    Thread(int sd):_sd(sd){};

    void message(){
        char buffer[80];
        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];

        struct sockaddr cliente;
        socklen_t clientelen = sizeof(struct sockaddr);

        while(true){
            int bytes = recvfrom(_sd, (void *) &buffer, 79, 0, 
            &cliente, &clientelen);
            
            if(bytes==-1) return;

            buffer[bytes] ='\0';
            getnameinfo(&cliente, clientelen, host, NI_MAXHOST,
            serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

            std::cout << "Host: " << host << " Port: " << serv << std::endl;
            std::cout << "Thread: " << std::this_thread::get_id() << std::endl;

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
                        sendto(_sd,buffer,len,0,&cliente,clientelen);
                        break;
                    case 'd':
                        len = strftime(buffer, 79,"%F", tInfo);
                        sendto(_sd,buffer,len,0,&cliente,clientelen);
                        break;
                    default:
                        std::cout << "Comando no soportado " << buffer[0] << std::endl;
                        break;
                }
            }

            sleep(3);
        }
    }
};

int main (int argc, char** argv){
    if(argc != 3) return -1;

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

    bind(sd, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    std::thread pool[MAX_THREAD];
    
    for(int i=0; i< MAX_THREAD;i++){
        Thread *t = new Thread(sd);

        pool[i] = std::thread([&t](){
            t->message();
            delete t;
        });        
    }

    for(int i=0; i < MAX_THREAD; i++){
        pool[i].detach();
    }

    char buffer;
    while (true){
        std::cin >> buffer;
        if(buffer== 'q') break;
    }

    close(sd);
    return 0;
}