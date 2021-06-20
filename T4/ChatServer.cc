#include <thread>
#include "Chat.h"

int main(int argc, char **argv)
{
    ChatServer es(argv[1], argv[2], argv[3]);
    std::cout << "Esperando jugador 2....." << std::endl;
    es.do_messages();
    es.input_thread();

    return 0;
}
