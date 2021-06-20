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
        
        ChatMessage cmsg;

        Socket *client;

        socket.recv(cmsg,client);
        
        std::unique_ptr<Socket> cliente(client);

        std::string tablero;
        std::string UI;

        switch (cmsg.type)
        {
        case ChatMessage::MessageType::LOGIN: //LOGIN
            clients.push_back(std::move(cliente));
            clientNick = cmsg.nick;
            std::cout << cmsg.nick << " conectado. Empezando partida..." << std::endl;
            if(nick == cmsg.nick) nick.push_back('_'); //si ambos se llaman igual, modificamos el nombre internamente
            //configurar partida
            std::cout<<"A cuantas rondas quieres hacer la partida? 1,3,5, 0 (infinito)"<<std::endl;
            //comunica al cliente que se esta configurando la partida
            cmsg.message = nick + " está configurando la partida, espera...";
            cmsg.type = ChatMessage::MessageType::MESSAGE;
            socket.send(cmsg,*cliente);  
            connect = true;
            break;
        case ChatMessage::MessageType::LOGOUT: //LOGOUT
            std::cout << cmsg.nick << " desconectado" << std::endl;
            break;
        default:
            ChatMessage::MessageType res;
            //comprobamos que el input sea válido
            isValid(cmsg,res);
            cmsg.type = res;
            if(res == ChatMessage::MessageType::INVALIDO) cmsg.message = "INSERTA UNA CASILLA DEL 1 AL 9 PARA METER TU FICHA" ;
            socket.send(cmsg,*cliente); 
            break;
        }
    }
}

void ChatServer::input_thread(){
    while (true)
    {
        while(turn)
        {
            if(connect){ //si alguien se ha conectado, se pueden realizar inputs. Esperará hasta que alguien entre para comenzar
                //Lee el input del servidor
                std::string msg;
                std::getline(std::cin,msg);
                //Creamos un mensaje con el input recibido
                ChatMessage cmsg(nick,msg);
                cmsg.type= ChatMessage::MESSAGE; //default
                ChatMessage::MessageType res;
                //comprobamos que el input sea válido
                isValid(cmsg,res);
                //comprobamos si la partida ha comenzado o no para configurar la partida o leer inputs de partida
                if(!started){
                    switch (res)
                    {
                        case ChatMessage::MessageType::INVALIDO:
                            std::cout<<"INSERTA UN NÚMERO ENTRE LOS SIGUIENTES: -1 (infinitas rondas), 1, 3, 5" << std::endl;
                            break;
                        default:
                            started = true;
                            //renderiza en el jugador 1 (servidor) el primer tablero
                            std::string tablero = renderGame(); std::string UI = renderUI();
                            std::cout<<UI<<tablero <<std::endl;
                            //enviar al cliente el primer tablero
                            cmsg.message = UI + tablero;
                            cmsg.type = ChatMessage::MessageType::MESSAGE;
                            break;
                    }
                }
                else{
                //según el resultado de la comprobación (errores o mensajes válidos se contemplan), el servidor reacciona ante su propio input recibido
                    switch (res)
                        {
                        case ChatMessage::MessageType::INVALIDO :
                            std::cout<<"INSERTA UNA CASILLA DEL 1 AL 9 PARA METER TU FICHA" << std::endl;
                            break;
                        case ChatMessage::MessageType::GANA1 :
                            cmsg.type = res;
                            cmsg.message = "Ha ganado "+nick;
                            std::cout<< cmsg.message <<std::endl;
                            break;
                        case ChatMessage::MessageType::GANA2 :
                            cmsg.type = res;
                            cmsg.message = "Ha ganado "+clientNick;
                            std::cout<<cmsg.message <<std::endl;
                            break;
                        case ChatMessage::MessageType::EMPATE :
                            cmsg.type = res;
                            cmsg.message = "EMPATE";
                            std::cout<<cmsg.message <<std::endl;
                            break;
                        case ChatMessage::MessageType::ENCURSO :
                            cmsg.type = res;   
                            cmsg.message = renderUI() + renderGame();
                            std::cout<<cmsg.message<<std::endl;
                            turn = false;
                            contadorTurno++;
                            break;
                        default:
                            break;
                        }   
                }
                socket.send(cmsg,*clients[0].get());
            }
           
        }
    }
}

//LÓGICA DEL JUEGO


void ChatServer::isValid(ChatMessage msg, ChatMessage::MessageType &m) {
    
    const char *buffer = msg.message.c_str();
    int i = std::atoi(buffer);
    
    if(!started){
        if(i == -1 || i == 1 || i==3 || i == 5 ){
            contadorRonda = i;
        }
        else{
           m = ChatMessage::MessageType::INVALIDO; 
        }

    }
    else if(i <10 && i > 0)
    {
        if(casillas[i]!=-1) //CASILLA OCUPADA
            m = ChatMessage::MessageType::INVALIDO;
        else{
            if(msg.nick == nick) //Es tu nick, asi que eres el servidor
                casillas[i] = 0;
            else
                casillas[i] = 1;   
            
            if(contadorTurno >3){     //SÓLO comprobará la condición de victoria si han pasado los turnos suficientes como para comprobar un ganador.
                m = winner(); //Comprueba el estado del tablero y guarda el mensaje a enviar al cliente
                endGame(m);
            }
            else{m=ChatMessage::MessageType::ENCURSO;}
            
     
        }
    }
 
}  

