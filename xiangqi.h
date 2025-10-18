//
//  xiangqi.h
//  helloworldsdl
//
//  Created by Liana Xie on 7/10/24.
//

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <optional>
#include <map>          //std::map
#include <future>       //std::future, std::async
#include <mutex>        //std::mutex, std::lock_guard

#ifdef __WIN32__            //socket headers
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

//colors

//color = #000000 = black
constexpr SDL_Color COLOR_BLACK = {0, 0, 0, SDL_ALPHA_OPAQUE};
//color = #c80000 = red
constexpr SDL_Color COLOR_RED = {200, 0, 0, SDL_ALPHA_OPAQUE};
constexpr SDL_Color COLOR_WHITE = {255, 255, 255, SDL_ALPHA_OPAQUE};

//color = white
constexpr SDL_Color COLOR_HIGHLIGHT = COLOR_WHITE;
//white overlay
constexpr SDL_Color COLOR_SELECTED = {255, 255, 255, SDL_ALPHA_OPAQUE/3};

//color = yellow
constexpr SDL_Color COLOR_CHECK = {255, 255, 0, SDL_ALPHA_OPAQUE};

//color = #c89632
constexpr SDL_Color COLOR_BOARD = {200, 150, 50, SDL_ALPHA_OPAQUE};
//color = #e6b446
constexpr SDL_Color COLOR_PIECE = {230, 180, 70, SDL_ALPHA_OPAQUE};

//black, half transparent
constexpr SDL_Color COLOR_WAITINGOVERLAY = {0, 0, 0, SDL_ALPHA_OPAQUE/2};

//font paths
constexpr const char UNIFONTEX_PATH[] = "/Users/lianaxie/Downloads/unifontex-font/Unifontexmono-DYWdE.ttf";
constexpr const char GB18030BITMAP_PATH[] = "/System/Library/Fonts/Supplemental/NISC18030.ttf";
constexpr const char HEITISTLIGHT_PATH[] = "/System/Library/Fonts/STHeiti Light.ttc";
constexpr const char HIROGINOMINCHOPRON_PATH[] = "/System/Library/Fonts/ヒラギノ明朝 ProN.ttc";
constexpr const char PINGFANGHK_PATH[] = "/System/Library/Fonts/PingFang.ttc";
constexpr const char* FONT_PATH = HEITISTLIGHT_PATH;

//display ratios
constexpr int BOARD_PIECE_RATIO = 25;
constexpr int BOARD_PIECEFONT_RATIO = 25;
constexpr int HIGHLIGHT_THICKNESS = 2;
constexpr int CHECK_THICKNESS = 3;

//game options
constexpr const bool RED_FIRST = true;
constexpr const bool WANGBUJIANWANG = true;

constexpr const char16_t XQPIECETYPE_WCHARS1[16][4] = {
    u"車", u"馬", u"炮", u"兵", u"相", u"仕", u"帥",     //red
    u"車", u"馬", u"砲", u"卒", u"象", u"士", u"將"      //black
};
constexpr const char16_t XQPIECETYPE_WCHARS2[16][4] = {
    u"俥", u"傌", u"炮", u"兵", u"相", u"仕", u"帥",     //red
    u"車", u"馬", u"砲", u"卒", u"象", u"士", u"將"      //black
};
constexpr const char16_t * XQPIECE_CHARSET = *XQPIECETYPE_WCHARS2;


class XQPiece{
    
public:
    /*
     "　" - ideographic space character
     車　     Chariot     R
     馬　     Horse       H
     炮/砲    Cannon      C
     兵/卒    Pawn        P
     相/象    Elephant    E
     仕/士    Advisor     A
     帥/將    King        K
    */
    enum Type { R, H, C, P, E, A, K, NONE };
    enum Side { RED, BLACK };
    static constexpr Type Types[7] = {R, H, C, P, E, A, K};
    static constexpr Side Sides[2] = {RED, BLACK};
    
private:
    Type type_;
    Side side_;
    //used for rendering text
    SDL_Texture* texture_;
    SDL_Rect textRect_;     //contains text to be written. should always have its coordinates set externally before use
    
public:
    //constructors
    XQPiece(): type_(NONE), side_(RED), texture_(nullptr){ ; }
    XQPiece(Type t, Side s, TTF_Font* f, SDL_Renderer* renderer);
    
