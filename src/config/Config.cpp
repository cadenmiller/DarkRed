#include <cctype>
#include <cassert>
#include <cstdlib>
#include <string>
#include <fstream>
#include <algorithm>
#include <filesystem>

#include "core/Debug.hpp"
#include "Config.hpp"
#include "Configurable.hpp"
#include "ConfigParser.hpp"



//==============================================================================
// IConfigurable
//==============================================================================

IConfigurable::IConfigurable(CConfig *const pConfig)
    : m_pConfig(pConfig)
{
    assert(m_pConfig);
    m_pConfig->SubscribeEvents(this);
}

IConfigurable::~IConfigurable()
{
    if (m_pConfig) m_pConfig->UnsubscribeEvents(this);
}


//==============================================================================
// CConfig
//==============================================================================

const std::string CConfig::DEFAULT_CONFIG = "\
    # Default configuration\n \
    window={\n \
        width=1920;\n \
        height=1080;\n \
    }\n \
    renderer={\n \
        antialias=1;\n \
    }\n \
";

CConfig::CConfig(const std::string& configPath)
    : m_path(configPath)
{
    EnsureDirectoriesExist();
    ResetIfConfigDoesNotExist();
    ParseInto();
}

CConfig::~CConfig()
{
}

void CConfig::Reset()
{
    std::ofstream config(DEFAULT_CONFIG_PATH, std::ios::out);
    config << DEFAULT_CONFIG;
}

bool CConfig::HasValue(const std::string& name) const noexcept
{
    return m_values.contains(name);
}

int CConfig::GetIntValue(const std::string& name)
{
    
}

void CConfig::SubscribeEvents(const IConfigurable* pConfigurable)
{
    m_subscribers.push_back(pConfigurable);
}

void CConfig::UnsubscribeEvents(const IConfigurable* pConfigurable)
{
    m_subscribers.erase(std::find(m_subscribers.begin(), m_subscribers.end(), pConfigurable));
}

void CConfig::EnsureDirectoriesExist()
{
    /* Ensure directories exist. */
    if (!std::filesystem::exists("config"))
    {
        std::filesystem::create_directory("config");
        Reset();
    }
}

void CConfig::ResetIfConfigDoesNotExist()
{
    if (!std::filesystem::exists(DEFAULT_CONFIG_PATH))
    {
        Reset();
    }
}

void CConfig::ParseInto()
{
    Debug::Log("Opening config: {}\n", m_path);

    /* Open the config file. */
    std::ifstream configFile(m_path, std::ios::in);
    if (configFile.bad()) throw std::runtime_error("Could not open config file.");

    /* Read in the whole config. */
    std::string content((std::istreambuf_iterator<char>(configFile)),
                        (std::istreambuf_iterator<char>()));

    CParser parser{content};
    auto values = parser.Parse();

    for (auto pair : values)
    {
        Debug::Log("Value [{}, {}, {}] \n", pair.first, (int)(pair.second.type), (pair.second.type == EParsedType::eInt) ? std::to_string(pair.second.i) : std::string{pair.second.s});
    }

    Debug::Log("Parsed config: {}\n", m_path);
}


