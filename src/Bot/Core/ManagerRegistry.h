/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_MANAGER_REGISTRY_H
#define _PLAYERBOT_MANAGER_REGISTRY_H

#include <memory>
#include <typeindex>
#include <unordered_map>

#include "Bot/Interface/ITravelManager.h"
#include "Bot/Interface/IRandomBotManager.h"
#include "Bot/Interface/IBotRepository.h"
#include "Bot/Interface/IBetterQuestingConfig.h"
#include "Bot/Interface/ICoreQuestDataMgr.h"
#include "Bot/Interface/IQuestGuideMgr.h"

/**
 * @brief Central registry for all manager interfaces
 *
 * This class provides a single point of access for all manager interfaces,
 * replacing direct singleton access with dependency injection support.
 *
 * Usage in production:
 * @code
 * // Get the global registry
 * auto& registry = ManagerRegistry::Instance();
 *
 * // Access managers through interfaces
 * auto& travelMgr = registry.GetTravelManager();
 * auto& randomBotMgr = registry.GetRandomBotManager();
 * @endcode
 *
 * Usage in tests:
 * @code
 * ManagerRegistry registry;
 * registry.SetTravelManager(std::make_shared<MockTravelManager>());
 * // Use registry in tests...
 * @endcode
 */
class ManagerRegistry
{
public:
    ManagerRegistry() = default;
    ~ManagerRegistry() = default;

    // Singleton access for global registry
    static ManagerRegistry& Instance()
    {
        static ManagerRegistry instance;
        return instance;
    }

    // Prevent copying
    ManagerRegistry(ManagerRegistry const&) = delete;
    ManagerRegistry& operator=(ManagerRegistry const&) = delete;

    // Manager accessors
    ITravelManager& GetTravelManager()
    {
        return *_travelManager;
    }

    IRandomBotManager& GetRandomBotManager()
    {
        return *_randomBotManager;
    }

    IBotRepository& GetBotRepository()
    {
        return *_botRepository;
    }

    IBetterQuestingConfig& GetBetterQuestingConfig()
    {
        return *_betterQuestingConfig;
    }

    ICoreQuestDataMgr& GetCoreQuestDataMgr()
    {
        return *_coreQuestDataMgr;
    }

    IQuestGuideMgr& GetQuestGuideMgr()
    {
        return *_questGuideMgr;
    }

    // Const accessors
    ITravelManager const& GetTravelManager() const
    {
        return *_travelManager;
    }

    IRandomBotManager const& GetRandomBotManager() const
    {
        return *_randomBotManager;
    }

    IBotRepository const& GetBotRepository() const
    {
        return *_botRepository;
    }

    IBetterQuestingConfig const& GetBetterQuestingConfig() const
    {
        return *_betterQuestingConfig;
    }

    ICoreQuestDataMgr const& GetCoreQuestDataMgr() const
    {
        return *_coreQuestDataMgr;
    }

    IQuestGuideMgr const& GetQuestGuideMgr() const
    {
        return *_questGuideMgr;
    }

    // Manager setters for dependency injection
    void SetTravelManager(std::shared_ptr<ITravelManager> manager)
    {
        _travelManager = std::move(manager);
    }

    void SetRandomBotManager(std::shared_ptr<IRandomBotManager> manager)
    {
        _randomBotManager = std::move(manager);
    }

    void SetBotRepository(std::shared_ptr<IBotRepository> repository)
    {
        _botRepository = std::move(repository);
    }

    void SetBetterQuestingConfig(std::shared_ptr<IBetterQuestingConfig> config)
    {
        _betterQuestingConfig = std::move(config);
    }

    void SetCoreQuestDataMgr(std::shared_ptr<ICoreQuestDataMgr> manager)
    {
        _coreQuestDataMgr = std::move(manager);
    }

    void SetQuestGuideMgr(std::shared_ptr<IQuestGuideMgr> manager)
    {
        _questGuideMgr = std::move(manager);
    }

    // Check if managers are initialized
    bool HasTravelManager() const { return _travelManager != nullptr; }
    bool HasRandomBotManager() const { return _randomBotManager != nullptr; }
    bool HasBotRepository() const { return _botRepository != nullptr; }
    bool HasBetterQuestingConfig() const { return _betterQuestingConfig != nullptr; }
    bool HasCoreQuestDataMgr() const { return _coreQuestDataMgr != nullptr; }
    bool HasQuestGuideMgr() const { return _questGuideMgr != nullptr; }

    bool IsInitialized() const
    {
        return _travelManager && _randomBotManager && _botRepository;
    }

    // Generic template-based access (for extensibility)
    template<typename T>
    T& Get();

    template<typename T>
    void Register(std::shared_ptr<T> manager);

private:
    std::shared_ptr<ITravelManager> _travelManager;
    std::shared_ptr<IRandomBotManager> _randomBotManager;
    std::shared_ptr<IBotRepository> _botRepository;
    std::shared_ptr<IBetterQuestingConfig> _betterQuestingConfig;
    std::shared_ptr<ICoreQuestDataMgr> _coreQuestDataMgr;
    std::shared_ptr<IQuestGuideMgr> _questGuideMgr;

    // Generic storage for additional managers
    std::unordered_map<std::type_index, std::shared_ptr<void>> _managers;
};

// Template specializations
template<>
inline ITravelManager& ManagerRegistry::Get<ITravelManager>()
{
    return GetTravelManager();
}

template<>
inline IRandomBotManager& ManagerRegistry::Get<IRandomBotManager>()
{
    return GetRandomBotManager();
}

template<>
inline IBotRepository& ManagerRegistry::Get<IBotRepository>()
{
    return GetBotRepository();
}

template<>
inline void ManagerRegistry::Register<ITravelManager>(std::shared_ptr<ITravelManager> manager)
{
    SetTravelManager(std::move(manager));
}

template<>
inline void ManagerRegistry::Register<IRandomBotManager>(std::shared_ptr<IRandomBotManager> manager)
{
    SetRandomBotManager(std::move(manager));
}

template<>
inline void ManagerRegistry::Register<IBotRepository>(std::shared_ptr<IBotRepository> repository)
{
    SetBotRepository(std::move(repository));
}

template<>
inline IBetterQuestingConfig& ManagerRegistry::Get<IBetterQuestingConfig>()
{
    return GetBetterQuestingConfig();
}

template<>
inline ICoreQuestDataMgr& ManagerRegistry::Get<ICoreQuestDataMgr>()
{
    return GetCoreQuestDataMgr();
}

template<>
inline IQuestGuideMgr& ManagerRegistry::Get<IQuestGuideMgr>()
{
    return GetQuestGuideMgr();
}

template<>
inline void ManagerRegistry::Register<IBetterQuestingConfig>(std::shared_ptr<IBetterQuestingConfig> config)
{
    SetBetterQuestingConfig(std::move(config));
}

template<>
inline void ManagerRegistry::Register<ICoreQuestDataMgr>(std::shared_ptr<ICoreQuestDataMgr> manager)
{
    SetCoreQuestDataMgr(std::move(manager));
}

template<>
inline void ManagerRegistry::Register<IQuestGuideMgr>(std::shared_ptr<IQuestGuideMgr> manager)
{
    SetQuestGuideMgr(std::move(manager));
}

// Convenience macro for accessing the global registry
#define sManagerRegistry ManagerRegistry::Instance()

#endif