    //rule of three
    XQPiece(XQPiece& other) = delete;
    XQPiece(XQPiece&& other): type_(other.type_), side_(other.side_), texture_(other.texture_), textRect_(other.textRect_){
        other.texture_ = nullptr;
    }
    void operator=(XQPiece& other) = delete;
    void operator=(XQPiece&& other){
        type_=other.type_; side_=other.side_;
        texture_=other.texture_; textRect_=other.textRect_;
        other.texture_ = nullptr;
    }
    ~XQPiece();
    
    //return cstring containing char displayed on the piece
    static const Uint16* displayChar(Type t, Side s){
        return reinterpret_cast<const Uint16*>(XQPIECETYPE_WCHARS1[ static_cast<size_t>(t) + (7*(s==BLACK)) ]);
        //return reinterpret_cast<const Uint16*>(u"P");
    }
    //return the piece's rect, on which the piece's char is displayed, and set its coordinates
    SDL_Rect* setRectCoords(SDL_Point p){ textRect_.x=p.x; textRect_.y=p.y; return &textRect_; }
    
    //return information about piece
    Type type() const { return type_; }
    Side side() const { return side_; }
    SDL_Rect* textRect() { return &textRect_; }
    SDL_Texture* texture() { return texture_; }
    const SDL_Rect* textRect() const { return &textRect_; }
    const SDL_Texture* texture() const { return texture_; }
};


constexpr int XQBOARD_SPOTS_WIDTH = 9;
constexpr int XQBOARD_SPOTS_HEIGHT = 10;

constexpr XQPiece::Type DEFAULT_HALFBOARDSTATE[XQBOARD_SPOTS_HEIGHT/2][XQBOARD_SPOTS_WIDTH] = {
    { XQPiece::Type::R, XQPiece::Type::H, XQPiece::Type::E, XQPiece::Type::A, XQPiece::Type::K,
      XQPiece::Type::A, XQPiece::Type::E, XQPiece::Type::H, XQPiece::Type::R },
    { XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::NONE,
      XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::NONE },
    { XQPiece::Type::NONE, XQPiece::Type::C, XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::NONE,
      XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::C, XQPiece::Type::NONE },
    { XQPiece::Type::P, XQPiece::Type::NONE, XQPiece::Type::P, XQPiece::Type::NONE, XQPiece::Type::P,
      XQPiece::Type::NONE, XQPiece::Type::P, XQPiece::Type::NONE, XQPiece::Type::P },
    { XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::NONE,
      XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::NONE, XQPiece::Type::NONE }
};

//point on the board (as opposed to on the window)
//{0,0} is the most upper-left spot on the board where a piece can be placed
struct BoardPoint{
    int x;
    int y;
    
    bool operator==(const BoardPoint&) const = default;
};

struct XQGameMessage{
    enum Flag : uint8_t{
        MOVE=0, RESTART, QUIT
    };
    Flag flag;
    
    struct MessagePoint{
        uint8_t x_from;
        uint8_t y_from;
        uint8_t x_to;
        uint8_t y_to;
        
        MessagePoint() = default;
        MessagePoint(BoardPoint from, BoardPoint to)
            : x_from(from.x), y_from(from.y), x_to(to.x), y_to(to.y){}
        BoardPoint from() const { return {x_from, y_from}; }
        BoardPoint to() const { return {x_to, y_to}; }
    } points;
};

using XQGameBoardState_t = XQPiece* [XQBOARD_SPOTS_HEIGHT][XQBOARD_SPOTS_WIDTH];
using XQGameConstBoardState_t = const XQPiece* [XQBOARD_SPOTS_HEIGHT][XQBOARD_SPOTS_WIDTH];
using BoardPointArray_t = std::array<BoardPoint, XQBOARD_SPOTS_HEIGHT*XQBOARD_SPOTS_WIDTH>;

