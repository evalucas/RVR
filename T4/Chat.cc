#include "Chat.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void ChatMessage::to_bin()
{
    alloc_data(MESSAGE_SIZE);

    memset(_data, 0, MESSAGE_SIZE);

    //Serializar los campos type, nick y message en el buffer _data

    char *tmp = _data;
    
    memcpy(tmp, &type, sizeof(uint8_t));

    tmp +=sizeof(uint8_t);

    memcpy(tmp, nick.c_str(), 8 * sizeof(char));

    tmp += 8*sizeof(char);

    memcpy(tmp, message.c_str(), 80 * sizeof(char));
}

int ChatMessage::from_bin(char * bobj)
{
    alloc_data(MESSAGE_SIZE);

    memcpy(static_cast<void *>(_data), bobj, MESSAGE_SIZE);

    //Reconstruir la clase usando el buffer _data

    char *tmp = _data;
    
    memcpy(&type, tmp, sizeof(uint8_t));

    tmp +=sizeof(uint8_t);

    nick = tmp;

    tmp += 8*sizeof(char);

    message = tmp;

    return 0;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void ChatServer::do_messages()
{
    while (true)
    {
        /*
         * NOTA: los clientes están definidos con "smart pointers", es necesario
         * crear un unique_ptr con el objeto socket recibido y usar std::move
         * para añadirlo al vector
         */

        //Recibir Mensajes en y en función del tipo de mensaje
        // - LOGIN: Añadir al vector clients
        // - LOGOUT: Eliminar del vector clients
        // - MESSAGE: Reenviar el mensaje a todos los clientes (menos el emisor)
        
        ChatMessage msg;

        Socket *client = &socket;

        socket.recv(msg, socket);
        
        std::unique_ptr<Socket> cliente(client);

        switch (msg.type)
        {
        case 0: //LOGIN
            clients.push_back(std::move(cliente));
            std::cout << msg.nick << " conectado" << std::endl;
            connect = true;
            break;
        
        case 1: //MESSAGE
            //Comprobar si es valido...
            for(auto it= clients.begin(); it!= clients.end(); ++it)
            {
                if(!(**it==*cliente)){
                    socket.send(msg,**it);
                }
            }
            break;

        case 2: //LOGOUT

            for(auto it= clients.begin(); it!= clients.end(); ++it)
            {
                if(**it==*cliente){
                    clients.erase(it);
                    break;
                }
            }
            std::cout << msg.nick << " desconectado" << std::endl;
            break;
        
        default:
            break;
        }
    }
}

void ChatServer::input_thread(){
    while (true)
    {
        while(turn)
        {
            if(connect){ 
                // Leer stdin con std::getline
                // Enviar al servidor usando socket
                std::string msg;
                std::getline(std::cin,msg);

                ChatMessage cmsg(nick,msg);
                cmsg.type= ChatMessage::MESSAGE;

                ChatMessage::MessageType m;
                isValid(cmsg,m);
                
                switch (m)
                {
                case 0:
                    /* code */
                    break;
                
                default:
                    break;
                }

                socket.send(cmsg,socket);
            }
           
        }
    }
}

bool ChatServer::isValid(ChatMessage msg, ChatMessage::MessageType m) {
    int m; /*= (int)cmsg.message;*/ //este parseo hay que hacerlo bien obv
    if(m <10 && m > 0)
    {
        if(casillas[m]!=-1){
            //Casilla ocupada
            return false;
        }
        else{
            
            if(msg.nick == nick) //Es tu nick, asi que eres el servidor
            {
                casillas[m] = 0;

            }
            else{
                casillas[m] = 1;
            }
            m = winner();

            return true;
        }
      
    }
    return false; 
}  

ChatMessage::MessageType ChatServer::winner(){
    //-1 0 1 2
    int res; // -1 continuar; >-1 finalizar (0 empate, 1 gana J1, 2 gana J2)
    //comprueba filas
    for(int i = 0 ; i < 3 ; i++){
        
        //int value = casilla
    }
    //comprueba columnas

    //comprueba diagonales
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void ChatClient::login()
{
    std::string msg;

    ChatMessage em(nick, msg);
    em.type = ChatMessage::LOGIN;

    socket.send(em, socket);
}

void ChatClient::logout()
{
    // Completar
    std::string msg;

    ChatMessage em(nick,msg);
    em.type = ChatMessage::LOGOUT;

    socket.send(em,socket);
}

void ChatClient::input_thread()
{
    while (true)
    {
        //Enviar la accion

        // Leer stdin con std::getline
        // Enviar al servidor usando socket
        std::string msg;
        std::getline(std::cin,msg);

        ChatMessage cmsg(nick,msg);
        cmsg.type= ChatMessage::MESSAGE;
        socket.send(cmsg,socket);
    }
}

void ChatClient::net_thread()
{
    //Recibir el Tablero (render)
    while(true)
    {
        //Recibir Mensajes de red
        //Mostrar en pantalla el mensaje de la forma "nick: mensaje"
        ChatMessage msg;
        socket.recv(msg);
        std::cout << msg.nick << ": " << msg.message << std::endl;
    }
}

