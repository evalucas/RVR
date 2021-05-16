#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#include <iostream>
#include <thread>
#include <condition_variable>

/*
* ./echo_server_t <dir> <puerto>
*/

std::condition_variable num_cv;
std::mutex num_mutex;
int num_clientes = 0;

class Thread{
private:
    int _sd;
public:
    Thread(int sd):_sd(sd){};

    void message(){

        while(true){
            char buffer[80];

            int bytes = recv(_sd, (void *) &buffer, 79, 0);

            if(bytes <=0){
                std::cout << "Thread: " << std::this_thread::get_id() << " Conexión terminada" << std::endl;
                break;
            }
            buffer[bytes] = '\0';

            send(_sd, buffer, bytes,0);

        }
        close(_sd);

        num_mutex.lock();
        num_clientes--;

        if(num_clientes < MAX_THREAD)
        {
            num_cv.notify_all();
        }

        num_mutex.unlock();
    }
};

int main (int argc, char** argv){
    if(argc != 3) {
        return -1;
    }

    struct addrinfo hints;
    struct addrinfo * res;

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

    bind(sd, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    listen(sd,16);

    while(true){
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];

    struct sockaddr cliente;
    socklen_t clientelen = sizeof(struct sockaddr);

    int client_sd = accept(sd,&cliente,&clientelen);

    num_mutex.lock();
    num_clientes--;

    num_mutex.unlock();
        
    getnameinfo(&cliente, clientelen, host, NI_MAXHOST,
    serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

    std::cout << "Conexión desde " << host << " " << serv << std::endl;

    Thread* t = new Thread(sd);
    std::thread([&t](){
        t->message();
        delete t;
    }).detach();

    }

    close(sd);

    return 0;
}