// https://github.com/qrmd0/AvZLib/tree/main/crescendo/avz-more
#pragma once
#include "libavz.h"

#ifndef __AZM_VERSION__
#define __AZM_VERSION__ 220822
#endif

namespace cresc {

// 自定义僵尸子类
struct ZombieAZM : public AZombie {
    // 攻击判定的横向宽度
    int& AttackWidth()
    {
        return MRef<int>(0xA4);
    }
};

// 自定义AMainObject子类
struct MainObjectAZM : public AMainObject {
    // 冰道坐标数组（最大值为800）
    uint32_t* IceTrailAbscissaList()
    {
        return MVal<uint32_t*>(0x60C);
    }

    // 冰道倒计时数组
    uint32_t* IceTrailCountdownList()
    {
        return MVal<uint32_t*>(0x624);
    }
};

// 获得MainObjectAZM对象
inline MainObjectAZM* MyMainObject()
{
    return (MainObjectAZM*)AGetMainObject();
}

// 获得row行col列无偏移植物的内存坐标
inline std::pair<int, int> GetPlantCoord(int row, int col)
{
    auto scene = AGetMainObject()->Scene();
    if (scene == 0 || scene == 1)
        return {40 + (col - 1) * 80, 80 + (row - 1) * 100};
    if (scene == 2 || scene == 3)
        return {40 + (col - 1) * 80, 80 + (row - 1) * 85};
    return {40 + (col - 1) * 80, 70 + (row - 1) * 85 + (col <= 5 ? (6 - col) * 20 : 0)};
}

// 获得某特定类型植物的防御域
// 使用现行标准，内存坐标 → 90,0)基准的调试模式
// https://wiki.crescb.com/timespace/ts/
inline std::pair<int, int> GetDefenseRange(APlantType type)
{
    switch (type) {
    case ATALL_NUT:
        return {30, 70};
    case APUMPKIN:
        return {20, 80};
    case ACOB_CANNON:
        return {20, 120};
    default:
        return {30, 50};
    }
}

// 获得某特定类型植物对小丑爆炸的防御域
inline std::pair<int, int> GetExplodeDefenseRange(APlantType type)
{
    switch (type) {
    case ATALL_NUT:
        return {10, 90};
    case APUMPKIN:
        return {0, 100};
    case ACOB_CANNON:
        return {0, 100};
    default:
        return {10, 70};
    }
}

// 判断某僵尸攻击域和某植物防御域是否重叠
inline bool JudgeHit(AZombie* zombie, int plant_row, int plant_col, APlantType plant_type = APEASHOOTER)
{
    auto z = (ZombieAZM*)zombie;
    auto plantX = GetPlantCoord(plant_row, plant_col).first;
    auto def = GetDefenseRange(plant_type);
    return zombie->Row() + 1 == plant_row && ((int(z->Abscissa()) + z->AttackAbscissa() <= plantX + def.second) && (plantX + def.first <= int(z->Abscissa()) + z->AttackAbscissa() + z->AttackWidth()));
}

// 判断小丑爆炸范围和某植物防御域是否重叠
inline bool JudgeExplode(AZombie* zombie, int plant_row, int plant_col, APlantType plant_type = APEASHOOTER)
{
    int x = zombie->Abscissa() + 60, y = zombie->Ordinate() + 60; // 小丑爆心偏移
    auto [p_x, p_y] = GetPlantCoord(plant_row, plant_col);
    int y_dis = 0;
    if (y < p_y)
        y_dis = p_y - y;
    else if (y > p_y + 80)
        y_dis = y - (p_y + 80);
    if (y_dis > 90)
        return false;
    int x_dis = std::sqrt(90 * 90 - y_dis * y_dis);
    auto [d_left, d_right] = GetExplodeDefenseRange(plant_type);
    return p_x + d_left - x_dis <= x && x <= p_x + d_right + x_dis;
}

// 某格子的核坑倒计时
inline int DoomCD(int row, int col)
{
    int doom_cd = 0;
    for (auto& place : aAlivePlaceItemFilter) {
        if (place.Row() == row - 1 && place.Col() == col - 1 && place.Type() == APlaceItemType::CRATER) {
            doom_cd = place.Value();
            break;
        }
    }
    return doom_cd;
}

inline int DoomCD(AGrid g) { return DoomCD(g.row, g.col); }

// 某行的冰道倒计时
inline int IceTrailCD(int row)
{
    if (row < 1 || row > 6)
        return -1;

    return MyMainObject()->IceTrailCountdownList()[row - 1];
}

// 某格子的冰道倒计时
inline int IceTrailCD(int row, int col)
{
    if (row < 1 || row > 6)
        return -1;

    if (col < 1 || col > 9)
        return -1;

    int ice_trail_abs = MyMainObject()->IceTrailAbscissaList()[row - 1];
    int no_cover_limit = 80 * col + 28 + (col == 9 ? 3 : 0); // 9列格子被冰道覆盖有特判
    if (ice_trail_abs >= no_cover_limit)
        return 0;
    else
        return IceTrailCD(row);
}

inline int IceTrailCD(AGrid g) { return IceTrailCD(g.row, g.col); }

// 某格子的核坑倒计时和冰道倒计时的较大值
inline int GridCD(int row, int col)
{
    return std::max(DoomCD(row, col), IceTrailCD(row, col));
}

inline int GridCD(AGrid g) { return GridCD(g.row, g.col); }

// 返回某卡能否成功用在某处
// 主要用于防止Card函数一帧内调用多次Card导致不明原因崩溃
inline bool Plantable(int type, int row, int col)
{
    int _type = type % AM_PEASHOOTER;
    return AAsm::GetPlantRejectType(_type, row - 1, col - 1) == AAsm::NIL;
}

// 返回某卡的冷却倒计时
inline int CardCD(const int& plantType)
{
    int index_seed = AGetSeedIndex(plantType);
    if (index_seed == -1)
        return INT32_MAX;

    auto seeds = AGetMainObject()->SeedArray();
    if (seeds[index_seed].IsUsable())
        return 0;

    return seeds[index_seed].InitialCd() - seeds[index_seed].Cd() + 1;
}

// 是否出现以下出怪
inline bool TypeExist(int zombie_type)
{
    auto zombie_type_list = AGetZombieTypeList();
    return zombie_type >= 0 && zombie_type <= 32 && zombie_type_list[zombie_type];
}

// 是否出现以下出怪之一
inline bool TypeOr(const std::vector<int>& zombie_types)
{
    auto zombie_type_list = AGetZombieTypeList();
    for (int z : zombie_types)
        if (z >= 0 && z <= 32 && zombie_type_list[z])
            return true;
    return false;
}

// 是否包含以下所有出怪
inline bool TypeAnd(const std::vector<int>& zombie_types)
{
    auto zombie_type_list = AGetZombieTypeList();
    for (int z : zombie_types)
        if (z >= 0 && z <= 32 && !zombie_type_list[z])
            return false;
    return true;
}

// 某格子植物受锤最短用时（不受锤返回0）
inline int GridHitCD(int row, int col, APlantType plant_type = APEASHOOTER)
{
    int hitCD = 0;
    for (auto& zombie : aAliveZombieFilter) {
        if (ARangeIn(zombie.Type(), {AGARGANTUAR, AGIGA_GARGANTUAR}) && zombie.State() == 70 && JudgeHit(&zombie, row, col, plant_type)) {
            float animation_progress = zombie.AnimationPtr()->CirculationRate();
            if (animation_progress >= 0.648)
                continue;

            int remain_time = int((zombie.SlowCountdown() > 0 ? 414 : 207) * (0.648 - animation_progress)) + 1 + zombie.FreezeCountdown();
            if (remain_time > hitCD)
                hitCD = remain_time;
        }
    }
    return hitCD;
}

}; // namespace cresc