class XQGame{
private:
    static constexpr sockaddr_in zeroedSockAddr = {};
    
public:
    enum GameType{
        OFFLINE, SERVER, CLIENT
    };
    
    static consteval BoardPointArray_t boardPointArray(){
        BoardPointArray_t res;
        for(int x=0; x<XQBOARD_SPOTS_WIDTH; ++x){
            for(int y=0; y<XQBOARD_SPOTS_HEIGHT; ++y){
                res[y*XQBOARD_SPOTS_WIDTH + x] = {x, y};
            }
        }
        return res;
        //return boardPointArray_;
    }
    
private:
    /* -- NETWORK PLAY -- */
    GameType gameType;
    //-1 in offline games; fd of the socket
    int mySocketfd;
    //unused socket addresses will be zeroed
    sockaddr_in serverSockAddr = zeroedSockAddr;
    //flags
    std::atomic<bool> connected = false;
    //accessing network mother task/thread
    std::future<void> networkTask;
    
    //where to store the message sent by this game
    XQGameMessage lastSentMessage;
    
    /* -- GAME STATE -- */
    XQPiece pieces[32];         //stores pieces involved in the game
    //array of the board, containing pointers to pieces in `pieces`.
    //first index is y, second is x.
    XQGameBoardState_t boardState;
    //XQPiece* boardState[XQBOARD_SPOTS_HEIGHT][XQBOARD_SPOTS_WIDTH];
    
    //location of selected piece
    std::optional<BoardPoint> selectedPieceSpot;
    //current turn
    bool redTurn;
    //whether game has been won
    std::optional<XQPiece::Side> gameWon;
    
    //whether turn's general is in check; not supposed to be accessed directly
    //read value through `inCheck(side)` and update value with `updateCheckness`
    std::map<XQPiece::Side, bool> inCheckMap;
    //whether current turn is in checkmate or stalemate; not supposed to be accessed directly
    //read value through `inMate` and update value with `updateCheckness`
    std::map<XQPiece::Side, bool> inMateMap;
    
    //whether game is over; will draw end game screen if so
    bool gameOver = false;
    //whether game is to restart after exiting gameOver
    bool willRestart = false;
    
    /* -- DISPLAY -- */
    int winWidth;
    int winHeight;
    int boardWidth;
    //stores coordinates of line intersections/where pieces can be placed
    //as with others, first index is y, second is x
    SDL_Point spotCoords[XQBOARD_SPOTS_HEIGHT][XQBOARD_SPOTS_WIDTH];
    //used for piece rendering
    std::vector<SDL_Point> fillPoints;
    std::vector<SDL_Point> edgePoints;
    //piece display
    std::optional<BoardPoint> highlightedPieceSpot;
    int pieceFontSize;
    //overlay message; drawn on overlay unless otherwise specified
    std::u16string overlayMessage;
    //SDL
    TTF_Font* pieceFont;
    SDL_Window* win_;
    SDL_Renderer* renderer_;
    
public:
    XQGame(int ww, int wh, int bw);
    XQGame(int ww, int wh, int bw, in_port_t port);
    XQGame(int ww, int wh, int bw, in_addr_t ip, in_port_t port);
    
    //rule of three
    XQGame(XQGame& other) = delete;
    XQGame(XQGame&& other) = delete;
    void operator=(XQGame& other) = delete;
    void operator=(XQGame&& other) = delete;
    ~XQGame();
    
    SDL_Window* win(){ return win_; }
    SDL_Renderer* renderer(){ return renderer_; }
    
    bool hasSelection(){ return selectedPieceSpot.has_value(); }
    
    void run(SDL_Event& event);
    
    void drawBoard();
    
    void mouseMovedEvent(SDL_Point coords);
    void mouseDownEvent(SDL_Point coords);
    
