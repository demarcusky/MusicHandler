#pragma once

class MusicHandler {
public:
    struct ExplorationSettings {
        static long hoursBetweenTracks;
    };

    struct CombatSettings {
        static bool enableSmart;
        static double healthCheckMult;
    };

    typedef RE::BSEventNotifyControl (MusicHandler::* FnProcessEvent) (const RE::BSMusicEvent*, RE::BSTEventSource<RE::BSMusicEvent>*);

    static void initialise();

    RE::BSEventNotifyControl ProcessEvent(const RE::BSMusicEvent* a_event, RE::BSTEventSource<RE::BSMusicEvent>* a_eventSource);
private:
    static std::unordered_map<uintptr_t, FnProcessEvent> fnHash;

    static void Hook();
};