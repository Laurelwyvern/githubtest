//
//  xiangqi.cpp
//  helloworldsdl
//
//  Created by Liana Xie on 7/12/24.
//

#include "xiangqi.h"
#include "renderaids.h"
#include <SDL2/SDL_ttf.h>
#include <iostream>     //std::cout
#include <algorithm>    //std::fill, std::copy, std::min, std::max, std::abs
#include <limits.h>     //integer limits
#include <cmath>        //std::sqrt
#include <iterator>     //iterators for arrays
#include <fcntl.h>      //O_NONBLOCK
#include <thread>       //std::this_thread::sleep_for

XQGame::XQGame(int ww, int wh, int bw): winWidth(ww), winHeight(wh), boardWidth(bw), redTurn(RED_FIRST), gameWon({}), gameType(OFFLINE), mySocketfd(-1)
{
    
    //prepare renderer
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return;
    }
    
    if(TTF_Init() != 0) {
        std::cerr << "TTF_Init Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }
    
    win_ = SDL_CreateWindow("Hello SDL", 100, 100, winWidth, winHeight, SDL_WINDOW_SHOWN);
    
    if (win_ == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }
    
    renderer_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer_ == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(win_);
        SDL_Quit();
        return;
    }
    
    //make the renderer able to draw transparent
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    
    
    //prepare fonts
    pieceFontSize = boardWidth/(BOARD_PIECEFONT_RATIO);
    pieceFont = TTF_OpenFont(FONT_PATH, pieceFontSize);
    if(pieceFont == nullptr){
        std::cerr << "TTF_OpenFont Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(win_);
        SDL_Quit();
        return;
    }
    
    //SDL functions will not throw exceptions because they are C functions
    //remaining will also not throw
    
    //prepare pieces
    setDefault();
    
    //upper left hand corner of the board border
    SDL_Point borderUpperLeft = {(winWidth-boardWidth)/2, (winHeight-boardWidth)/2};
    //lower right hand corner of the board border
    
    
    double rowHeight = boardWidth/(XQBOARD_SPOTS_HEIGHT+1);
    double rowWidth = rowHeight;
    int sideBorderWidth = boardWidth-((XQBOARD_SPOTS_WIDTH+1)*rowWidth);
    sideBorderWidth = sideBorderWidth + sideBorderWidth/2;
    
    //fill in `spotCoords`
    /* -- FIRST index is y, SECOND is x! -- */
    //upper left
    spotCoords[0][0] = {borderUpperLeft.x+sideBorderWidth, borderUpperLeft.y+static_cast<int>(rowHeight)};
    
    //fill in all the inbetweens
    for(int i=0; i<XQBOARD_SPOTS_WIDTH; ++i){
        for(int j=0; j<XQBOARD_SPOTS_HEIGHT; ++j){
            spotCoords[j][i] = {static_cast<int>(spotCoords[0][0].x+(i*rowWidth)),
                static_cast<int>(spotCoords[0][0].y+(j*rowHeight))};
        }
    }
    
    //make sure checkness is correct
    updateCheckness();
}

//server version of XQGame
XQGame::XQGame(int ww, int wh, int bw, in_port_t port) : XQGame(ww, wh, bw) {
    //make socket
    //AF_INET = IPv4
    //SOCK_STREAM = TCP
    //O_NONBLOCK = non-blocking; 0 for default policy
    mySocketfd = socket(AF_INET, SOCK_STREAM, 0);
    int mySocketfdTemp = mySocketfd;
    if(mySocketfdTemp<0){
        std::cerr << "Socket Creation Error with code " << errno << std::endl;
        //exit if socket could not be created
        throw errno;
    }
    
    //set game type only if socket creation successful
    gameType = SERVER;
    
    //make the struct to represent the socket
    serverSockAddr.sin_family = AF_INET;
    //INADDR_ANY = listening to all IP addresses
    serverSockAddr.sin_addr.s_addr = INADDR_ANY;
    serverSockAddr.sin_port = htons(port);
    
    //bind the socket
    //needs some casting due to `bind` being a C function
    //`bind` takes a sockaddr* rather than a sockaddr_in*
    if(bind(mySocketfdTemp, reinterpret_cast<struct sockaddr*>(&serverSockAddr), static_cast<socklen_t>(sizeof(serverSockAddr)))
       <0 ){
        std::cerr << "Socket Binding Error with code " << errno << std::endl;
        //exit if socket could not be binded
        throw errno;
    }
    
    //begin listening on the socket
    if(listen(mySocketfdTemp, 5)
       <0 ){
        std::cerr << "Socket Listening Error with code " << errno << std::endl;
        //exit if socket could not be listened on
        throw errno;
    }
    
    //begin waiting for accept
    networkTask = std::async(std::launch::async, &XQGame::networkMotherTask, this);
}

