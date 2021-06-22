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

    char *tmp = bobj;
    
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
        if(!quit){
            
            ChatMessage cmsg;
            Socket *client = new Socket(socket);
            socket.recv(cmsg,client);   
            if(clients.size() == 0){ //si no ha añadido ningún cliente aún, se trata de la primera conexión. Guardamos la información del cliente en dicho caso.
                std::unique_ptr<Socket> cliente = std::make_unique<Socket>(*client);
                clients.push_back(std::move(cliente));
                clientNick = cmsg.nick;
            }
            if(cmsg.type == ChatMessage::LOGOUT){ //comprobamos que el mensaje recibido sea de tipo LOGOUT aunque se encuentre en su turno
                createMessage(cmsg);
                std::cout << cmsg.message << std::endl;
                socket.send(cmsg,*clients[0]);
                turn = false;
            }
            else if(!turn){//sólo comprobará el mensaje recibido si NO es su turno, a no ser que sea de tipo LOGOUT
                createMessage(cmsg);
                if(cmsg.type != ChatMessage::MessageType::INVALIDO){
                    std::cout << cmsg.message << std::endl;
                    std::cout<< "-----------------------"<<std::endl;
                    turn = true;}
                if(cmsg.type == ChatMessage::MessageType::ENCURSO) 
                    cmsg.type = ChatMessage::MessageType::MESSAGE; //para que el cliente sepa que aún no le toca
                socket.send(cmsg,*clients[0]); 
            }
                   
        }
        
    }
}

void ChatServer::input_thread(){
    while (true)
    {
        if(!quit && connect) //si alguien se ha conectado, se pueden realizar inputs. Esperará hasta que alguien entre para comenzar
        {
           //Lee el input del servidor
            std::string msg;
            std::cin>>msg;
            //Creamos un mensaje con el input recibido
            ChatMessage cmsg = ChatMessage(nick,msg);
            cmsg.type= ChatMessage::MESSAGE; //default
             //comprobamos que el input sea de LOGOUT aunque no sea su turno
            if(msg == "q"){
                cmsg.type = ChatMessage::LOGOUT;
                createMessage(cmsg);
                std::cout << cmsg.message << std::endl;
                socket.send(cmsg,*clients[0]);
                turn = false;
            }
            else if (turn){ //comprobamos que cualquier tipo de input que no sea de LOGOUT sea válido, en su turno
                ChatMessage::MessageType res;
                isValid(cmsg,res);
                cmsg.type = res;
                //según el resultado de la comprobación (errores o mensajes válidos se contemplan), el servidor reacciona ante su propio input recibido
                createMessage(cmsg);
                std::cout << cmsg.message << std::endl;
                std::cout<< "-----------------------"<<std::endl;
                if(cmsg.type != ChatMessage::MessageType::INVALIDO) //a no ser que haya habido un error del servidor, debe enviar al cliente el input del servidor.
                    {socket.send(cmsg,*clients[0]);
                    turn = false;} //el turno se pone a false  
                //si el movimiento del jugador hace que finalice la partida, debe enviar además una nueva partida al cliente
            }
            
        }
    }
    
}

//LÓGICA DEL JUEGO
void ChatServer::createMessage(ChatMessage &cmsg){

    switch (cmsg.type)
        {
            case ChatMessage::MessageType::INVALIDO :
                cmsg.message = "INSERTA UNA CASILLA LIBRE DEL 1 AL 9 PARA METER TU FICHA";
                break;
            case ChatMessage::MessageType::GANA1 :
                cmsg.message += "Ganó "+nick + '\n';
                break;
            case ChatMessage::MessageType::GANA2 :
                cmsg.message += "Ganó "+clientNick+ '\n';
                break;
            case ChatMessage::MessageType::EMPATE :
                cmsg.message += "EMPATE"+ '\n';
                break;
            case ChatMessage::MessageType::ENCURSO :   
                cmsg.message = renderUI() + renderGame();
                contadorTurno++;
                break;
            case ChatMessage::MessageType::LOGIN: 
                std::cout << cmsg.nick << " conectado. Empezando partida..." << std::endl;
                if(nick == cmsg.nick) nick.push_back('_'); //si ambos se llaman igual, modificamos el nombre internamente
                connect = true;
                cmsg.message = renderUI() + renderGame();
                break;
            case ChatMessage::MessageType::LOGOUT: 
                cmsg.message = cmsg.nick + " desconectado";
                quit = true;
                break;
            default: 
                //en este caso, está recibiendo cualquier tipo de input del cliente.
                    //comprobamos que el input sea válido
                    ChatMessage::MessageType res;
                    isValid(cmsg,res);
                    cmsg.type = res;
                    //volvemos a crear un mensaje para enviar al cliente/mostrar en pantalla del servidor.
                    createMessage(cmsg);
                
                break;
        }   
}

