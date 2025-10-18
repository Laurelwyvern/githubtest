#include <cstdlib>      //atoi
#include <arpa/inet.h>  //inet_pton
#include "xiangqi.h"    //XQGame
#include <iostream>     //std::cerr

in_addr_t ipv4AddrFromStr(const std::string& str){
    in_addr_t res;
    if(inet_pton(AF_INET, str.data(), &res) == 1){
        return res;
    }
    return 0;
}


int main(int argc, char* argv[]) {
    
    SDL_Event event;
    try{
        //server version
        if(argc == 2){
            XQGame game(750, 500, 500, std::atoi(argv[1]));
            game.run(event);
        }
        //client version
        else if(argc == 3){
            XQGame game(750, 500, 500, ipv4AddrFromStr(argv[1]), std::atoi(argv[2]));
            game.run(event);
        }
        //offline version
        else{
            XQGame game(750, 500, 500);
            game.run(event);
        }
    }catch(errno_t eno){
        std::cerr << "caught exception with errno " << eno << "; program quit" << std::endl;
    }
}