//client version of XQGame
XQGame::XQGame(int ww, int wh, int bw, in_addr_t ip, in_port_t port) : XQGame(ww, wh, bw) {
    //make socket
    //AF_INET = IPv4
    //SOCK_STREAM = TCP
    //O_NONBLOCK = non-blocking; 0 for default policy
    mySocketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(mySocketfd<0){
        std::cerr << "Socket Creation Error with code " << errno << std::endl;
        //exit if socket could not be created
        throw errno;
    }
    
    //set game type only if socket creation successful
    gameType = CLIENT;
    
    //make the struct to represent the socket
    serverSockAddr.sin_family = AF_INET;
    serverSockAddr.sin_addr.s_addr = ip;
    serverSockAddr.sin_port = htons(port);
    
    //connect to server
    //needs some casting due to `connect` being a C function
    //`connect` takes a sockaddr* rather than a sockaddr_in*
    if(connect(mySocketfd, reinterpret_cast<struct sockaddr*>(&serverSockAddr), static_cast<socklen_t>(sizeof(serverSockAddr)))
       <0 ){
        std::cerr << "Socket Connecting Error with code " << errno << std::endl;
        //exit if socket could not be connected
        throw errno;
    }else{
        connected = true;
        
        //begin waiting for accept
        networkTask = std::async(std::launch::async, &XQGame::networkMotherTask, this);
    }
}

void XQGame::setDefault(){
    //set turn
    redTurn = RED_FIRST;
    gameWon = {};
    
    //prepare pieces
    std::fill(std::begin(boardState[0]), std::end(boardState[XQBOARD_SPOTS_WIDTH]), nullptr);
    
    //fill in the `boardState` and `pieces` arrays
    int pieceNum = 0;
    int defBoardIndex = 0;
    //fill in pieces on black side
    for(; pieceNum<16; ++pieceNum){
        while(DEFAULT_HALFBOARDSTATE[defBoardIndex/9][defBoardIndex%9] == XQPiece::Type::NONE){
            ++defBoardIndex;
        }
        pieces[pieceNum] = XQPiece(DEFAULT_HALFBOARDSTATE[defBoardIndex/9][defBoardIndex%9], XQPiece::Side::BLACK, pieceFont, renderer_);
        boardState[defBoardIndex/9][defBoardIndex%9] = pieces+pieceNum;
        
        ++defBoardIndex;
    }
    //fill in pieces on red side
    defBoardIndex = 45-1;
    int boardStateIndex = 45;
    for(; pieceNum<32; ++pieceNum){
        //walk through default board state backwards to have it mirrored
        while(DEFAULT_HALFBOARDSTATE[defBoardIndex/9][defBoardIndex%9] == XQPiece::Type::NONE){
            --defBoardIndex;
            ++boardStateIndex;
        }
        pieces[pieceNum] = XQPiece(DEFAULT_HALFBOARDSTATE[defBoardIndex/9][defBoardIndex%9], XQPiece::Side::RED, pieceFont, renderer_);
        
        boardState[boardStateIndex/9][boardStateIndex%9] = pieces+pieceNum;
        
        --defBoardIndex;
        ++boardStateIndex;
    }
    
    //update checkness
    updateCheckness();
}

