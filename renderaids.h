//
//  renderaids.h
//  helloworldsdl
//
//  Created by Liana Xie on 7/12/24.
//
#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <iostream>

inline void drawLineWithPoints(SDL_Renderer* renderer, SDL_Point p1, SDL_Point p2){
    SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
}

inline void setRendererColor(SDL_Renderer* renderer, SDL_Color c){
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}

//many thanks to some people on this thread and wikipedia's midpoint circle algorithm article
//https://discourse.libsdl.org/t/query-how-do-you-draw-a-circle-in-sdl2-sdl2/33379
inline void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius, std::vector<SDL_Point>& points){
    
    points.clear();
    
    const int32_t diameter = (radius * 2);

    int32_t x = (radius - 1);
    int32_t y = 0;
    int32_t tx = 1;
    int32_t ty = 1;
    int32_t error = (tx - diameter);
    
    while (x >= y)
    {
        // Each of the following renders an octant of the circle
        points.push_back({centerX + x, centerY - y});
        points.push_back({centerX + x, centerY + y});
        points.push_back({centerX - x, centerY - y});
        points.push_back({centerX - x, centerY + y});
        points.push_back({centerX + y, centerY - x});
        points.push_back({centerX + y, centerY + x});
        points.push_back({centerX - y, centerY - x});
        points.push_back({centerX - y, centerY + x});

        if (error <= 0){
            ++y;
            error += ty;
            ty += 2;
        }
        if (error > 0){
            --x;
            tx += 2;
            error += (tx - diameter);
        }
    }
    
    SDL_RenderDrawPoints(renderer, points.data(), static_cast<int>(points.size()));
}

//helper function for drawCircleFilled
inline void addCircleFilledSlices(int centerX, int offsetX, int upperY, int lowerY, std::vector<SDL_Point>& fillPoints){
    for(++upperY; upperY<lowerY; ++upperY){
        fillPoints.push_back({centerX-offsetX, upperY});
        if(offsetX!=0){
            fillPoints.push_back({centerX+offsetX, upperY});
        }
    }
}
inline void drawCircleFilled(SDL_Renderer* renderer, int centerX, int centerY, int radius, SDL_Color fillColor, std::vector<SDL_Point>& edgePoints, std::vector<SDL_Point>& fillPoints){
    
    SDL_Color oldColor = {255, 255, 255, 255};
    SDL_GetRenderDrawColor(renderer, &(oldColor.r), &(oldColor.g), &(oldColor.b), &(oldColor.a));
    //std::cout << oldColor.r << std::endl;
    
    edgePoints.clear();
    fillPoints.clear();
    
    const int32_t diameter = (radius * 2);
    
    int32_t x = (radius - 1);
    int32_t y = 0;
    int32_t tx = 1;
    int32_t ty = 1;
    int32_t error = (tx - diameter);
    bool xChanged = false;
    
    while (x >= y)
    {
        // Each of the following renders an octant of the circle edge
        edgePoints.push_back({centerX + x, centerY - y});
        edgePoints.push_back({centerX + x, centerY + y});
        edgePoints.push_back({centerX - x, centerY - y});
        edgePoints.push_back({centerX - x, centerY + y});
        edgePoints.push_back({centerX + y, centerY - x});
        edgePoints.push_back({centerX + y, centerY + x});
        edgePoints.push_back({centerX - y, centerY - x});
        edgePoints.push_back({centerX - y, centerY + x});
        
        if(xChanged && x!=y){
            addCircleFilledSlices(centerX, x, centerY-y, centerY+y, fillPoints);
            xChanged = false;
        }
        addCircleFilledSlices(centerX, y, centerY-x, centerY+x, fillPoints);
                
        if (error <= 0){
            ++y;
            error += ty;
            ty += 2;
        }
        if (error > 0){
            --x;
            tx += 2;
            error += (tx - diameter);
            xChanged = true;
        }
    }
    
    setRendererColor(renderer, fillColor);
    SDL_RenderDrawPoints(renderer, fillPoints.data(), static_cast<int>(fillPoints.size()) );
    
    
    setRendererColor(renderer, oldColor);
    SDL_RenderDrawPoints(renderer, edgePoints.data(), static_cast<int>(edgePoints.size()) );
}

inline void drawTextOnRect(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Rect* rect){
    //SDL_RenderDrawRect(renderer, rect);
    SDL_RenderCopy(renderer, texture, NULL, rect);
    //std::cout << "drew character\n";
}

inline int getTextureWidth(SDL_Texture* texture) {
    int width;
    SDL_QueryTexture(texture, NULL, NULL, &width, NULL);
    return width;
}

inline int getTextureHeight(SDL_Texture* texture) {
    int height;
    SDL_QueryTexture(texture, NULL, NULL, NULL, &height);
    return height;
}

