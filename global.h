#pragma once

#if __has_include("GetZombieAbscissas/GetZombieAbscissas_2_0.h")
#include "GetZombieAbscissas/GetZombieAbscissas_2_0.h"
#else
#error ("You must install the Extension "qrmd/GetZombieAbscissas.h" to run this script")
#endif

#include "avz_more.h"
#include "util.h"

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define CheckPlant(grid, type) AGetPlantIndex(grid.row, grid.col, type) >= 0

class Judge {
public:
    // 检查某格子是否有梯子
    static bool IsExistLadder(int row, int col)
    {
        for (auto& placeItem : aAlivePlaceItemFilter) {
            if (placeItem.Type() == APlaceItemType::LADDER && placeItem.Row() == row - 1 && placeItem.Col() == col - 1)
                return true;
        }
        return false;
    }

    // 检查[list]中是否有格子在[delay_time]后可用
    static bool IsGridUsable(std::vector<AGrid> list, int delay_time = 0)
    {
        for (const auto& grid : list) {
            if (IsGridUsable(grid, delay_time))
                return true;
        }
        return false;
    }

    // 检查格子在[delay_time]后是否可用
    static bool IsGridUsable(AGrid grid, int delay_time = 0)
    {
        return IsGridUsable(grid.row, grid.col, delay_time);
    }

    static bool IsGridUsable(int row, int col, int delay_time = 0)
    {
        if (row <= 0 || col <= 0)
            return false;

        return cresc::GridCD(row, col) <= delay_time;
    }

    // 检查[plantType]植物的种子在[delay_time]后是否可用
    static bool IsSeedUsable(APlantType plantType, int delay_time = 0)
    {
        return cresc::CardCD(plantType) <= delay_time;
    }

    // 判断(row, col)是否有一次性植物
    static bool IsExistAsh(int row, int col)
    {
        if (row <= 0 || col <= 0)
            return false;

        static std::unordered_set<APlantType> ashType = {ACHERRY_BOMB, AJALAPENO, ADOOM_SHROOM, ASQUASH, ABLOVER, APOTATO_MINE, AICE_SHROOM};
        for (const auto& type : ashType) {
            if (AGetPlantIndex(row, col, type) >= 0)
                return true;
        }
        return false;
    }

    // 判断(row, col)是否可以直接种植
    static bool CanPlant(APlantType type, int row, int col)
    {
        if (row <= 0 || col <= 0)
            return false;

        return cresc::Plantable(type, row, col) || IsExistAsh(row, col);
    }

    // 判断某格子是否可以直接种植
    static bool CanPlant(APlantType type, AGrid grid)
    {
        return CanPlant(type, grid.row, grid.col);
    }

    // 检查特定僵尸是否存在
    static bool IsZombieExist(std::function<bool(AZombie*)> zombies)
    {
        return !AAliveFilter<AZombie>(zombies).Empty();
    }

    // 检查[row]行是否存在横坐标小于[abscissas]血量大于[hp]的僵尸
    // [hp]不填时默认为0
    static bool IsExist(int row, float abscissas, int hp = 0, std::unordered_set<int> zombieTypes = {AGIGA_GARGANTUAR, AGARGANTUAR, AZOMBONI})
    {
        return IsZombieExist([=](AZombie* z) { return zombieTypes.contains(z->Type()) && z->Abscissa() < abscissas && z->Row() == row - 1 && z->Hp() > hp; });
    }

    // 检查[wave]波是否有[type]僵尸刷出
    static bool IsRefreshZombie(int wave, int type)
    {
        return IsZombieExist([=](AZombie* z) { return z->Type() == type && z->ExistTime() <= ANowTime(wave); });
    }

    // 返回锤击进度
    // 如果 < 0 代表还未锤击
    // 如果 > 0 代表已经锤击
    static float HammerRate(AZombie* zombie)
    {
        auto circulationRate = zombie->AnimationPtr()->CirculationRate();
        return circulationRate - HAMMER_CIRCULATION_RATE;
    }

    // 检查植物在[time]cs后是否要被冰车碾压
    static bool IsWillBeCrushed(AZombie* zombie, int plantRow, int plantCol, int time)
    {
        auto zombie_index = zombie->Index();
        auto abscVec = _qrmd::AGetZombieAbscissas(zombie_index, time);
        if (abscVec.empty()) {
            return true;
        }
        auto zombieX = abscVec.back();
        int zombieRow = zombie->Row();
        int plantX = cresc::GetPlantCoord(plantRow + 1, plantCol + 1).first;
        // 冰车碾压 +10~+143
        std::pair<float, float> Attack = {zombieX + 10.0, zombieX + 143.0};
        // 普通植物受啃碾砸+30~+50
        std::pair<float, float> defense = {plantX + 30.0, plantX + 50.0};
        return zombieRow == plantRow && HasIntersection(Attack, defense);
    }

