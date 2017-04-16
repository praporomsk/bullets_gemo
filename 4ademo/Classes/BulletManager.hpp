//
//  BulletManager.hpp
//  4ademo
//
//  Created by Roman Semenov on 14/04/2017.
//
//

#pragma once

#include "DataTypes.h"

class Bullet;

class BulletManager
{
public:
    /// получаем инстанс обьекта
    static BulletManager* getInstance();
    /// обновлям пули с проверяем колизии со стенами
    void update(Walls& walls, float delta);
    /// добавляем пули (не из главного потока!)
    void fire(const BulletData& bullet);
    /// получить позиции видимых пуль
    std::vector<cocos2d::Vec2> getBulletsPositions() const;
protected:
    /// движение пули и столкновение со стенами
    void updateBullet(Bullet& b, Walls& walls, float length);

    Bullets mBullets;
    Bullets mBulletsFromThread;
    float mGameTime = 0;
    /// можно использовать std::recursive_mutex если метод
    /// будет вызываться из нескольких потоков
    std::mutex mBulletsMutex;
private:
    friend class GameLayer;
    
    BulletManager();
    virtual ~BulletManager();
    
    static BulletManager* mInstance;
};
// _________________________________________________________________________________________