void ChatServer::isValid(ChatMessage &msg, ChatMessage::MessageType &m) {
    
    if(msg.message == "q") {
        m = ChatMessage::MessageType::LOGOUT;
        return;
    }

    const char *buffer = msg.message.c_str();
    int i = std::atoi(buffer);

    if(i <10 && i > 0)
    {
        i--;
        if(casillas[i]!=-1) //CASILLA OCUPADA
            m = ChatMessage::MessageType::INVALIDO;
        else{
            if(msg.nick == nick) //Es tu nick, asi que eres el servidor
                casillas[i] = 0;
                
            else
                casillas[i] = 1;   
                
            if(contadorTurno >3){     //SÓLO comprobará la condición de victoria si han pasado los turnos suficientes como para comprobar un ganador.
                m = winner(); //Comprueba el estado del tablero y guarda el mensaje a enviar al cliente
                msg.message = renderUI() + renderGame(); //si gana, pierde, o empata, añade el ultimo tablero
                if(m == ChatMessage::MessageType::GANA1 || m == ChatMessage::MessageType::GANA2 ||m == ChatMessage::MessageType::EMPATE) quit = true;
            }
            else{m=ChatMessage::MessageType::ENCURSO;}
            
     
        }
    }
 
}  

std::string ChatServer::renderUI(){
    //RENDERIZAR UI (turno actual, puntos de cada jugador, nicknames)
    std::string UI = " ";
    UI += nick + " VS " + clientNick ;
    UI.push_back('\n');
    UI += "Turno n.: " +std::to_string(contadorTurno+1);
    UI.push_back('\n');
    UI+= "TURNO DE ";
    if(!turn)UI+=nick;
    else UI+=clientNick;
    UI.push_back('\n');
    UI.push_back('\n');



    return UI;  
}

//'nick1': 0 ----- 'nick2': 0
//Turno: '0'
//Rondas Restantes: '0'

// _|O|X
// _|_|_
// O|_|X
std::string ChatServer::renderGame(){ //No se si esto tiene que estar aqui o en client
// 
    std::string tablero = "";
    for(int i=0; i<3;i++){
        for(int j=0; j<3;j++){
            int k = j+(3*i);
            if(casillas[k]==0) tablero.push_back('X');
            else if(casillas[k]==1) tablero.push_back('O');
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
        if(casillas[i*3]==casillas[i*3+1] && casillas[i*3+1]==casillas[i*3+2] ){
            if(casillas[i*3]==0) return ChatMessage::MessageType::GANA1;
            else if(casillas[i*3]==1) return ChatMessage::MessageType::GANA2; 
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
    if(casillas[2]==casillas[4] && casillas[4]==casillas[6]){
        if(casillas[2]==0) return ChatMessage::MessageType::GANA1;
        else if(casillas[2]==1) return ChatMessage::MessageType::GANA2; 
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
    quit = true;
}

void ChatClient::input_thread()
{
    while (true)
    {
    
        if(!quit){
            //Lee el input del cliente
            std::string msg;
            std::cin>>msg;
            //envía el input al servidor, y éste comprobará si es válido
            ChatMessage cmsg(nick,msg);
            cmsg.type= ChatMessage::MESSAGE; //default
            //comprueba si el input es de LOGOUT aunque no sea su turno
            if(msg == "q"){
                quit = false;
                cmsg.type = ChatMessage::LOGOUT;
                socket.send(cmsg,socket);
            }
            else if(turn){
                turn = false;
                socket.send(cmsg,socket);
            }
                
        }
    }
}

void ChatClient::net_thread()
{

    //Recibir el Tablero (render)
    while(true)
    {
    
        if(!quit){
            //Recibir Mensajes de red
            //Mostrar en pantalla el mensaje de la forma "nick: mensaje"
            ChatMessage cmsg;
            socket.recv(cmsg);
            
            if(cmsg.type == ChatMessage::LOGOUT){//comprueba si el mensaje recibido es de tipo LOGOUT aunque sea su turno
                quit = true;
                turn = false;
                std::cout<<cmsg.message<<std::endl;
            }
            else if (!turn){ //sólo comprobará el mensaje recibido si NO es su turno, a no ser que sea de tipo LOGOUT
                switch (cmsg.type)
                {
                case ChatMessage::MessageType::ENCURSO :
                    turn = true;
                    std::cout<<cmsg.message<<std::endl;
                    std::cout<< "-----------------------"<<std::endl;
                    break;
                default: //default, victoria, derrota, empate, input inválido
                    std::cout<<cmsg.message<<std::endl;
                    std::cout<< "-----------------------"<<std::endl;
                    if(cmsg.type == ChatMessage::MessageType::INVALIDO) turn = true;
                    break;
                }
            }
       
        }
    }
     
}


