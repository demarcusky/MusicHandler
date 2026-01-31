#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class MusicHandler {
public:
    struct ExplorationSettings {
        static long hoursBetweenTracks;
    };

    struct CombatSettings {
        static bool enableSmart;
        static bool disableLowThreatCombatMusic;
        static double healthCheckMult;
    };

    typedef RE::BSEventNotifyControl (MusicHandler::* FnProcessEvent) (const RE::BSMusicEvent*, RE::BSTEventSource<RE::BSMusicEvent>*);

    static void initialise();

    RE::BSEventNotifyControl ProcessEvent(const RE::BSMusicEvent* a_event, RE::BSTEventSource<RE::BSMusicEvent>* a_eventSource);
private:
    static std::unordered_map<uintptr_t, FnProcessEvent> fnHash;
    static json tracklists;

    bool isCombatTrack(const RE::BSMusicEvent* a_event);
    bool isExplorationTrack(const RE::BSMusicEvent* a_event);

    static void Hook();
};