    // 检查僵尸是否在樱桃爆炸范围内
    // 这里是模糊计算，灰烬的爆炸范围实际是个圆，这里简化成一个3*3的矩形进行计算
    // 更精确的计算方式参考：
    // https://wiki.pvz1.com/doku.php?id=%E6%8A%80%E6%9C%AF:%E7%B2%BE%E7%A1%AE%E6%95%B0%E6%8D%AE
    static bool IsCherryExplode(AZombie* zombie, int cherryRow, int cherryCol)
    {
        int centerX = cresc::GetPlantCoord(cherryRow, cherryCol).first + 40;
        int zombieRow = zombie->Row() + 1;
        return HasIntersection({centerX - 115, centerX + 115}, _GetDefenseRange(zombie)) && std::abs(zombieRow - cherryRow) <= 1;
    }

protected:
    // 获得某僵尸的横向防御域
    static std::pair<float, float> _GetDefenseRange(AZombie* zombie)
    {
        auto zombieX = zombie->Abscissa();
        auto left = zombieX + zombie->BulletAbscissa();
        auto right = left + zombie->HurtWidth();
        return {left, right};
    }

    // 巨人僵尸锤击命中时的动画循环率
    static constexpr float HAMMER_CIRCULATION_RATE = 0.644;
};

// 使用卡片，自动铲除占位植物，补种花盆或睡莲叶
inline void UseCard(APlantType plant_type, int row, int col)
{
    if (!AIsSeedUsable(plant_type))
        return;

    int type = plant_type % AM_PEASHOOTER;
    auto PlantRejectType = AAsm::GetPlantRejectType(type, row - 1, col - 1);
    if (PlantRejectType == AAsm::NEEDS_POT && AIsSeedUsable(AFLOWER_POT))
        ACard(AFLOWER_POT, row, col);
    if (PlantRejectType == AAsm::NOT_ON_WATER && AIsSeedUsable(ALILY_PAD))
        ACard(ALILY_PAD, row, col);
    if (PlantRejectType == AAsm::NOT_HERE && cresc::GridCD(row, col) == 0 && !Judge::IsExistAsh(row, col))
        AShovel(row, col);

    ACoLaunch([=]() -> ACoroutine {
        co_await [=] { return cresc::Plantable(type, row, col); };
        ACard(plant_type, row, col);
    });
}

// 得到某位置[type]植物的血量
// 如果不存在该种植物，返回0
inline int PlantHp(AGrid grid, APlantType type = APUMPKIN)
{
    auto ptr = AGetPlantPtr(grid.row, grid.col, type);
    return ptr == nullptr ? 0 : ptr->Hp();
};

// 按键控制游戏速度和脚本运行
// ------------参数------------
// [speed]设置初始速度，默认为1.0
inline void KeyController(float speed)
{
    static std::vector<float> GAME_SPEED_GEARS = {0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0};
    ASetGameSpeed(speed);
    AConnect('A', [=] {
        int game_speed_gear = GAME_SPEED_GEARS.size();
        float game_speed = 10.0 / AGetPvzBase()->TickMs();
        for (int i = 0; i < GAME_SPEED_GEARS.size(); ++i) {
            if (game_speed <= GAME_SPEED_GEARS[i]) {
                game_speed_gear = i;
                break;
            }
        }
        if (game_speed_gear > 0) {
            ASetGameSpeed(GAME_SPEED_GEARS[game_speed_gear - 1]);
        }
    });
    AConnect('D', [=] {
        int game_speed_gear = 0;
        float game_speed = 10.0 / AGetPvzBase()->TickMs();
        for (int i = GAME_SPEED_GEARS.size() - 1; i >= 0; --i) {
            if (game_speed >= GAME_SPEED_GEARS[i]) {
                game_speed_gear = i;
                break;
            }
        }
        if (game_speed_gear < GAME_SPEED_GEARS.size() - 1) {
            ASetGameSpeed(GAME_SPEED_GEARS[game_speed_gear + 1]);
        }
    });
    AConnect('S', [=] { ASetGameSpeed(1); });
    AConnect('Q', [=] {
        auto hWnd = AGetPvzBase()->Hwnd();
        auto ret = MessageBoxW(hWnd, L"您想要立即停止脚本的运行吗？点击确定脚本将停止运行，点击取消则会在完成本轮选卡后结束脚本的运行", L"Waring", MB_ICONINFORMATION | MB_OKCANCEL | MB_APPLMODAL);
        ASetReloadMode(AReloadMode::NONE);
        if (ret == 1) {
            ret = MessageBoxW(hWnd, L"请再次确定是否要立即停止脚本运行，立即停止脚本运行可能会出现一些异常情况", L"Waring", MB_ICONINFORMATION | MB_OKCANCEL | MB_APPLMODAL);
            if (ret == 1)
                ATerminate("您主动结束了脚本的运行！");
        }
    });
}

#endif //!__GLOBAL_H__