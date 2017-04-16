//
//  GameLayer.hpp
//  4ademo
//
//  Created by Roman Semenov on 13/04/2017.
//
//

#pragma once

#include "cocos2d.h"
#include "DataTypes.h"

#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <string>
#include <unordered_map>
#include <functional>

class BulletManager;

class GameLayer : public cocos2d::Layer
{
public:
    static GameLayer* create();
    ~GameLayer() override;
    
    bool init() override;
    void update(float delta) override;
    void draw(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, uint32_t flags) override;
protected:
    void createWalls();
    void onDraw(const cocos2d::Mat4 &transform, uint32_t flags);
    
    void onTouchesBegan(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event *event)override;
    void onTouchesMoved(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event *event)override;
    void onTouchesEnded(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event *event)override;
protected:
    cocos2d::CustomCommand mCustomCommand;
    
    BulletManager* mBulletManager;
    Walls mWalls;
    TouchLine mTouchLine;
    
    cocos2d::GLProgram* mShader;
    GLint mColorLocation;
    GLint mPosLocation;
    
    float mGameTime = 0.0f;
    float mTouchTime = 0.0f;
    
private:
    /// метод выполняемый в отдельном потоке
    void ThreadFunction();
    
    std::mutex mBulletsMutex;
    std::condition_variable mSleepCondition;
    std::thread* mThread = nullptr;
    bool mNeedQuit = false;
    std::vector<BulletData> mBulletsRequests;
};
