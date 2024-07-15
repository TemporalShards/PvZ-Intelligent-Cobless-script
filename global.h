#pragma once
#include "GetZombieAbscissas/GetZombieAbscissas_2_0.h"
#include "avz_more_autoplay.h"
#include "util.h"

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

class Judge {
public:
    // 检查[list]中是否有格子在[delay_time]后可用
    static bool IsGridUsable(std::vector<AGrid> list, int delay_time = 0)
    {
        for (const auto& grid : list) {
            if (IsGridUsable(grid, delay_time)) {
                return true;
            }
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
        if (row <= 0 || col <= 0) {
            return false;
        }
        return cresc::GridCD(row, col) <= delay_time;
    }

    // 检查[plantType]植物的种子在[delay_time]后是否可用
    static bool IsSeedUsable(APlantType plantType, int delay_time = 0)
    {
        return cresc::CardCD(plantType) <= delay_time;
    }

    // 判断(row, col)是否有一次性植物
    static bool IsExistAsh(int row, int col, std::unordered_set<APlantType> ashType = {ACHERRY_BOMB, AJALAPENO, ADOOM_SHROOM, ASQUASH, ABLOVER, APOTATO_MINE, AICE_SHROOM})
    {
        if (row <= 0 || col <= 0) {
            return false;
        }
        for (const auto& type : ashType) {
            if (AGetPlantIndex(row, col, type) >= 0)
                return true;
        }
        return false;
    }

    // 判断(row, col)是否可以直接种植
    static bool CanPlant(APlantType type, int row, int col)
    {
        if (row <= 0 || col <= 0) {
            return false;
        }
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
        auto circulationRate = cresc::GetAnimation(zombie)->CirculationRate();
        return circulationRate - HAMMER_CIRCULATION_RATE;
    }

    // 检查植物在[time]cs后是否要被冰车碾压
    static bool IsWillBeCrushed(AZombie* zombie, int plantRow, int plantCol, int time)
    {
        auto zombie_index = zombie->MRef<uint16_t>(0x158);
        auto abscVec = _qrmd::AGetZombieAbscissas(zombie_index, time);
        if (abscVec.empty()) {
            return true;
        }
        auto zombieX = abscVec[time];
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

    // 检查[grid]格子是否有[plant_type]植物
    static bool CheckPlant(AGrid grid, APlantType plant_type)
    {
        return AGetPlantIndex(grid.row, grid.col, plant_type) >= 0;
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

    int type = plant_type > AIMITATOR ? plant_type - AM_PEASHOOTER : plant_type;
    auto PlantRejectType = AAsm::GetPlantRejectType(type, row - 1, col - 1);
    if (PlantRejectType == AAsm::NEEDS_POT && AIsSeedUsable(AFLOWER_POT))
        ACard(AFLOWER_POT, row, col);
    if (PlantRejectType == AAsm::NOT_ON_WATER && AIsSeedUsable(ALILY_PAD))
        ACard(ALILY_PAD, row, col);
    if (PlantRejectType == AAsm::NOT_HERE && cresc::GridCD(row, col) == 0 && !Judge::IsExistAsh(row, col))
        AShovel(row, col);

    ACoLaunch([=]() -> ACoroutine {
        co_await [=] { return cresc::Plantable(static_cast<APlantType>(type), row, col); };
        ACard(plant_type, row, col);
    });
}

// 清除浓雾
inline void ClearFog(bool isEnable)
{
    AMRef<uint16_t>(0x41a68d) = isEnable ? 0xd231 : 0xf23b;
}

// 游戏控制
class GameController {
protected:
    static constexpr std::array<float, 7> GAME_SPEED_GEARS = {0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0};
    float _default_speed = 1.0;
    AKey _decelerate_key = 'A';
    AKey _accelerate_key = 'D';
    AKey _reset_speed_key = 'S';
    AKey _script_stop_key = 'Q';

public:
    // 启用按键速度控制，默认速度倍率[speed]=1.0
    // [speed]有7个档位：{0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0}
    // A：速度加快一档
    // D：速度减慢一档
    // W：速度恢复原速
    // Q：脚本停止运行
    void Enable(const float& speed = 1.0);

    // 设置减速快捷键
    void SetDecelerateKey(const AKey& key) { this->_decelerate_key = key; }

    // 设置加速快捷键
    void SetAccelerateKey(const AKey& key) { this->_accelerate_key = key; }

    // 设置恢复默认速度快捷键
    void SetResetSpeedKey(const AKey& key) { this->_reset_speed_key = key; }

    // 设置脚本停止运行快捷键
    void SetScriptStopKey(const AKey& key) { this->_script_stop_key = key; }
} inline game_controller;

inline void GameController::Enable(const float& speed)
{
    ASetGameSpeed(speed);
    _default_speed = 10.0 / AGetPvzBase()->TickMs();
    AConnect(_decelerate_key, [=] {
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
    AConnect(_accelerate_key, [=] {
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
    AConnect(_reset_speed_key, [=] { ASetGameSpeed(1); });

    AConnect(_script_stop_key, [=] {
        auto pvzHwnd = AGetPvzBase()->MRef<HWND>(0x350);
        auto ret = MessageBoxW(pvzHwnd, L"您想要立即停止脚本的运行吗？点击确定脚本将停止运行，点击取消则会在完成本轮选卡后结束脚本的运行", L"Waring", MB_ICONINFORMATION | MB_OKCANCEL | MB_APPLMODAL);
        ASetReloadMode(AReloadMode::NONE);
        if (ret == 1) {
            ATerminate("您主动结束了脚本的运行！");
        }
    });
}

// 取自：https://github.com/qrmd0/AvZLib/tree/main/qrmd/qmLib#avz-qmlib
// 指定键均被按下时，返回true。
// ------------参数------------
// keys 要检测的按键的虚拟键码，详见：https://learn.microsoft.com/zh-cn/windows/win32/inputdev/virtual-key-codes
// isRequirePvZActive 是否要求PvZ窗口是当前窗口
// ------------示例------------
// AConnect([] { return true; }, [] {
//     if(AGetIsKeysDown({VK_CONTROL, VK_SHIFT, 'D'}, false)){
//         ALogger<AMsgBox> _log;
//         _log.Debug("您按下了全局快捷键 Ctrl+Shift+D");
//     } });
inline bool AGetIsKeysDown(const std::vector<int>& keys, bool isRequirePvZActive = true)
{
    auto pvzHandle = AGetPvzBase()->MRef<HWND>(0x350);
    if (isRequirePvZActive && GetForegroundWindow() != pvzHandle)
        return false;

    for (const auto& each : keys) {
        if ((GetAsyncKeyState(each) & 0x8000) == 0)
            return false;
    }
    return true;
}
inline bool AGetIsKeysDown(const int& key, bool isRequirePvZActive = true)
{
    auto pvzHandle = AGetPvzBase()->MRef<HWND>(0x350);
    if (isRequirePvZActive && GetForegroundWindow() != pvzHandle)
        return false;

    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

#endif //!__GLOBAL_H__