#ifndef _IMGUI_IDLE_OPTIMIZER_H__
#define _IMGUI_IDLE_OPTIMIZER_H__

#include <chrono>
#include <functional>

#include "SDL.h"

class ImGuiIdleOptimizer {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double>;
    
    // 构造函数
    ImGuiIdleOptimizer(float idle_timeout = 1.0f, float idle_fps = 9.0f, float normal_fps = 60.0f) 
        : m_lastInputTime(Clock::now())
        , m_idleTimeout(idle_timeout)  // 1秒空闲超时
        , m_normalFrameRate(normal_fps)  // 正常帧率
        , m_idleFrameRate(idle_fps)     // 空闲帧率
        , m_currentFrameRate(60.0f)
        , m_isIdle(false)
    {
        m_delay_idle_ = 1000.0f / idle_fps;
        m_delay_normal_ = 1000.0f / normal_fps;
        last_tick = Clock::now();
    }
    
    // 设置空闲超时时间（秒）
    void setIdleTimeout(double seconds) {
        m_idleTimeout = seconds;
    }
    
    // 设置帧率
    void setFrameRates(float normalFrameRate, float idleFrameRate) {
        m_normalFrameRate = normalFrameRate;
        m_idleFrameRate = idleFrameRate;
        m_delay_idle_ = 1000.0f / m_idleFrameRate;
        m_delay_normal_ = 1000.0f / m_normalFrameRate;
    }
    
    // 在每帧开始时调用，用于检测输入和更新帧率
    void beginFrame() {
        auto now = Clock::now();
        Duration idleTime = now - m_lastInputTime;
        
        bool wasIdle = m_isIdle;
        m_isIdle = (idleTime.count() >= m_idleTimeout);
        
        // 如果空闲状态发生变化，更新帧率
        if (wasIdle != m_isIdle) {
            m_currentFrameRate = m_isIdle ? m_idleFrameRate : m_normalFrameRate;
            onFrameRateChanged(m_currentFrameRate);
        }

        now = Clock::now();
        Uint32 delta_ms = Duration(now - last_tick).count() * 1000;
        int delay_ms = m_isIdle ? m_delay_idle_ : m_delay_normal_;
        if (delay_ms > delta_ms) {
            SDL_Delay(delay_ms - delta_ms);
        }
        last_tick = Clock::now();
    }
    
    // 在检测到用户输入时调用
    void onUserInput() {
        m_lastInputTime = Clock::now();
        
        // 如果当前处于空闲状态，立即切换到正常帧率
        if (m_isIdle) {
            m_isIdle = false;
            m_currentFrameRate = m_normalFrameRate;
            onFrameRateChanged(m_currentFrameRate);
        }
    }
    
    // 获取当前帧率
    float getCurrentFrameRate() const {
        return m_currentFrameRate;
    }
    
    // 检查当前是否处于空闲状态
    bool isIdle() const {
        return m_isIdle;
    }
    
    // 设置帧率变化回调
    void setFrameRateCallback(std::function<void(float)> callback) {
        m_frameRateCallback = callback;
    }
    
    // 手动重置空闲计时器
    void resetIdleTimer() {
        m_lastInputTime = Clock::now();
    }
    
private:
    void onFrameRateChanged(float newFrameRate) {
        if (m_frameRateCallback) {
            m_frameRateCallback(newFrameRate);
        }
    }
    
private:
    TimePoint m_lastInputTime;
    double m_idleTimeout;      // 空闲超时时间（秒）
    float m_normalFrameRate;   // 正常帧率
    float m_idleFrameRate;     // 空闲帧率
    float m_currentFrameRate;  // 当前帧率
    bool m_isIdle;             // 是否处于空闲状态
    Uint32 m_delay_normal_;
    Uint32 m_delay_idle_;
    TimePoint last_tick;
    std::function<void(float)> m_frameRateCallback;
};

#endif