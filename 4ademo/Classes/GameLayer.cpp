//
//  GameLayer.cpp
//  4ademo
//
//  Created by Roman Semenov on 13/04/2017.
//
//

#include "GameLayer.hpp"
#include "BulletManager.hpp"

USING_NS_CC;

GameLayer* GameLayer::create()
{
    GameLayer* p = new GameLayer();
    if (p && p->init()) {
        p->autorelease();
        return p;
    }
    delete p;
    return nullptr;
}
// _________________________________________________________________________________________
GameLayer::~GameLayer()
{
    mNeedQuit = true;
    mSleepCondition.notify_one();
    if (mThread)    
        mThread->join();

    CC_SAFE_DELETE(mThread);
    
    mShader->release();
    delete mBulletManager;
}
// _________________________________________________________________________________________
bool GameLayer::init()
{
    if (!Layer::init())
        return false;
    
    mThread = new std::thread(&GameLayer::ThreadFunction, this);
    
    mShader=GLProgramCache::getInstance()->getGLProgram(GLProgram::SHADER_NAME_POSITION_U_COLOR);
    mShader->retain();
    mColorLocation = mShader->getUniformLocation("u_color");
    CHECK_GL_ERROR_DEBUG();
    mPosLocation = mShader->getUniformLocation("u_pointSize");
    CHECK_GL_ERROR_DEBUG();
    
    mBulletManager = new BulletManager();
    createWalls();
    
    auto listener = EventListenerTouchAllAtOnce::create();
    listener->onTouchesBegan = CC_CALLBACK_2(GameLayer::onTouchesBegan, this);
    listener->onTouchesMoved = CC_CALLBACK_2(GameLayer::onTouchesMoved, this);
    listener->onTouchesEnded = CC_CALLBACK_2(GameLayer::onTouchesEnded, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    
    scheduleUpdate();
    return true;
}
// _________________________________________________________________________________________
void GameLayer::createWalls()
{
    Size winSize = Director::getInstance()->getWinSize();
    
    /// создаем рандомом стены
    for (int i = 0; i < 2000; ++i) {
        mWalls.push_back({Vec2(random(0.0f, winSize.width), random(0.0f, winSize.height)),
            Vec2(random(0.0f, winSize.width), random(0.0f, winSize.height))});
    }
}
// _________________________________________________________________________________________
void GameLayer::update(float delta)
{
    mGameTime += delta;
    mBulletManager->update(mWalls, delta);
}
// _________________________________________________________________________________________
void GameLayer::draw(Renderer *renderer, const Mat4 &transform, uint32_t flags)
{
    mCustomCommand.init(_globalZOrder);
    mCustomCommand.func = CC_CALLBACK_0(GameLayer::onDraw, this, transform, flags);
    renderer->addCommand(&mCustomCommand);
}
// _________________________________________________________________________________________
void GameLayer::onDraw(const Mat4 &transform, uint32_t flags)
{
    Director* director = Director::getInstance();
    director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, transform);
    
    //draw
    CHECK_GL_ERROR_DEBUG();
    
    // рисуем стены
    glLineWidth( 1.0f );
    Color4F color = Color4F::RED;
    
    mShader->use();
    mShader->setUniformsForBuiltins();
    mShader->setUniformLocationWith4fv(mColorLocation, (GLfloat*) &color.r, 1);
    
    if (!mWalls.empty()) {
        std::vector<Vec2> walls;
        walls.reserve(mWalls.size() * 2);
        for (const Wall& wall : mWalls) {
            walls.push_back(wall.p1);
            walls.push_back(wall.p2);
        }
        GL::enableVertexAttribs( GL::VERTEX_ATTRIB_FLAG_POSITION );
        glVertexAttribPointer(GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, &walls.front());
        glDrawArrays(GL_LINES, 0, walls.size());
        
        CC_INCREMENT_GL_DRAWN_BATCHES_AND_VERTICES(1, walls.size());
    }

    //рисуем пули
    std::vector<Vec2> bullets = mBulletManager->getBulletsPositions();
    if (!bullets.empty()) {
        color = Color4F::GREEN;

        mShader->setUniformLocationWith4fv(mColorLocation, (GLfloat*) &color.r, 1);
        mShader->setUniformLocationWith1f(mPosLocation, 3);
        
        glVertexAttribPointer(GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, &bullets.front());
        glDrawArrays(GL_POINTS, 0, (GLsizei) bullets.size());
        
        CC_INCREMENT_GL_DRAWN_BATCHES_AND_VERTICES(1, bullets.size());
    }

    //рисуем линию от надатия мышкой
    if (mTouchLine.isVisible) {
        color = Color4F::BLUE;
        mShader->setUniformLocationWith4fv(mColorLocation, (GLfloat*) &color.r, 1);

        glVertexAttribPointer(GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 0, &mTouchLine.p1);
        glDrawArrays(GL_LINES, 0, 2);
        
        CC_INCREMENT_GL_DRAWN_BATCHES_AND_VERTICES(1, 2);
    }
    CHECK_GL_ERROR_DEBUG();
};
// _________________________________________________________________________________________
void GameLayer::ThreadFunction()
{
    std::mutex signalMutex;
    std::unique_lock<std::mutex> signal(signalMutex);
    while (!mNeedQuit) {
        std::vector<BulletData> bData;
        
        mBulletsMutex.lock();
        std::swap(bData, mBulletsRequests);
        mBulletsMutex.unlock();
        
        if (bData.empty()) {
            mSleepCondition.wait(signal);
            continue;
        }
        
        for (const BulletData& data : bData) {
            if (BulletManager::getInstance())
                BulletManager::getInstance()->fire(data);
        }
    }
}
// _________________________________________________________________________________________
#pragma mark -
#pragma mark Touches
void GameLayer::onTouchesBegan(const std::vector<Touch*>& touches, Event *event)
{
    Touch* touch = touches.front();
    mTouchLine.p1 = mTouchLine.p2 = touch->getLocation();
    mTouchLine.isVisible = true;
    mTouchTime = mGameTime;
}
// _________________________________________________________________________________________
void GameLayer::onTouchesMoved(const std::vector<Touch*>& touches, Event *event)
{
    Touch* touch = touches.front();
    mTouchLine.p2 = touch->getLocation();
}
// _________________________________________________________________________________________
void GameLayer::onTouchesEnded(const std::vector<Touch*>& touches, Event *event)
{
    mTouchLine.isVisible = false;
    const float speedCoef = 5.0f;
    
    BulletData data;
    data.pos = mTouchLine.p1;
    data.dir = mTouchLine.p2 - mTouchLine.p1;
    data.speed = mTouchLine.p1.distance(mTouchLine.p2) * speedCoef;
    data.time = mGameTime + mGameTime - mTouchTime;
    data.lifeTime = 4.0f;
    
    mBulletsMutex.lock();
    mBulletsRequests.push_back(data);
    mBulletsMutex.unlock();
    
    mSleepCondition.notify_one();
}