XQGame::~XQGame(){
    //network-related
    //if restarting or game is not over, socket has not been closed yet
    if(gameType!=OFFLINE && (willRestart || !gameOver)){
        //close socket
        close(mySocketfd);
    }
    
    //SDL-related
    TTF_CloseFont(pieceFont);
    TTF_Quit();
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(win_);
    SDL_Quit();
}

void XQGame::endGame(bool restarting, std::string overlayMsg){
    gameOver = true;
    willRestart = restarting;
    
    if(!restarting){
        //close socket
        close(mySocketfd);
        mySocketfd = -1;
    }
}

bool XQGame::taskAccept(){
    //blocks
    int acceptedSockfd = accept(mySocketfd, nullptr, nullptr);
    
    //check if accept succeeded
    if(acceptedSockfd >= 0){
        //succeeded
        //get old socket
        int oldSocketfd = mySocketfd;
        //set socket to newly accepted socket = connected!
        mySocketfd = acceptedSockfd;
        connected = true;
        //close old socket (done afterwards for thread-safety)
        close(oldSocketfd);
        return true;
    }else{
        //idk this usually shouldnt happen
        return false;
    }
}

static const XQGameMessage XQ_QUIT_MESSAGE = {XQGameMessage::Flag::QUIT, {{0, 0}, {0, 0}}};
static const SDL_Event EVENT_QUIT = XQGame::makeEvent(XQ_QUIT_MESSAGE);

XQGameMessage XQGame::taskRecieve(){
    //where to store the message sent by other player
    XQGameMessage lastRecievedMessage;
    
    //wait for their turn/check for messages
    //0 = no flags
    size_t msgLen = recv(mySocketfd, &lastRecievedMessage, sizeof(XQGameMessage), 0);
    
    if(msgLen <= 0){
        //other party disconnected (0) or socket error (negative)
        lastRecievedMessage = XQ_QUIT_MESSAGE;
    }
    
    return lastRecievedMessage;
}

void XQGame::networkMotherTask(){
    
    if(gameType==SERVER){
        //begin accepting moves (client always moves second)
        std::future<bool> acceptTask = std::async(std::launch::async, &XQGame::taskAccept, this);
        //wait until a client connects
        if(!acceptTask.get()){
            //quit if accept failed
            return;
        }
    }
    
    std::future<XQGameMessage> recieveTask;
    XQGameMessage lastRecievedMessage;
    
    do {
        recieveTask = std::async(std::launch::async, &XQGame::taskRecieve, this);
        SDL_Event eventToPush = makeEvent(recieveTask.get());
        //send event to SDL
        SDL_PushEvent(&eventToPush);
    }
    //stop if the message is QUIT
    while (lastRecievedMessage.flag!=XQGameMessage::QUIT);
    
}

union EventCodeEnvelope{
    Uint32 code;
    XQGameMessage::MessagePoint points;
};

SDL_Event XQGame::makeEvent(XQGameMessage message){
    SDL_UserEvent userEvent;
    
    SDL_Event result;
    
    userEvent.type = SDL_USEREVENT + message.flag;
    userEvent.timestamp = SDL_GetTicks();
    
    if(userEvent.type >= SDL_LASTEVENT){
        //error case
        SDL_QuitEvent quitEvent;
        quitEvent.type = SDL_QUIT;
        quitEvent.timestamp = userEvent.timestamp;
        result.quit = quitEvent;
    }
    else{
        EventCodeEnvelope envelope;
        envelope.points = message.points;
        
        userEvent.code = envelope.code;
        userEvent.data1 = nullptr;
        userEvent.data2 = nullptr;
        result.user = userEvent;
    }
    
    return result;
}
XQGameMessage XQGame::messageFromEvent(SDL_Event event){
    XQGameMessage result;
    
    result.flag = static_cast<XQGameMessage::Flag>(event.user.type-SDL_USEREVENT);
    
    EventCodeEnvelope envelope;
    envelope.code = event.user.code;
    result.points = envelope.points;
    
    return result;
}

