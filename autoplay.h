// 取自https://github.com/Rottenham/pvz-autoplay
#pragma once
#ifndef __AUTOPLAY_H__
#define __AUTOPLAY_H__

#include "libavz.h"
#include <iomanip>

namespace AutoplayInternal {

std::random_device rd;
std::mt19937 gen(rd());

inline uint32_t GetRandomSeed()
{
    std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
    return dist(gen);
}

}; // namespace AutoplayInternal

inline void UpdateZombiesPreview()
{
    AAsm::KillZombiesPreview();
    AGetMainObject()->SelectCardUi_m()->IsCreatZombie() = false;
}

inline void OriginalSpawn()
{
    auto zombie_type_list = AGetMainObject()->ZombieTypeList();
    for (int i = 0; i < 33; i++)
        zombie_type_list[i] = false;

    // update zombies type
    __asm__ __volatile__(
        "movl 0x6a9ec0, %%esi;"
        "movl 0x768(%%esi), %%esi;"
        "movl 0x160(%%esi), %%esi;"
        "calll *%[func];"
        :
        : [func] "r"(0x00425840)
        : "esi", "memory");

    AAsm::PickZombieWaves();
    UpdateZombiesPreview();
}

inline void SetSeedAndUpdate(uint32_t seed)
{
    AGetMainObject()->MRef<uint32_t>(0x561c) = seed;
    OriginalSpawn();
}

inline std::string SetRandomSeed()
{
    auto seed = AutoplayInternal::GetRandomSeed();
    SetSeedAndUpdate(seed);
    std::ostringstream oss;
    oss << "出怪种子: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << seed;
    return oss.str();
}

#endif //!__AUTOPLAY_H__