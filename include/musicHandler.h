#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class MusicHandler {
public:
    struct ExplorationSettings {
        static inline long hoursBetweenTracks;
    };

    struct CombatSettings {
        static inline bool enableSmart;
        static inline bool disableLowThreatCombatMusic;
        static inline double healthCheckMult;
    };

    typedef RE::BSEventNotifyControl (MusicHandler::* FnProcessEvent) (const RE::BSMusicEvent*, RE::BSTEventSource<RE::BSMusicEvent>*);

    static void initialise();

    RE::BSEventNotifyControl ProcessEvent(const RE::BSMusicEvent* a_event, RE::BSTEventSource<RE::BSMusicEvent>* a_eventSource);
private:
    static inline std::unordered_map<uintptr_t, FnProcessEvent> fnHash;
    static inline json tracklists;

    float lastMusic = 0.0f;

    bool isCombatTrack(const RE::BSMusicEvent* a_event);
    bool isExplorationTrack(const RE::BSMusicEvent* a_event);
    bool shouldPlayCombatMusic();
    bool shouldPlayExplorationMusic();

    static void Hook();
};