void XQGame::run(SDL_Event& event){
    
    while (true) {
        
        if(SDL_WaitEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return;
            }
            if(event.type == SDL_MOUSEMOTION){
                mouseMovedEvent({event.motion.x, event.motion.y});
            }
            else if(event.type == SDL_MOUSEBUTTONDOWN){
                mouseDownEvent({event.button.x, event.button.y});
            }
            else if(event.type >= SDL_USEREVENT && event.type < SDL_LASTEVENT){
                //custom events from the network task
                XQGameMessage message = messageFromEvent(event);
                if(message.flag == XQGameMessage::Flag::MOVE){
                    movePiece(message.points.from(), message.points.to());
                }
                else if(message.flag == XQGameMessage::Flag::RESTART){
                    endGame(true);
                }
                else if(message.flag == XQGameMessage::Flag::QUIT){
                    endGame(false);
                }
            }
            
            //rendering
            SDL_RenderClear(renderer_);
            
            drawBoard();
            
            //Overlays if needed
            if(gameOver){
                if(!gameWon){
                    drawOverlay(u"QUIT OR DISCONNECTED", COLOR_WHITE);
                }
                else if(*gameWon == XQPiece::Side::RED){
                    drawOverlay(u"RED WON!", COLOR_WHITE);
                }
                else{
                    drawOverlay(u"BLACK WON!", COLOR_WHITE);
                }
            }
            else if(gameType!=OFFLINE && !connected){
                drawOverlay(u"CONNECTING...", COLOR_WHITE);
            }
            
            SDL_RenderPresent(renderer_);
        }
        
        //std::this_thread::sleep_for(std::chrono::milliseconds{50});
    }
}

//TODO: cache surface+texture
void XQGame::drawOverlay(std::u16string overlayMsg, SDL_Color color){
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    
    //base overlay
    SDL_Rect baseOverlay = { (winWidth-boardWidth)/2, (winHeight-boardWidth)/2, boardWidth, boardWidth };
    setRendererColor(renderer_, COLOR_WAITINGOVERLAY);
    SDL_RenderFillRect(renderer_, &baseOverlay);
    
    //overlay message
    SDL_Surface* overlayMsgSurface = TTF_RenderUNICODE_Blended(pieceFont, reinterpret_cast<Uint16*>(overlayMsg.data()), color);
    SDL_Texture* overlayMsgTexture = SDL_CreateTextureFromSurface(renderer_, overlayMsgSurface);
    int texWidth = overlayMsgSurface->w;
    int texHeight = overlayMsgSurface->h;
    SDL_Rect msgRect = {(winWidth-texWidth)/2, (winHeight-texHeight)/2, texWidth, texHeight};
    setRendererColor(renderer_, color);
    drawTextOnRect(renderer_, overlayMsgTexture, &msgRect);
    
    SDL_FreeSurface(overlayMsgSurface);
    SDL_DestroyTexture(overlayMsgTexture);
    setRendererColor(renderer_, COLOR_BLACK);
}

void XQGame::drawBlankBoard(){
    //upper left hand corner of the board border
    SDL_Point borderUpperLeft = {(winWidth-boardWidth)/2, (winHeight-boardWidth)/2};
    
    //board base shape
    SDL_Rect baseBoard = { borderUpperLeft.x, borderUpperLeft.y, boardWidth, boardWidth };
    setRendererColor(renderer_, COLOR_BOARD);
    SDL_RenderFillRect(renderer_, &baseBoard);
    
    
    constexpr const int BSW = XQBOARD_SPOTS_WIDTH;
    constexpr const int BSH = XQBOARD_SPOTS_HEIGHT;
    
    setRendererColor(renderer_, COLOR_BLACK);
    //draw all horizontal lines
    for(int i=0; i<BSH; ++i){
        drawLineWithPoints(renderer_, pieceSpot({0, i}), pieceSpot({BSW-1, i}));
    }
    //draw all vertical lines
    drawLineWithPoints(renderer_, pieceSpot({0, 0}), pieceSpot({0, BSH-1}));
    drawLineWithPoints(renderer_, pieceSpot({BSW-1, 0}), pieceSpot({BSW-1, BSH-1}));
    for(int i=1; i<BSW-1; ++i){
        drawLineWithPoints(renderer_, pieceSpot({i, 0}), pieceSpot({i, (BSH-1)/2}));
        drawLineWithPoints(renderer_, pieceSpot({i, (BSH+1)/2}), pieceSpot({i, BSH-1}));
    }
    //draw diagonal lines
    int startX = 3;
    int endX = 5;
    drawLineWithPoints(renderer_, pieceSpot({startX, 0}), pieceSpot({endX, 2}));
    drawLineWithPoints(renderer_, pieceSpot({endX, 0}), pieceSpot({startX, 2}));
    drawLineWithPoints(renderer_, pieceSpot({startX, BSH-3}), pieceSpot({endX, BSH-1}));
    drawLineWithPoints(renderer_, pieceSpot({endX, BSH-3}), pieceSpot({startX, BSH-1}));
}

