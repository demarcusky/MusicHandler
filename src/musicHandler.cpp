#include <SimpleIni.h>
#include <regex>

#include "musicHandler.h"

bool MusicHandler::shouldPlayCombatMusic() {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto processLists = RE::ProcessLists::GetSingleton();

    auto totalHealth = 0.0f;
    bool spotted = false;
    for (auto& handle : processLists->highActorHandles) {
        auto actor = handle.get().get();
        if (!actor || !actor->Is3DLoaded() || actor->IsDead() || !actor->IsHostileToActor(player)) continue;

        float health = actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
        SKSE::log::info("Enemy Health: {:.1f}", health);
        totalHealth += health;

        bool lastSeen = true;
        if (player->HasLineOfSight(actor, lastSeen)) {
            spotted = true;
        }
    }

    if (MusicHandler::CombatSettings::disableLowThreatCombatMusic) {
        // Disable music if the combined health of all current enemies if less than the player's health multiplied.
        if (totalHealth < (player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth) * MusicHandler::CombatSettings::healthCheckMult)) {
            SKSE::log::info("BLOCK Combat Music: EnemyHealth = {}", totalHealth);
            return false;
        }
    }

    if (MusicHandler::CombatSettings::enableSmart) {
        // disable music until player spots enemy, is hit, or hits enemy
        if (!spotted && !player->HasBeenAttacked() && !player->IsAttacking()) return false;
    }
    return true;
}

bool MusicHandler::shouldPlayExplorationMusic() {
    auto calendar = RE::Calendar::GetSingleton();
    if (!calendar) return true;

    float currTime = calendar->GetHoursPassed();
    float hours = currTime - MusicHandler::lastMusic;

    if (hours < MusicHandler::ExplorationSettings::hoursBetweenTracks) {
        SKSE::log::info("BLOCK Exploration Music after {} hours", hours);
        return false;
    }

    SKSE::log::info("ALLOW Exploration Music after {} hours", hours);
    lastMusic = currTime;
    return true;
}

bool MusicHandler::isCombatTrack(const RE::BSMusicEvent* a_event) {
    RE::BGSMusicType* mt = static_cast<RE::BGSMusicType*>(a_event->musicType);
    if (!mt) {
        SKSE::log::info("Failed to cast...");
    }
    std::string editorId = mt->GetFormEditorID();

    std::vector<std::string> tracks = tracklists["Combat"];
    for (auto& pattern : tracks) {
        auto r = std::regex(pattern, std::regex::icase);
        if (std::regex_search(editorId, r)) {
            SKSE::log::info("IS Combat Track...");
            return true;
        }
    }
    SKSE::log::info("NOT Combat Track...");
    return false;
}

bool MusicHandler::isExplorationTrack(const RE::BSMusicEvent* a_event) {
    RE::BGSMusicType* mt = static_cast<RE::BGSMusicType*>(a_event->musicType);
    if (!mt) {
        SKSE::log::info("Failed to cast...");
    }
    std::string editorId = mt->GetFormEditorID();

    std::vector<std::string> tracks = tracklists["Exploration"];

    for (auto& pattern : tracks) {
        auto r = std::regex(pattern, std::regex::icase);
        if (std::regex_search(editorId, r)) {
            SKSE::log::info("IS Exploration Track...");
            return true;
        }
    }
    SKSE::log::info("NOT Exploration Track...");
    return false;
}

RE::BSEventNotifyControl MusicHandler::ProcessEvent(const RE::BSMusicEvent* a_event, RE::BSTEventSource<RE::BSMusicEvent>* a_eventSource) {
    if (a_event->musicType) {
        if (isCombatTrack(a_event) && a_event->msgType == RE::BSMusicEvent::MUSIC_MESSAGE_TYPE::kAdd) {
            if (shouldPlayCombatMusic()) {
                return RE::BSEventNotifyControl::kContinue;
            } else {
                return RE::BSEventNotifyControl::kStop;
            }
        } else if (isExplorationTrack(a_event) && a_event->msgType == RE::BSMusicEvent::MUSIC_MESSAGE_TYPE::kAdd) {
            if (shouldPlayExplorationMusic()) {
                return RE::BSEventNotifyControl::kContinue;
            } else {
                return RE::BSEventNotifyControl::kStop;
            }
        }
    }
    
    FnProcessEvent fn = fnHash.at(*(uintptr_t*)this);
    return (this->*fn)(a_event, a_eventSource);
}

void MusicHandler::Hook() {
    REL::Relocation<uintptr_t> vtable{ RE::VTABLE_BSMusicManager[0] };
    FnProcessEvent fn = SKSE::stl::unrestricted_cast<FnProcessEvent>(vtable.write_vfunc(1, &MusicHandler::ProcessEvent));
    fnHash.insert(std::pair<uintptr_t, FnProcessEvent>(vtable.address(), fn));
}

void MusicHandler::initialise() {
    constexpr auto path = L"Data/SKSE/Plugins/MusicHandler.ini";

    CSimpleIniA ini;
    ini.SetUnicode();
    ini.LoadFile(path);
    MusicHandler::ExplorationSettings::hoursBetweenTracks = ini.GetLongValue("Exploration", "kHoursBetweenTracks", 3);
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