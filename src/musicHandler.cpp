#include <SimpleIni.h>
#include <regex>

#include "musicHandler.h"

bool shouldPlayCombatMusic() {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto processLists = RE::ProcessLists::GetSingleton();

    auto totalHealth = 0.0f;
    bool spotted = false;
    for (auto& actorHandle : processLists->highActorHandles) {
        auto actor = actorHandle.get();
        if (!actor) continue;
        
        auto actorPtr = actor.get();
        if (!actorPtr || actorPtr->IsDead() || !actorPtr->IsHostileToActor(player)) continue;
        
        // Check if actor is in combat with player
        auto combatTarget = actorPtr->GetActorRuntimeData().currentCombatTarget.get();
        if (combatTarget.get() != player) continue;

        float health = actorPtr->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
        totalHealth += health;

        bool lastSeen = true;
        if (player->HasLineOfSight(actorPtr, lastSeen)) {
            spotted = true;
        }
    }

    if (MusicHandler::CombatSettings::disableLowThreatCombatMusic) {
        // Disable music if the combined health of all current enemies if less than the player's health multiplied.
        if (totalHealth < (player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth) * MusicHandler::CombatSettings::healthCheckMult)) {
            return false;
        }
    }

    if (MusicHandler::CombatSettings::enableSmart) {
        // disable music until player spots enemy, is hit, or hits enemy
        if (!spotted && !player->HasBeenAttacked() && !player->IsAttacking()) return false;
    }
    return true;
}

bool MusicHandler::isCombatTrack(const RE::BSMusicEvent* a_event) {
    RE::BGSMusicType* mt = dynamic_cast<RE::BGSMusicType*>(a_event->musicType);
    std::string editorId = mt->GetFormEditorID();

    std::vector<std::string> tracks = tracklists["Combat"];

    for (auto& pattern : tracks) {
        auto r = std::regex(pattern, std::regex::icase);
        if (std::regex_search(editorId, r)) {
            return true;
        }
    }
    return false;
}

bool MusicHandler::isExplorationTrack(const RE::BSMusicEvent* a_event) {
    RE::BGSMusicType* mt = dynamic_cast<RE::BGSMusicType*>(a_event->musicType);
    std::string editorId = mt->GetFormEditorID();

    std::vector<std::string> tracks = tracklists["Exploration"];

    for (auto& pattern : tracks) {
        auto r = std::regex(pattern, std::regex::icase);
        if (std::regex_search(editorId, r)) {
            return true;
        }
    }
    return false;
}

RE::BSEventNotifyControl MusicHandler::ProcessEvent(const RE::BSMusicEvent* a_event, RE::BSTEventSource<RE::BSMusicEvent>* a_eventSource) {
    if (isCombatTrack(a_event)) {
        if (shouldPlayCombatMusic()) {
            return RE::BSEventNotifyControl::kContinue;
        } else {
            return RE::BSEventNotifyControl::kStop;
        }
    } else if (isExplorationTrack(a_event)) {
        
    }
    
    FnProcessEvent fn = fnHash.at(*(uintptr_t*)this);
    if (fn) (this->*fn)(a_event, a_eventSource);
}

void MusicHandler::initialise() {
    constexpr auto path = L"Data/SKSE/Plugins/MusicHandler.ini";

    CSimpleIniA ini;
    ini.SetUnicode();
    ini.LoadFile(path);
    MusicHandler::ExplorationSettings::hoursBetweenTracks = ini.GetLongValue("Exploration", "kHoursBetweenTracks", 0);
    MusicHandler::CombatSettings::enableSmart = ini.GetBoolValue("Combat", "bEnableSmart", true);
    MusicHandler::CombatSettings::disableLowThreatCombatMusic = ini.GetBoolValue("Combat", "bDisableLowThreatCombatMusic", true);
    MusicHandler::CombatSettings::healthCheckMult = ini.GetDoubleValue("Combat", "kHealthCheckMult", 1.0);
    ini.SetLongValue("Exploration", "kHoursBetweenTracks", MusicHandler::ExplorationSettings::hoursBetweenTracks);
    ini.SetBoolValue("Combat", "bEnableSmart", MusicHandler::CombatSettings::enableSmart);
    ini.SetBoolValue("Combat", "bDisableLowThreatCombatMusic", MusicHandler::CombatSettings::disableLowThreatCombatMusic);
    ini.SetDoubleValue("Combat", "kHealthCheckMult", MusicHandler::CombatSettings::healthCheckMult);
    (void)ini.SaveFile(path);

    std::ifstream (L"Data\\SKSE\\Plugins\\MusicHandler.json") >> tracklists;

    SKSE::log::info("Data initialised...");

    Hook();
}

void MusicHandler::Hook() {
    REL::Relocation<uintptr_t> vtable{ RE::VTABLE_BSMusicManager[0] };
    FnProcessEvent fn = SKSE::stl::unrestricted_cast<FnProcessEvent>(vtable.write_vfunc(1, &MusicHandler::ProcessEvent));
    fnHash.insert(std::pair<uintptr_t, FnProcessEvent>(vtable.address(), fn));
}