void XQGame::drawPiece(BoardPoint spot){
    XQPiece* piece = pieceAt(spot);
    if(piece == nullptr)
        return;
    
    int radius = boardWidth/BOARD_PIECE_RATIO;
    SDL_Point coords = pieceSpot(spot);
    
    if(piece->side() == XQPiece::Side::RED){
        setRendererColor(renderer_, COLOR_RED);
    }
    
    //draw the piece
    drawCircleFilled(renderer_, coords.x, coords.y, radius, COLOR_PIECE, edgePoints, fillPoints);
    drawCircle(renderer_, coords.x, coords.y, radius-radius/6, edgePoints);
    //draw character on piece
    SDL_Point textCoords = { coords.x-(piece->textRect()->w/2), coords.y-(piece->textRect()->h/2) };
    drawTextOnRect(renderer_, piece->texture(), piece->setRectCoords(textCoords));
    
    //highlight and select
    if(highlightedPieceSpot == spot){
        setRendererColor(renderer_, COLOR_HIGHLIGHT);
        for(int i=1; i<=HIGHLIGHT_THICKNESS; ++i){
            drawCircle(renderer_, coords.x, coords.y, radius+i, edgePoints);
        }
    }
    //check/mate indicator
    if(piece->type() == XQPiece::Type::K){
        if(inCheck(piece->side())){
            setRendererColor(renderer_, COLOR_CHECK);
            for(int i=1; i<=CHECK_THICKNESS; ++i){
                drawCircle(renderer_, coords.x, coords.y, radius+i, edgePoints);
            }
        }
        if(inMate(piece->side())){
            int len = radius*0.8;
            setRendererColor(renderer_, COLOR_CHECK);
            for(int i=-CHECK_THICKNESS; i<=CHECK_THICKNESS; ++i){
                drawLineWithPoints(renderer_, {coords.x+len-i, coords.y-len-i},
                                   {coords.x-len-i, coords.y+len-i});
            }
        }
    }
    if(selectedPieceSpot == spot){
        setRendererColor(renderer_, COLOR_SELECTED);
        drawCircleFilled(renderer_, coords.x, coords.y, radius, COLOR_SELECTED, edgePoints, fillPoints);
    }
    
    setRendererColor(renderer_, COLOR_BLACK);
}

void XQGame::drawBoard(){
    SDL_RenderClear(renderer_);
    
    drawBlankBoard();
    
    int boardIndex = 0;
    for(; boardIndex<90; ++boardIndex){
        drawPiece({boardIndex%9, boardIndex/9});
    }
}

void XQGame::mouseMovedEvent(SDL_Point coords){
    if(!gameOver){
        highlightedPieceSpot = pieceAtCoords(coords);
    }
}

void XQGame::mouseDownEvent(SDL_Point coords){
    std::optional<BoardPoint> spot = boardSpotFromCoords(coords);
    //exit/restart if game over
    if(gameOver){
        if(willRestart){
            setDefault();
        }
        else{
            SDL_Event quitEvent = EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
        }
    }
    //only do things if coordinates are valid
    else if(spot){
        //check to deselect piece
        if(hasSelection() && selectedPieceSpot==spot){
            //deselect
            selectedPieceSpot = {};
            return;
        }
        //check to select piece (can already have selection, in which case it will be for selecting a different one)
        else if(!noPieceAt(*spot) && pieceAt(*spot)->side()==currentSide() && myTurn()){
            selectedPieceSpot = spot;
        }
        //check to move piece
        else if(hasSelection() && myTurn()){
            checkAndMovePiece(*selectedPieceSpot, *spot);
        }
    }
}

