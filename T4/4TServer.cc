#include <thread>
#include "4T.h"

int main(int argc, char **argv)
{
    T4Server es(argv[1], argv[2], argv[3]);
    std::cout << "Esperando jugador 2....." << std::endl;
    
    std::thread net_thread([&es](){ es.do_messages(); });
   
    es.input_thread();

    return 0;
}
