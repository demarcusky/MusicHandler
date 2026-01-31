#include <SimpleIni.h>
#include "musicHandler.h"

RE::BSEventNotifyControl MusicHandler::ProcessEvent(const RE::BSMusicEvent* a_event, RE::BSTEventSource<RE::BSMusicEvent>* a_eventSource) {
    
    
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
    MusicHandler::CombatSettings::healthCheckMult = ini.GetDoubleValue("Combat", "kHealthCheckMult", 1.0);

    ini.SetLongValue("Exploration", "kHoursBetweenTracks", MusicHandler::ExplorationSettings::hoursBetweenTracks);
    ini.SetBoolValue("Combat", "bEnableSmart", MusicHandler::CombatSettings::enableSmart);
    ini.SetDoubleValue("Combat", "kHealthCheckMult", MusicHandler::CombatSettings::healthCheckMult);

    (void)ini.SaveFile(path);

    SKSE::log::info("Data initialised...");

    Hook();
}

void MusicHandler::Hook() {
    REL::Relocation<uintptr_t> vtable{ RE::VTABLE_BSMusicManager[0] };
    FnProcessEvent fn = SKSE::stl::unrestricted_cast<FnProcessEvent>(vtable.write_vfunc(1, &MusicHandler::ProcessEvent));
    fnHash.insert(std::pair<uintptr_t, FnProcessEvent>(vtable.address(), fn));
}