std::optional<BoardPoint> XQGame::pieceAtCoords(SDL_Point coords) const {
    if( auto boardSpot = boardSpotFromCoords(coords); boardSpot && !noPieceAt(*boardSpot) ) {
        return *boardSpot;
    }
    else {
        return {};
    }
}

std::optional<BoardPoint> XQGame::boardSpotFromCoords(SDL_Point coords) const {
    for(int i=0; i<XQBOARD_SPOTS_WIDTH; ++i){
        for(int j=0; j<XQBOARD_SPOTS_HEIGHT; ++j){
            SDL_Point spotCoords = pieceSpot({i, j});
            double distanceSquared = (spotCoords.x-coords.x)*(spotCoords.x-coords.x) + (spotCoords.y-coords.y)*(spotCoords.y-coords.y);
            if( std::sqrt(distanceSquared) <= boardWidth/BOARD_PIECE_RATIO){
                return BoardPoint{i, j};
            }
        }
    }
    return {};
}

void XQGame::movePiece(BoardPoint from, BoardPoint to){
    
    //win game if general captured
    if(!noPieceAt(to) && pieceAt(to)->type() == XQPiece::Type::K){
        gameWon = pieceAt(from)->side();
    }
    
    XQPiece& movedPiece = *pieceAt(from);
    //move piece at from to to
    boardState[to.y][to.x] = &movedPiece;
    boardState[from.y][from.x] = nullptr;
    
    selectedPieceSpot = {};
    
    //send move to other player if playing online
    if(gameType!=OFFLINE && myTurn()){
        lastSentMessage = makeMessage(XQGameMessage::MOVE, from, to);
        send(mySocketfd, &lastSentMessage, sizeof(lastSentMessage), 0);
    }
    if(!gameWon){
        //update check and mate
        updateCheckness();
        //win game if in mate
        if(inMate(currentSide())){
            gameWon = currentOppSide();
        }else if(inMate(currentOppSide())){
            gameWon = currentSide();
        }
    }
    
    if(gameWon){
        endGame(true);
    }
    
    //switch turns to other side
    redTurn = !redTurn;
}

bool XQGame::validMove(BoardPoint from, BoardPoint to) const {
    return validMove(from, to, boardState);
}

