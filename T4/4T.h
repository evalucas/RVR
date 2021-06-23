#include <string>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <memory>

#include "Serializable.h"
#include "Socket.h"


class GameMessage: public Serializable
{
public:
    static const size_t MESSAGE_SIZE = sizeof(char) * 600 + sizeof(uint8_t);

    enum MessageType
    {
        LOGIN   = 0,
        MESSAGE = 1,
        LOGOUT  = 2,

        GANA1   = 3,
        GANA2   = 4,
        EMPATE  = 5,
        ENCURSO = 6,
        
        INVALIDO= 7,
        QUIT = 8

        //
        //Tipos de mensaje para el juego
    };

    GameMessage(){};

    GameMessage(const std::string& n, const std::string& m):nick(n),message(m){};

    void to_bin();

    int from_bin(char * bobj);

    uint8_t type;

    std::string nick;
    std::string message;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/**
 *  Clase para el servidor de chat
 */
class T4Server
{//Servidor-Cliente
public:
    T4Server(const char * s, const char * p, const char * n): socket(s, p),
        nick(n){
            for(int i=0; i<9;i++){casillas.push_back(-1);}
            socket.bind();
        };
    // {
    //     // socket.bind();
    // };

    /**
     *  Thread principal del servidor recive mensajes en el socket y
     *  lo distribuye a los clientes. Mantiene actualizada la lista de clientes
     */
    void do_messages();

    void input_thread();

    void isValid(GameMessage &cmsg, GameMessage::MessageType &m);
    GameMessage::MessageType winner();
    std::string renderGame();
    std::string renderUI();
    void endGame(GameMessage::MessageType t);

    //void closeServer();

private:
    /**
     *  Lista de clientes conectados al servidor de Chat, representados por
     *  su socket
     */
    std::vector<std::unique_ptr<Socket>> clients;

    /**
     * Socket del servidor
     */
    Socket socket;
        /**
     * Nick del usuario
     */
    std::string nick;
    std::string clientNick;
    bool turn = false;
    bool connect = false;
    std::vector<int> casillas;
    int contadorTurno = 0;
    bool quit = false;
    void createMessage(GameMessage &cmsg);
    void reset();
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/**
 *  Clase para el cliente de chat
 */
class T4Client
{
public:
    /**
     * @param s dirección del servidor
     * @param p puerto del servidor
     * @param n nick del usuario
     */
    T4Client(const char * s, const char * p, const char * n):socket(s, p),
        nick(n){};

    /**
     *  Envía el mensaje de login al servidor
     */
    void login();

    /**
     *  Envía el mensaje de logout al servidor
     */
    void logout();

    /**
     *  Rutina principal para el Thread de E/S. Lee datos de STDIN (std::getline)
     *  y los envía por red vía el Socket.
     */
    void input_thread();

    /**
     *  Rutina del thread de Red. Recibe datos de la red y los "renderiza"
     *  en STDOUT
     */
    void net_thread();

private:

    /**
     * Socket para comunicar con el servidor
     */
    Socket socket;

    /**
     * Nick del usuario
     */
    std::string nick;
    bool turn = false;
    bool quit = false;
};

