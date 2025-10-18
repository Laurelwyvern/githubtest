//
//  xiangqigame.hpp
//  xiangqi
//
//  Created by Liana Xie on 7/8/24.
//

#pragma once

class XQPiece{
    
public:
    /*
     "　" - ideographic space character
     帥/將    King        K
     車　     Chariot     R
     馬　     Horse       H
     炮/砲    Cannon      C
     兵/卒    Pawn        P
     仕/士    Advisor     A
     相/象    Elephant    E
    */
    enum Type { K, R, H, C, P, A, E };
    
private:
    bool sideRed_;
    Type type_;
    
    
};