bool XQGame::validMove(BoardPoint from, BoardPoint to, const XQGameBoardState_t &temp) const {
    const XQPiece& piece = *pieceAt(from, temp);
    
    //cannot move to a spot an ally piece currently occupies
    if(!noPieceAt(to, temp) && pieceAt(to, temp)->side()==piece.side()){
        return false;
    }
    
    //whether or not to enforce 王不见王
    if constexpr(WANGBUJIANWANG){
        //perform move with transient boardState
        XQGameBoardState_t tempBoardState;
        std::copy(std::begin(boardState[0]), std::end(boardState[XQBOARD_SPOTS_WIDTH]), std::begin(tempBoardState[0]));
        tempBoardState[to.y][to.x] = tempBoardState[from.y][from.x];
        tempBoardState[from.y][from.x] = nullptr;
        //check if the two generals are on the same file with nothing in between
        BoardPoint redGeneral = generalLocation(XQPiece::Side::RED, tempBoardState);
        BoardPoint blackGeneral = generalLocation(XQPiece::Side::BLACK, tempBoardState);
        //only need to check if they're facing if they are on the same file
        bool nothingBetween = true;
        if(redGeneral.x == blackGeneral.x){
            for(int i=blackGeneral.y+1; i<redGeneral.y; ++i){
                if(tempBoardState[i][blackGeneral.x]!=nullptr){
                    nothingBetween = false;
                    break;
                }
            }
            if(nothingBetween){
                return false;
            }
        }
    }
    
    switch (piece.type()) {
        //chariot
        case XQPiece::Type::R: {
            if(from.y == to.y){
                for(int i=std::min(from.x, to.x)+1; i<std::max(from.x, to.x); ++i){
                    if(!noPieceAt({i, from.y}, temp)){ return false; }
                }
                return true;
            }
            else if(from.x == to.x){
                for(int i=std::min(from.y, to.y)+1; i<std::max(from.y, to.y); ++i){
                    if(!noPieceAt({from.x, i}, temp)){ return false; }
                }
                return true;
            }
            return false;
        }
        //horse
        case XQPiece::Type::H: {
            int yDiff = to.y-from.y;
            int xDiff = to.x-from.x;
            return ((std::abs(xDiff)==1 && yDiff== 2 && noPieceAt({from.x, from.y+1}, temp)) ||
                    (std::abs(xDiff)==1 && yDiff==-2 && noPieceAt({from.x, from.y-1}, temp)) ||
                    (std::abs(yDiff)==1 && xDiff== 2 && noPieceAt({from.x+1, from.y}, temp)) ||
                    (std::abs(yDiff)==1 && xDiff==-2 && noPieceAt({from.x-1, from.y}, temp)));
        }
        //cannon
        case XQPiece::Type::C: {
            //counts number of pieces between `from` and `to`, not including whatever is on `to` or `from`
            bool between = false;
            if(from.y == to.y){
                for(int i=std::min(from.x, to.x)+1; i<std::max(from.x, to.x); ++i){
                    if(!noPieceAt({i, from.y}, temp)){
                        if(!between){ between = true; }
                        //more than one between = not valid
                        else{ return false; }
                    }
                }
            }
            else if(from.x == to.x){
                for(int i=std::min(from.y, to.y)+1; i<std::max(from.y, to.y); ++i){
                    if(!noPieceAt({from.x, i}, temp)){
                        if(!between){ between = true; }
                        //more than one between = not valid
                        else{ return false; }
                    }
                }
            }
            else{
                return false;
            }
            //non-capture move
            if(!between && noPieceAt(to, temp)){ return true; }
            //capture; must jump over another piece
            else if(between && !noPieceAt(to, temp)){ return true; }
            return false;
        }
        //pawn
        case XQPiece::Type::P: {
            bool sideBlack = pieceAt(from, temp)->side() == XQPiece::Side::BLACK;
            bool crossedRiver = (sideBlack && from.y < XQBOARD_SPOTS_HEIGHT/2) ||
                                (!sideBlack && from.y >= XQBOARD_SPOTS_HEIGHT/2);
            int forwardOne = sideBlack ? from.y+1 : from.y-1;
            
            if(crossedRiver){
                //hasnt crossed the river
                return (to.x==from.x && to.y==(forwardOne));            //one spot forward
            }else{
                //has crossed the river
                return (to.y==from.y && std::abs(from.x-to.x)==1) ||    //one spot to the side
                        (to.x==from.x && to.y==(forwardOne));           //one spot forward
            }
        }
        //elephant
        case XQPiece::Type::E: {
            bool sideBlack = pieceAt(from, temp)->side() == XQPiece::Side::BLACK;
            bool crossingRiver = (sideBlack && to.y >= XQBOARD_SPOTS_HEIGHT/2) ||
                                (!sideBlack && to.y < XQBOARD_SPOTS_HEIGHT/2);
            return ( !crossingRiver &&
                    std::abs(to.x-from.x)==2 && std::abs(to.y-from.y)==2 &&    //田 movement
                    noPieceAt({(to.x+from.x)/2, (to.y+from.y)/2}, temp));             //no piece in center of 田
        }
        //advisor
        case XQPiece::Type::A: {
            return (inRightPalace(to, piece.side()) &&
                    (std::abs(from.y-to.y)==1 && std::abs(from.x-to.x)==1));
        }
        //general
        case XQPiece::Type::K: {
            return (inRightPalace(to, piece.side()) &&
                    ((from.x==to.x && std::abs(from.y-to.y)==1) || (from.y==to.y && std::abs(from.x-to.x)==1)));
        }
        default:
            return false;
    }
}