void ChatServer::endGame(ChatMessage::MessageType t){
    switch (t)
    {
    case ChatMessage::MessageType::GANA1 :
        puntos1++; contadorRonda--; contadorTurno=0; turn=true;
        break;
    case ChatMessage::MessageType::GANA2 :
        puntos2++; contadorRonda--; contadorTurno=0; turn=true;
    break;
    case ChatMessage::MessageType::EMPATE :
        puntos1++; puntos2++; contadorRonda--; contadorTurno=0; turn=true;
    break;
    default:
        break;
    }

    if(contadorRonda == 0){started=false; }
}

std::string ChatServer::renderUI(){
    //RENDERIZAR UI (turno actual, puntos de cada jugador, nicknames)
    std::string UI;
    std::string ronda;
    if(contadorRonda < 0) ronda = "infinitas";
    else ronda = contadorRonda;

    UI = nick + ": "; 
    UI+= puntos1 + " ----- " + clientNick + ": " ;
    UI+= puntos2 + '\n' + "Turno: ";
    UI+= contadorTurno + '\n' + "Rondas Restantes: " + ronda + '\n' + '\n';
    
    return UI;  
}

//'nick1': 0 ----- 'nick2': 0
//Turno: '0'
//Rondas Restantes: '0'

// _|O|X
// _|_|_
// O|_|X
std::string ChatServer::renderGame(){ //No se si esto tiene que estar aqui o en client

    std::string tablero = "";
    for(int i=0; i<3;i++){
        for(int j=0; j<3;j++){
            if(casillas[j+i]==0) tablero.push_back('X');
            else if(casillas[j+i]==1) tablero.push_back('O');
            else tablero.push_back('_');

            if(j==0 || j==1) tablero.push_back('|');
        }
        tablero.push_back('\n');
    }
    return tablero;
}
ChatMessage::MessageType ChatServer::winner(){

    //comprueba filas
    for(int i = 0 ; i < 3 ; i++){       
        if(casillas[i]==casillas[i+1] && casillas[i+1]==casillas[i+2] ){
            if(casillas[i]==0) return ChatMessage::MessageType::GANA1;
            else if(casillas[i]==1) return ChatMessage::MessageType::GANA2; 
        }
    }
    //comprueba columnas
    for(int i = 0 ; i < 3 ; i++){
        if(casillas[i]==casillas[i+3] && casillas[i+3]==casillas[i+6] ){
            if(casillas[i]==0) return ChatMessage::MessageType::GANA1;
            else if(casillas[i]==1) return ChatMessage::MessageType::GANA2; 
        } 
    }
    //comprueba diagonales
    if(casillas[0]==casillas[4] && casillas[4]==casillas[8]){
        if(casillas[0]==0) return ChatMessage::MessageType::GANA1;
        else if(casillas[0]==1) return ChatMessage::MessageType::GANA2; 
    }
    if(casillas[3]==casillas[4] && casillas[4]==casillas[6]){
        if(casillas[0]==0) return ChatMessage::MessageType::GANA1;
        else if(casillas[0]==1) return ChatMessage::MessageType::GANA2; 
    }
    //comprueba que el tablero esté lleno o no
    for(int i=0; i<9;i++){
        if(casillas[i]==-1) return ChatMessage::MessageType::ENCURSO;
    }

    return ChatMessage::MessageType::EMPATE;   
    
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
        while(turn)
        {
            //Lee el input del cliente
            std::string msg;
            std::getline(std::cin,msg);
            //envía el input al servidor, y éste comprobará si es válido
            ChatMessage cmsg(nick,msg);
            cmsg.type= ChatMessage::MESSAGE; //default
            socket.send(cmsg,socket);
        }
    }
}

void ChatClient::net_thread()
{
    //Recibir el Tablero (render)
    while(true)
    {
        //Recibir Mensajes de red
        //Mostrar en pantalla el mensaje de la forma "nick: mensaje"
        ChatMessage cmsg;
        socket.recv(cmsg);
        switch (cmsg.type)
            {
            case ChatMessage::MessageType::ENCURSO :
                std::cout<<cmsg.message <<std::endl;
                turn = true;
                break;
            default: //victoria, derrota, empate, input inválido
                std::cout<<cmsg.message <<std::endl;
                if(cmsg.type != ChatMessage::MessageType::INVALIDO) turn = false;
                break;
            }
        socket.send(cmsg,socket);
    }


        
}


