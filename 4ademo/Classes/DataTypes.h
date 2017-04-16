//
//  DataTypes.h
//  4ademo
//
//  Created by Roman Semenov on 14/04/2017.
//
//

#pragma once

#include "cocos2d.h"

struct BulletData
{
    cocos2d::Vec2 pos;
    cocos2d::Vec2 dir;
    float speed;
    float time;
    float lifeTime;
};
// _________________________________________________________________________________________
struct Bullet
{
    Bullet(const BulletData& data)
    :mData(data){
        mData.dir.normalize();
    }
    
    BulletData mData;
    bool mIsVisible = false;
};

using Bullets = std::deque<Bullet>;
// _________________________________________________________________________________________
struct Wall
{
    cocos2d::Vec2 p1;
    cocos2d::Vec2 p2;
};

using Walls = std::deque<Wall>;
// _________________________________________________________________________________________
struct TouchLine
{
    cocos2d::Vec2 p1;
    cocos2d::Vec2 p2;
    bool isVisible = false;
};