bool XQGame::inRightPalace(BoardPoint spot, XQPiece::Side side) const {
    if(side == XQPiece::Side::BLACK){
        return (spot.x>=3 && spot.x<=5) && (spot.y<3);
    }else{
        return (spot.x>=3 && spot.x<=5) && (spot.y>=XQBOARD_SPOTS_HEIGHT-3);
    }
}

BoardPoint XQGame::generalLocation(XQPiece::Side side, const XQGameBoardState_t &temp) const {
    int minRank, maxRank;
    if(side == XQPiece::Side::BLACK){
        minRank = 0;
        maxRank = 3;
    }else{
        minRank = XQBOARD_SPOTS_HEIGHT-3;
        maxRank = XQBOARD_SPOTS_HEIGHT;
    }
    for(int rank=minRank; rank<maxRank; ++rank){
        for(int file=3; file<=5; ++file){
            BoardPoint spot = {file, rank};
            if(!noPieceAt(spot, temp) && pieceAt(spot, temp)->type()==XQPiece::Type::K){
                return spot;
            }
        }
    }
    //error case, should never happen
    return {-1, -1};
}

XQGameMessage XQGame::makeMessage(XQGameMessage::Flag type, BoardPoint from, BoardPoint to){
    XQGameMessage result;
    result.flag = type;
    result.points = {from, to};
    return  result;
}

bool XQGame::inCheck(XQPiece::Side side) const {
    return inCheckMap.at(side);
}

//TODO: optimize
void XQGame::updateCheckness(){
    
    for(XQPiece::Side side : XQPiece::Sides){
        inCheckMap[side] = inCheck(side, boardState);
        inMateMap[side] = inMateCheck(side);
    }
}

bool XQGame::inMateCheck(XQPiece::Side side){
    //loop through each point on the board to find pieces of the same side
    for(BoardPoint from : boardPointArray()){
        if(!noPieceAt(from) && pieceAt(from)->side()==side){ //same side
            for(BoardPoint to : boardPointArray()){
                //if move is valid
                if(validMove(from, to)){
                    //perform move with transient boardState
                    XQGameBoardState_t tempBoardState;
                    std::copy(std::begin(boardState[0]), std::end(boardState[XQBOARD_SPOTS_WIDTH]), std::begin(tempBoardState[0]));
                    tempBoardState[to.y][to.x] = tempBoardState[from.y][from.x];
                    tempBoardState[from.y][from.x] = nullptr;
                    //check if in check
                    //if not in check, not in mate
                    if(!inCheck(side, tempBoardState)){
                        return false;
                    }
                    //otherwise, keep on going
                }
            }
        }
    }
    //no moves will result in not being check, so in mate
    return true;
}

bool XQGame::inCheck(XQPiece::Side side, const XQGameBoardState_t &temp) const {
    BoardPoint generalPoint = generalLocation(side, temp);
    
    //loop through each point on the board to find pieces of the opposite side
    for(BoardPoint from : boardPointArray()){
        if(!noPieceAt(from, temp) &&            //existing piece
           pieceAt(from, temp)->side()!=side && //opposite side
           validMove(from, generalPoint)        //can capture general
           ){
            return true;
        }
    }
    return false;
}

bool XQGame::inMate(XQPiece::Side side) const {
    return inMateMap.at(side);
}



XQPiece::XQPiece(Type t, Side s, TTF_Font* f, SDL_Renderer* renderer):
    type_(t), side_(s)
{
    SDL_Surface* surface_(TTF_RenderUNICODE_Blended(f, displayChar(t, s), s==XQPiece::Side::BLACK ? COLOR_BLACK : COLOR_RED));
    if(surface_ == nullptr){
        throw std::exception();
    }
    texture_ = SDL_CreateTextureFromSurface(renderer, surface_);
    textRect_.w = surface_->w;
    textRect_.h = surface_->h;
    SDL_FreeSurface(surface_);
}

XQPiece::~XQPiece(){
    if(texture_ != nullptr){        
        SDL_DestroyTexture(texture_);
    }
}



