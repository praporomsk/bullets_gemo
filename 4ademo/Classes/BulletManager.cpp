//
//  BulletManager.cpp
//  4ademo
//
//  Created by Roman Semenov on 14/04/2017.
//
//

#include "BulletManager.hpp"

USING_NS_CC;

BulletManager* BulletManager::mInstance = nullptr;

BulletManager* BulletManager::getInstance() {
    return mInstance;
}
// _________________________________________________________________________________________
BulletManager::BulletManager()
{
    CCASSERT(!mInstance, "BulletManager уже создан");
    mInstance = this;
}
// _________________________________________________________________________________________
BulletManager::~BulletManager() {
    mInstance = nullptr;
}
// _________________________________________________________________________________________
void BulletManager::update(Walls& walls, float delta)
{
    mGameTime += delta;

    //добавляем пули пришедщие из другова потока
    mBulletsMutex.lock();
    mBullets.insert(mBullets.end(), mBulletsFromThread.begin(), mBulletsFromThread.end());
    mBulletsFromThread.clear();
    mBulletsMutex.unlock();
    
    for (auto bIt = mBullets.begin(); bIt != mBullets.end(); ) {
        if (bIt->mIsVisible) {
            float length = bIt->mData.speed * delta;
            updateBullet(*bIt, walls, length);
        }else if (bIt->mData.time <= mGameTime){
            bIt->mIsVisible = true;
        }
        
        // удаляем пулю если время жизни закончилось
        if (bIt->mData.time + bIt->mData.lifeTime <  mGameTime) {
            bIt = mBullets.erase(bIt);
        }else{
            ++bIt;
        }
    }
}
// _________________________________________________________________________________________
void BulletManager::updateBullet(Bullet& b, Walls& walls, float length)
{
    // пуля за один update может попасть в несколько стен
    // поэтому будет искать стены пока не кончиться растояние, которое пуля должна пролететь
    while (length) {
        //проверяем на попадание в стены
        const Vec2 lastPos = b.mData.pos;
        Vec2 nextPos = lastPos + b.mData.dir * length;
        
        float distance = FLT_MAX;
        auto removeIt = walls.end();
        for (auto iter = walls.begin(); iter != walls.end(); ++iter) {
            // ищем пересчения со стеной
            float S, T;
            if (Vec2::isLineIntersect(iter->p1, iter->p2, lastPos, nextPos, &S, &T)
                && (S >= 0.0f && S <= 1.0f && T >= 0.0f && T <= 1.0f))
            {
                Vec2 cross;
                cross.x = iter->p1.x + S * (iter->p2.x - iter->p1.x);
                cross.y = iter->p1.y + S * (iter->p2.y - iter->p1.y);
                // выбираем ближайшую стену
                if (lastPos.distance(cross) < distance){
                    removeIt = iter;
                    //урезаем пройденный путь до стены
                    nextPos = cross;
                }
            }
        }
        
        if (removeIt != walls.end()){
            // находим нормаль
            Vec2 p = removeIt->p2 - removeIt->p1;
            Vec2 normale = Vec2(p.y, -p.x);
            normale.normalize();
            // расчитываем направление после отражения
            b.mData.dir = b.mData.dir - 2 * Vec2::dot(b.mData.dir, normale) * normale;
            walls.erase(removeIt);
            length -= lastPos.distance(nextPos);
        }else{
            // т.к стена не найдена мы прошли все расстояние
            length = 0;
        }
        
        b.mData.pos = nextPos;
    }
}
// _________________________________________________________________________________________
void BulletManager::fire(const BulletData& data)
{
    mBulletsMutex.lock();
    mBulletsFromThread.push_back(Bullet(data));
    mBulletsMutex.unlock();
}
// _________________________________________________________________________________________
std::vector<cocos2d::Vec2> BulletManager::getBulletsPositions() const
{
    std::vector<cocos2d::Vec2> vec;
    for (const Bullet& b : mBullets) {
        if (!b.mIsVisible)
            continue;
        
        vec.push_back(b.mData.pos);
    }
    return vec;
}