    /* helper functions for network-related tasks */
    static XQGameMessage makeMessage(XQGameMessage::Flag type, BoardPoint from, BoardPoint to);
    static SDL_Event makeEvent(XQGameMessage message);
    static XQGameMessage messageFromEvent(SDL_Event);
    
private:
    /* helper functions for network-related tasks */
    bool myTurn(){ return !gameOver && ((redTurn && gameType==SERVER) || (!redTurn && gameType==CLIENT) || gameType==OFFLINE); }
    //tasks and thread functions
    //returns whether the accept succeeded or not
    bool taskAccept();
    //returns message passed by other socket, QUIT if recieve did not succeed
    XQGameMessage taskRecieve();
    void networkMotherTask();
    
    /* helper functions for drawBoard */
    void drawPiece(BoardPoint spot);
    void drawBlankBoard();
    //draw overlay
    void drawOverlay(){ drawOverlay(overlayMessage, COLOR_WHITE); }
    void drawOverlay(std::u16string overlayMsg, SDL_Color color);
    
    XQPiece* pieceAt(BoardPoint spot) const { return boardState[spot.y][spot.x]; }
    bool noPieceAt(BoardPoint spot) const { return boardState[spot.y][spot.x]==nullptr; }
    SDL_Point pieceSpot(BoardPoint spot) const { return spotCoords[spot.y][spot.x]; }
    
    /* helper function for constuctor and restart */
    void setDefault();
    void endGame(bool restarting, std::string overlayMsg="");
    
    /* helper function for mouse functions */
    //returns the boardPoint corresponding to the window coords
    std::optional<BoardPoint> boardSpotFromCoords(SDL_Point coords) const;
    //returns a boardPoint only if there is a piece at the window coords
    std::optional<BoardPoint> pieceAtCoords(SDL_Point coords) const;
    
    //return pointer to selected piece, or nullptr if no such piece
    XQPiece* selectedPiece() const { return (selectedPieceSpot ? pieceAt(*selectedPieceSpot) : nullptr); }
    //checks game logic, returns true if move is valid, false otherwise
    bool validMove(BoardPoint from, BoardPoint to) const;
    //does the actual moving of the piece
    void movePiece(BoardPoint from, BoardPoint to);
    //checks move and moves piece in one
    void checkAndMovePiece(BoardPoint from, BoardPoint to){ if(validMove(from, to)){ movePiece(from, to); } }
    //check if `side` is in check
    bool inCheck(XQPiece::Side side) const;
    //check if `side` is in checkmate or stalemate
    bool inMate(XQPiece::Side side) const;
    //updates if sides are in check, checkmate, or stalemate
    void updateCheckness();
    
    /* helper functions for validMove */
    //returns true if the given BoardPoint is within the corresponding palace
    //functions same regardless of temp boardState or not, as palace position is static
    bool inRightPalace(BoardPoint spot, XQPiece::Side side) const;
    //returns which side's turn it is
    XQPiece::Side currentSide() const { if(redTurn){ return XQPiece::Side::RED; } return XQPiece::Side::BLACK; }
    //returns which side's turn it is
    XQPiece::Side currentOppSide() const { if(redTurn){ return XQPiece::Side::BLACK; } return XQPiece::Side::RED; }
    
    
    /* helper functions, some for temporary boardStates, involved in move checking */
    const XQPiece* pieceAt(BoardPoint spot, const XQGameBoardState_t &temp) const
    { return temp[spot.y][spot.x]; }
    bool noPieceAt(BoardPoint spot, const XQGameBoardState_t &temp) const
    { return temp[spot.y][spot.x]==nullptr; }
    BoardPoint generalLocation(XQPiece::Side side, const XQGameBoardState_t &temp) const;
    bool validMove(BoardPoint from, BoardPoint to, const XQGameBoardState_t &temp) const;
    //unlike `inCheck(side)`, this one is uncached and updates every time it is called
    bool inCheck(XQPiece::Side side, const XQGameBoardState_t &temp) const;
    //similarly uncached updating for one side's mateness
    bool inMateCheck(XQPiece::Side side);
};
