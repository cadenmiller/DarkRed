#include "Core/Log.hpp"
#include "Input/InputSystem.hpp"

#include <SDL2/SDL.h>

namespace bl
{

void InputSystem::registerAction(const std::string& name, std::vector<Key> keys)
{
    m_actionsEvents.emplace_back(InputAction{name, keys});

    // Add the keys to the key event
    for (Key key : m_actionsEvents.back().keys)
    {
        m_usedKeys.set(SDL_GetScancodeFromKey((SDL_KeyCode)key));
    }
}

void InputSystem::registerAxis(const std::string& name, std::vector<std::pair<Key, float>> axes)
{
    m_axisEvents.emplace_back(InputAxis{name, axes});

        // Add the keys to the key event
    for (auto pair : m_axisEvents.back().keys)
    {
        m_usedKeys.set(SDL_GetScancodeFromKey((SDL_KeyCode)pair.first));
    }
}

void InputSystem::registerController(InputController& controller)
{
    m_inputControllers.push_back(&controller);
}

void InputSystem::poll()
{
    
    // Pump the events to get the latest states
    SDL_PumpEvents();

    processKeyboard();

    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_WINDOWEVENT:break;   
        }
    }
}

void InputSystem::processKeyboard()
{

    // Get the current keystate.
    int keystateCount = 0;
    const uint8_t* pKeystate = SDL_GetKeyboardState(&keystateCount);

    if (!pKeystate)
    {
        Logger::Error("Could not get the SDL keystate!");
        return;
    }

    // Iterate through the keystates and activate events/axis on down.
    for (int keystateIndex = 0; keystateIndex < keystateCount; keystateIndex++)
    {
        // Skip unbound keys.
        if (!m_usedKeys[keystateIndex]) continue;

        const bl::Key key = (Key)SDL_GetKeyFromScancode((SDL_Scancode)keystateIndex);

        // Activate an action event.
        actionEvent(key, (InputEvent)pKeystate[keystateIndex]);

        // Activate an axis only if the key is down.
        if (pKeystate[keystateIndex])
        {
            axisEvent(key, 1.0f);
        }
    }
}

void InputSystem::actionEvent(Key key, InputEvent event)
{
    for (InputAction& action : m_actionsEvents)
    {
        if (std::find(action.keys.begin(), action.keys.end(), key) == action.keys.end()) continue;
        for (InputController* pInputController : m_inputControllers)
        {
            pInputController->onActionEvent(action.name, event);
        }
    }
}

void InputSystem::axisEvent(Key key, float axisValue)
{
    for (InputAxis& axis : m_axisEvents)
    {
        auto keyPair = std::find_if(axis.keys.begin(), axis.keys.end(), [key](auto& pair){ return pair.first == key; });
        if (keyPair == axis.keys.end()) continue;
        for (InputController* pInputController : m_inputControllers)
        {
            pInputController->onAxisEvent(axis.name, axisValue * (*keyPair).second);
        }
    }
}

} // namespace bl