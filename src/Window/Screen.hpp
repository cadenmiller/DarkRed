#pragma once

#include "Math/Vector2.hpp"
#include "Window/VideoMode.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <string>

namespace bl {

class Screen {
public:
    ~Screen();

    const std::vector<VideoMode>& getVideoModes();
    VideoMode getDesktopMode();

    unsigned int screen;
    std::string name;
    Extent2D resolution;
    Vector2f contentScale;
    unsigned int refreshRate;

private:
    Screen(unsigned int screen, const std::string& name, Extent2D resolution, Vector2f contentScale, unsigned int refreshRate, const std::vector<VideoMode>& videoModes, VideoMode desktopMode);

    GLFWmonitor*            m_pMonitor;
    std::vector<VideoMode>  m_videoModes;
    VideoMode               m_desktopMode;

//==== Static ====//
public: 
    static Screen getScreen(unsigned int index);
    static std::vector<Screen> getScreens();
};

} /* namespace bl */