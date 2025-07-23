#pragma once
#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include "global.h"

// 对场地进行划分
enum class PlaceType {
    ALL,  // 全场
    UP,   // 上半场
    DOWN, // 下半场
};

// 关卡类型
enum class Level {
    Slow,     // 慢速
    Variable, // 变速
    White,    // 纯白
    Fast,     // 快速
    TopSpeed, // 极速
};

std::array<float, 6> minGigaX;

// 放置原版核的位置
AGrid NuclearGrid;
// 放置复制核的位置
AGrid copyNuclearGrid;
// 放置辣椒的位置
AGrid jalapenoGrid;
// 放置樱桃的位置
AGrid cherryGrid;

// 冰车碾压各列普通植物的最早时间
static const std::unordered_map<int, int> CrashTime = {{5, 2227}, {6, 1555}, {7, 1054}, {8, 653}, {9, 318}};

// 各种情况的应对处理
class Processor : public AStateHook {
public:
    // 得到关卡类型
    Level GetLevelType()
    {
        if (cresc::TypeExist(AGIGA_GARGANTUAR)) {
            // 有红眼，同时{铁桶，橄榄，白眼}有其一，刷新基本稳定
            if (cresc::TypeExist(AGARGANTUAR)) {
                _levelType = Level::Slow;
            } else { // 有红眼，但是{铁桶，橄榄，白眼}都没有，变速
                if (cresc::TypeExist(AZOMBONI) && cresc::TypeOr({AFOOTBALL_ZOMBIE, ABUCKETHEAD_ZOMBIE}))
                    _levelType = Level::Slow;
                else
                    _levelType = Level::Variable;
            }
        } else {
            if (cresc::TypeExist(AGARGANTUAR)) {
                // // 有白眼，同时{铁桶，橄榄，冰车}有其一
                if (cresc::TypeOr({ABUCKETHEAD_ZOMBIE, AFOOTBALL_ZOMBIE, AZOMBONI}))
                    _levelType = Level::Slow;
                else // 只有白眼
                    _levelType = Level::White;
            } else {
                if (cresc::TypeOr({ABUCKETHEAD_ZOMBIE, AFOOTBALL_ZOMBIE})) {
                    if (cresc::TypeExist(AZOMBONI)) {
                        _levelType = Level::Fast;
                    } else {
                        // 边路偷两个南瓜保大喷，减少补喷
                        temporaryPumpkin.push_back({1, 5});
                        temporaryPumpkin.push_back({6, 5});
                        _levelType = Level::White;
                    }
                } else {
                    _levelType = Level::TopSpeed;
                }
            }
        }

        return this->_levelType;
    }

    // 返回樱桃的作用半场
    PlaceType CherryPos() { return this->_cherryPos; }

    // 返回窝瓜的放置行
    int SquashRow() { return this->_squashGrid.row; };

    // 在指定半场生成多个能够种植植物的格子
    // [OnlyCheckGrid]为true时，只检查格子可用性，否则优先选择能直接种植的格子
    static std::vector<AGrid> GetPlantableGrids(PlaceType type, APlantType plantType, bool OnlyCheckGrid = false)
    {
        std::set<int> rows;
        std::vector<AGrid> result;
        switch (type) {
        case PlaceType::ALL: // 实际上应该包含水路的格子，但是这个阵只有核弹放在水路，就不用考虑了
            rows = {1, 2, 5, 6};
            break;
        case PlaceType::UP:
            rows = {1, 2};
            break;
        case PlaceType::DOWN:
            rows = {5, 6};
            break;
        }
        for (int col = 9; col >= 4; --col) {
            for (const auto& r : rows)
                if (Judge::CanPlant(plantType, r, col) || (Judge::IsGridUsable(r, col) && OnlyCheckGrid))
                    result.push_back({r, col});
        }
        if (result.empty()) {
            for (int col = 9; col >= 4; --col) {
                for (const auto& r : rows)
                    if (Judge::IsGridUsable(r, col))
                        result.push_back({r, col});
            }
        }
        return result;
    }

    // 返回[rows]中所有红眼的血量之和最大的行
    // 如果没有红眼，返回-1
    int GetHpMaxRow(std::vector<int> rows)
    {
        std::ranges::sort(rows, [this](const auto& lhs, const auto& rhs) { return __GetGigaHp(lhs) > __GetGigaHp(rhs); });
        int row = rows.front();
        if (__GetGigaHp(row) > 0) {
            return row;
        } else {
            return -1;
        }
    }

    // 使用樱桃
    void UseCherry(PlaceType place_type = PlaceType::ALL)
    {
        ACoLaunch([=, this] -> ACoroutine {
            int now_wave = ANowWave();
            co_await [=, this] {
                _UpdateCherryGrid(place_type);
                int time = CrashTime.contains(cherryGrid.col + 1) ? CrashTime.at(cherryGrid.col + 1) : 318;
                return AIsSeedUsable(ACHERRY_BOMB) && ANowTime(now_wave) >= time && Judge::IsGridUsable(cherryGrid);
            };
            _SafeCard(ACHERRY_BOMB, cherryGrid);
            AGetInternalLogger()->Debug("A {}", cherryGrid);
        });
    }

    // 使用黑核
    void UseNuclear()
    {
        ACoLaunch([this]() -> ACoroutine {
            co_await [this] {
                _GetNuclearGrid();
                return AIsSeedUsable(ALILY_PAD) && AIsSeedUsable(ADOOM_SHROOM) && Judge::IsGridUsable(NuclearGrid);
            };
            _SafeCard(ADOOM_SHROOM, NuclearGrid);
            co_await [=] { return CheckPlant(NuclearGrid, ADOOM_SHROOM); };
            ASetPlantActiveTime(ADOOM_SHROOM, 100);
            AGetInternalLogger()->Debug("N {}", NuclearGrid);
        });
    }

    // 使用辣椒
    // [row]为-1时默认在与樱桃异侧的半场放辣椒
    void UseJalapeno(int row = -1)
    {
        ACoLaunch([=, this] -> ACoroutine {
            co_await [=, this] {
                _GetJalapenoGrid(row);
                return AIsSeedUsable(AJALAPENO) && Judge::IsGridUsable(jalapenoGrid);
            };
            _SafeCard(AJALAPENO, jalapenoGrid);
            AGetInternalLogger()->Debug("A' {}", jalapenoGrid);
        });
    }

    // 使用白核
    void UseCopyNuclear()
    {
        ACoLaunch([this]() -> ACoroutine {
            co_await [this] {
                _GetCopyNuclearGrid();
                return AIsSeedUsable(AM_DOOM_SHROOM) && AIsSeedUsable(ALILY_PAD) && Judge::IsGridUsable(copyNuclearGrid);
            };
            _SafeCard(AM_DOOM_SHROOM, copyNuclearGrid, 110);
            AGetInternalLogger()->Debug("N' {}", copyNuclearGrid);
        });
    }

    // 使用冰
    void UseIce()
    {
        ACoLaunch([this]() -> ACoroutine {
            co_await [this] {
                _GetIceGrid();
                return AIsSeedUsable(AICE_SHROOM);
            };
            _SafeCard(AICE_SHROOM, _iceGrid);
            co_await [this] { return CheckPlant(_iceGrid, AICE_SHROOM); };
            AGetInternalLogger()->Debug("I {}", _iceGrid);
            ASetPlantActiveTime(AICE_SHROOM, 100);
        });
    }

    // 使用复制冰
    void UseCopyIce()
    {
        ACoLaunch([this]() -> ACoroutine {
            co_await [this] {
                _GetCopyIceGrid();
                return AIsSeedUsable(AM_ICE_SHROOM) && Judge::IsGridUsable(_copy_iceGrid);
            };
            _SafeCard(AM_ICE_SHROOM, _copy_iceGrid);
        });
    }

    // 使用窝瓜
    // [row]为-1时默认在与辣椒异侧的半场放窝瓜
    // [auto_set_time]为true时自动将释放窝瓜的时间修正至能压到冰车的时机
    void UseSquash(int row = -1, bool auto_set_time = true)
    {
        ACoLaunch([=, this] -> ACoroutine {
            int now_wave = ANowWave();
            co_await [=, this] {
                _GetSquashGrid(row);
                int time = CrashTime.contains(_squashGrid.col - 1) ? CrashTime.at(_squashGrid.col - 1) - 182 : CrashTime.at(5);
                return AIsSeedUsable(ASQUASH) && (ANowTime(now_wave) >= time || !auto_set_time);
            };
            _SafeCard(ASQUASH, _squashGrid, 110);
            AGetInternalLogger()->Debug("a {}", _squashGrid);
        });
    }

    // 使用地刺
    void UseSpikeweed(int row)
    {
        ACoLaunch([=] -> ACoroutine {
            int wave = ANowWave();
            co_await [=] {
                if (AIsZombieExist(AZOMBONI, row)) {
                    int spikeweedCol;
                    for (int col : {9, 8, 7, 6, 5, 4}) {
                        if (Judge::IsGridUsable(row, col)) {
                            spikeweedCol = col;
                            break;
                        }
                    }
                    int time = CrashTime.contains(spikeweedCol) ? CrashTime.at(spikeweedCol) : CrashTime.at(9);
                    if (ANowTime(wave) >= time && AIsSeedUsable(ASPIKEWEED)) {
                        UseCard(ASPIKEWEED, row, spikeweedCol);
                        return !AIsSeedUsable(ASPIKEWEED);
                    }
                }
                return !AIsZombieExist(AZOMBONI, row);
            };
        });
    }

protected:
    // avz_more.h里的SafeCard不能自动补种花盆、睡莲叶，重新实现
    // 等待上下三行内小丑炸完后用卡，自动铲除占位植物，补种花盆或睡莲叶
    void _SafeCard(APlantType plant_type, int row, int col, int need_time = 100)
    {
        if (!AIsSeedUsable(plant_type))
            return;

        if (need_time > 110 || need_time < 0) {
            need_time = std::clamp(need_time, 0, 110);
        }

        ACoLaunch([=, this] -> ACoroutine {
            co_await [=, this] { return _IsAshSafe(row, col, need_time); };
            UseCard(plant_type, row, col);
        });
    }

    void _SafeCard(APlantType plant_type, AGrid grid, int need_time = 100)
    {
        this->_SafeCard(plant_type, grid.row, grid.col, need_time);
    }

    // 使用垫材
    void _Fodder()
    {
        if (!AIsSeedUsable(APUFF_SHROOM))
            return;

        for (const auto& [row, col] : _dangerGridVec) {
            if (!cresc::Plantable(APUFF_SHROOM, row, col))
                continue;

            ACard(APUFF_SHROOM, row, col);
            AGetInternalLogger()->Debug("Use PUFF to c({}, {})", row, col);
            return;
        }
        static std::vector<AGrid> gridList = {{1, 6}, {6, 6}, {2, 6}, {5, 6}, {1, 7}, {6, 7}};
        for (const auto& [row, col] : gridList) {
            if (!cresc::Plantable(APUFF_SHROOM, row, col))
                continue;

            if (!_IsFodderSafe(row, col))
                continue;

            ACard(APUFF_SHROOM, row, col);
            break;
        }
    }

    // 当冰车或巨人即将碾压/砸扁南瓜时将南瓜铲除
    void _RemovePumpkin()
    {
        std::vector<AGrid> PumpkinGridVec = {};
        for (auto& plant : aAlivePlantFilter) {
            if (plant.Type() == APUMPKIN) {
                PumpkinGridVec.push_back({plant.Row() + 1, plant.Col() + 1});
            }
        }
        for (const auto& [row, col] : PumpkinGridVec) {
            auto pumpkinIdx = AGetPlantIndex(row, col, APUMPKIN);
            if (pumpkinIdx < 0)
                continue;

            for (const auto& zomboniPtr : _zomboniVec) {
                auto zombie_abscissa = zomboniPtr->Abscissa();
                auto zombie_row = zomboniPtr->Row() + 1;
                if (zombie_abscissa < 80 * col + 31.5 && row == zombie_row) {
                    AShovel(zombie_row, col, APUMPKIN);
                    break;
                }
            }
            for (const auto& gigaPtr : _gigaVec) {
                if (!cresc::JudgeHit(gigaPtr, row, col, APUMPKIN))
                    continue;

                if (!gigaPtr->IsHammering())
                    continue;

                if (Judge::HammerRate(gigaPtr) < -0.01)
                    continue;

                // int fooderIndex = AGetPlantIndex(row, col + 1);
                // if (fooderIndex > pumpkinIdx)
                //     continue;

                AShovel(row, col, APUMPKIN);
                break;
            }
        }
    }

    // 修补南瓜头
    // 返回的是场上需要修补的南瓜头数量
    int _FixPumpkin(std::vector<AGrid> lst, int Fixhp, int startFixSun)
    {
        if (!AIsSeedUsable(APUMPKIN))
            return lst.size();

        if (AGetMainObject()->Sun() < startFixSun)
            return lst.size();

        std::vector<AGrid> needFixList = {};
        std::copy_if(lst.begin(), lst.end(), std::back_inserter(needFixList), [=](const auto& grid) {
            return PlantHp(grid) < Fixhp;
        });

        if (needFixList.empty())
            return 0;

        std::ranges::sort(needFixList, [=](const auto& lhs, const auto& rhs) { return PlantHp(lhs) < PlantHp(rhs); });

        for (const auto& [row, col] : needFixList) {
            if (Judge::IsExist(row, (col + 1.5) * 80) || !_IsFodderSafe(row, col, 200))
                continue;

            if (cresc::Plantable(APUMPKIN, row, col)) {
                ACard(APUMPKIN, row, col);
                return needFixList.size() - 1;
            }
        }
        return needFixList.size();
    }

    // 修补大喷菇
    void _FixFumeShroom()
    {
        if (!AIsSeedUsable(AFUME_SHROOM))
            return;

        for (const auto& [row, col] : _FumeShroomList) {
            if (Judge::IsExist(row, (col + 1.5) * 80, 0, {AGIGA_GARGANTUAR, AGARGANTUAR, AFOOTBALL_ZOMBIE})
                || !_IsFodderSafe(row, col, 100))
                continue;

            if (AGetPlantIndex(row, col, AFUME_SHROOM) >= 0)
                continue;

            if (Judge::IsGridUsable(row, col)) {
                UseCard(AFUME_SHROOM, row, col);
                return;
            }
        }

        // 如果当前边路压力太大，大喷临时当作垫材使用
        static std::vector<int> rows = {1, 2, 5, 6};
        std::ranges::sort(rows, [=](const auto& lhs, const auto& rhs) {
            return minGigaX[lhs - 1] < minGigaX[rhs - 1];
        });
        for (const auto& row : rows) {
            for (const auto& col : {4, 5}) {
                if (minGigaX[row - 1] < -50 && cresc::Plantable(AFUME_SHROOM, row, col)) {
                    if (!_IsFodderSafe(row, col))
                        continue;

                    ACard(AFUME_SHROOM, row, col);
                    AGetInternalLogger()->Warning("Use FUME to c({}, {})", row, col);
                    return;
                }
            }
        }

        // 没有冰车的关在边路偷喷，提高一下IO
        if (!cresc::TypeExist(AZOMBONI)
            && (AGetMainObject()->Sun() >= 4000 || _levelType == Level::Slow || _levelType == Level::Variable)) {
            static std::vector<AGrid> tempList = {{1, 6}, {6, 6}};
            for (const auto& [row, col] : tempList) {
                if (Judge::IsExist(row, (col + 2) * 80, 0, {AFOOTBALL_ZOMBIE, AGIGA_GARGANTUAR, AGARGANTUAR})
                    || !_IsFodderSafe(row, col, 100))
                    continue;

                if (AGetPlantIndex(row, col, AFUME_SHROOM) >= 0)
                    continue;

                // // 存在小喷当垫材的时候不偷喷
                // if (AGetPlantIndex(row, col, APUFF_SHROOM) >= 0 && minGigaX[row - 1] < 120)
                //     continue;

                if (Judge::IsGridUsable(row, col)) {
                    UseCard(AFUME_SHROOM, row, col);
                    return;
                }
            }
        }
    }

    // 自动使用三叶草
    void _AutoBlover()
    {
        if (!AIsSeedUsable(ABLOVER))
            return;

        // auto safe = [this](int row, int col) -> bool {
        //     // 检查巨人锤击
        //     auto hitCD = _GridHitCD(row, col, ABLOVER);
        //     return _IsAshSafe(row, col, 50) && (hitCD > 50 || hitCD == 0);
        // };

        if (_minBalloonX < 40) {
            auto grids = GetPlantableGrids(PlaceType::ALL, ABLOVER);
            for (const auto& [row, col] : grids) {
                if (!_IsAshSafe(row, col, 50))
                    continue;

                auto hitCD = _GridHitCD(row, col, ABLOVER);
                if ((hitCD > 50 || hitCD == 0) && cresc::Plantable(ABLOVER, row, col)) {
                    ACard(ABLOVER, row, col);
                    return;
                }
            }
        }

        // 如果当前边路压力太大，且不需要使用三叶草吹气球，三叶草临时当作垫材使用
        if (_minBalloonX < 280)
            return;

        static std::vector<int> rows = {1, 2, 5, 6};
        std::ranges::sort(rows, [=](const auto& lhs, const auto& rhs) {
            return minGigaX[lhs - 1] < minGigaX[rhs - 1];
        });
        for (const auto& row : rows) {
            for (const auto& col : {4, 5}) {
                if (minGigaX[row - 1] < -90 && cresc::Plantable(ABLOVER, row, col)) {
                    if (!_IsFodderSafe(row, col))
                        continue;

                    ACard(ABLOVER, row, col);
                    AGetInternalLogger()->Warning("Use Blover to c({}, {})", row, col);
                    return;
                }
            }
        }
    }

private:
    // 原版核的使用位置
    const std::vector<AGrid> _CraterList = {{3, 9}, {4, 9}, {3, 8}};
    // 复制核的使用位置
    const std::vector<AGrid> _copyCraterList = {{4, 7}, {3, 7}, {4, 8}};
    // 大喷菇的位置
    const std::vector<AGrid> _FumeShroomList = {{1, 4}, {2, 4}, {5, 4}, {6, 4}, {1, 5}, {2, 5}, {5, 5}, {6, 5}};
    // 最大修补血量的南瓜头位置，修补阈值2400
    const std::vector<AGrid> MaxFixHpPumpkin = {{3, 5}, {4, 5}, {3, 6}, {4, 6}, {1, 3}, {6, 3}, {1, 2}, {6, 2}, {1, 1}, {2, 1}, {5, 1}, {6, 1}};

    // 临时南瓜头的位置，修补阈值600
    std::vector<AGrid> temporaryPumpkin;

    // 边路以5列大喷为基准，岸路以4列南瓜为基准
    static constexpr std::array<float, 6> leftX = {470.f, 430.f, 800.f, 800.f, 430.f, 470.f};

    std::vector<AGrid> _dangerGridVec;
    std::vector<AZombie*> _gigaVec;
    std::vector<AZombie*> _boxZombieVec;
    std::vector<AZombie*> _zomboniVec;

    // 放置冰的位置
    AGrid _iceGrid;
    // 放置复制冰的位置
    AGrid _copy_iceGrid;
    // 放置窝瓜的位置
    AGrid _squashGrid;

    // 樱桃炸弹的作用半场
    PlaceType _cherryPos;

    // 气球僵尸的最小横坐标
    float _minBalloonX;

    ATickRunner __tickRunner;

    Level _levelType;

    virtual void _EnterFight() override
    {
        temporaryPumpkin = __AMoveToTop(temporaryPumpkin, {{1, 4}, {2, 4}, {5, 4}, {6, 4}});

        RemoveDuplicates(temporaryPumpkin);

        _minBalloonX = 800;

        _gigaVec.clear();
        _boxZombieVec.clear();
        _dangerGridVec.clear();
        _zomboniVec.clear();

        __tickRunner.Start([this] {
            _Observe();
            _GetDangerGridVec();
            _RemovePumpkin();
            _AutoBlover();
            _FixFumeShroom();
            _Fodder();

            // 修补血量2000是凭感觉设置的
            int needFix = _FixPumpkin(MaxFixHpPumpkin, 2000, 0);
            if (needFix <= 0) { // 最重要的南瓜修补完成后，才修补临时南瓜
                _FixPumpkin(temporaryPumpkin, 600, 5000);
            }
        },
            ATickRunner::ONLY_FIGHT, -1);
    }

    virtual void _ExitFight() override
    {
        temporaryPumpkin.clear();
    }

    // 计数器
    // 根据僵尸对阵的威胁程度添加计数偏移（凭感觉加的）
    int __Counter(const AGrid& grid)
    {
        int result = 0;
        for (auto& zombie : aAliveZombieFilter) {
            if (Judge::IsCherryExplode(&zombie, grid.row, grid.col))
                switch (zombie.Type()) {
                case AGIGA_GARGANTUAR:
                    result += 40;
                    break;
                case AGARGANTUAR:
                    result += 20;
                    break;
                case AFOOTBALL_ZOMBIE:
                case AZOMBONI:
                    result += 10;
                    break;
                default:
                    result += 1;
                    break;
                };
        }
        return result;
    }

    // 得到樱桃的位置
    void _UpdateCherryGrid(PlaceType place_type)
    {
        std::vector<AGrid> gridList = GetPlantableGrids(place_type, ACHERRY_BOMB, true);
        // 根据樱桃在不同位置能炸到僵尸的数量将格子列表进行排序
        std::ranges::sort(gridList, [this](const auto& lhs, const auto& rhs) { return __Counter(lhs) > __Counter(rhs); });
        // 选择收益最大的格子作为樱桃释放位置
        int cherryCd = cresc::CardCD(ACHERRY_BOMB);
        for (int i = 0; i < gridList.size(); ++i) {
            // 4列樱桃会炸梯子
            if (Judge::IsGridUsable(gridList[i], cherryCd) && gridList[i].col != 4) {
                cherryGrid = gridList[i];
                break;
            }
        }
        _cherryPos = cherryGrid.row <= 2 ? PlaceType::UP : PlaceType::DOWN;
    }

    // 得到种原版核的位置
    void _GetNuclearGrid()
    {
        std::vector<AGrid> canPlantList = {};
        for (const auto& [row, col] : _CraterList) {
            if (cresc::Plantable(ALILY_PAD, row, col)) {
                canPlantList.push_back({row, col});
            }
        }
        if (canPlantList.size() == 1) { // 只有一个位置
            NuclearGrid = canPlantList.front();
        } else if (!canPlantList.empty()) { // 多个位置，应该根据场上的情况决定
            // 难写，摆烂了
            // 下面是瞎写的，判断的逻辑主要针对某次挂机中在某一轮多次SL的出怪情况
            // 3-8的核弹范围较大，在边路巨人压力大的时候应该优先考虑
            AGrid temp = {3, 8};
            auto iter = std::ranges::find(canPlantList, temp);
            if (iter != canPlantList.end()) {
                if ((minGigaX[0] < 80 && minGigaX[5] < 80) || minGigaX[0] < 0 || minGigaX[5] < 0) {
                    NuclearGrid = temp;
                } else {
                    canPlantList.erase(iter);
                    NuclearGrid = aRandom.Choice(canPlantList);
                }
            } else {
                NuclearGrid = aRandom.Choice(canPlantList);
            }
        }
    }

    // 得到复制核的位置
    void _GetCopyNuclearGrid()
    {
        // 实际上复制核的位置也应该根据场上的情况决定，这里摆烂了
        // 或者将原版核和复制核的位置列表合并，根据场上的情况决定选取哪一个
        for (const auto& [row, col] : _copyCraterList) {
            if (cresc::Plantable(ALILY_PAD, row, col)) {
                copyNuclearGrid = {row, col};
                break;
            }
        }
    }

    // 得到种冰的位置
    void _GetIceGrid()
    {
        auto _grid = GetPlantableGrids(PlaceType::ALL, AICE_SHROOM);
        for (const auto& [row, col] : _grid) {
            if (_IsFodderSafe(row, col, 100)) {
                _iceGrid = {row, col};
                break;
            }
        }
    }

    // 得到种复制冰的位置
    void _GetCopyIceGrid()
    {
        auto gridList = GetPlantableGrids(PlaceType::ALL, AICE_SHROOM);
        std::vector<AGrid> targetList;
        for (const auto& [row, col] : gridList) {
            if (Judge::IsExist(row, (col + 1.5) * 80, 89, {ALADDER_ZOMBIE, AFOOTBALL_ZOMBIE, ABUCKETHEAD_ZOMBIE, ABACKUP_DANCER, ACONEHEAD_ZOMBIE, AZOMBIE, AGARGANTUAR, AGIGA_GARGANTUAR}))
                continue;

            if (_IsFodderSafe(row, col, 320)) {
                targetList.push_back({row, col});
            }
        }
        if (!targetList.empty()) {
            std::ranges::sort(targetList, [=](const auto& lhs, const auto& rhs) {
                return lhs.col < rhs.col; // 尽量选择靠后的格子
            });
            _copy_iceGrid = targetList.front();
        }
    }

    // 生成辣椒的位置
    void _GetJalapenoGrid(int row)
    {
        if (row == -1) {
            jalapenoGrid.row = _cherryPos == PlaceType::UP ? 6 : 1;
        } else {
            jalapenoGrid.row = row;
        }
        jalapenoGrid.col = -1;
        int jala_cd = cresc::CardCD(AJALAPENO);
        // 优先在有巨人的位置放辣椒，可以充当一个垫材
        for (const auto& [row, col] : _dangerGridVec) {
            if (row == jalapenoGrid.row && Judge::IsGridUsable(row, col, jala_cd)) {
                jalapenoGrid.col = col;
                return;
            }
        }
        if (jalapenoGrid.col == -1) {
            // 其次再释放的那一路中的可用格子
            for (int col = 9; col >= 4; --col) {
                if (Judge::IsGridUsable(jalapenoGrid.row, col, jala_cd)) {
                    jalapenoGrid.col = col;
                    return;
                }
            }
        }
    }

    // 生成窝瓜的格子
    void _GetSquashGrid(int row = -1)
    {
        if (row == -1)
            _squashGrid.row = jalapenoGrid.row == 6 ? 1 : 6;
        else
            _squashGrid.row = row;

        int squash_cd = cresc::CardCD(ASQUASH);
        for (int col = 9; col >= 4; --col) {
            if (Judge::IsGridUsable(_squashGrid.row, col, squash_cd)) {
                _squashGrid.col = col;
                return;
            }
        }
    }

    // 检查种在(row, col)的灰烬是否安全
    bool _IsAshSafe(int row, int col, int need_time = 100)
    {
        // 检查是否有丑爆
        for (const auto& boxPtr : _boxZombieVec) {
            if (cresc::JudgeExplode(boxPtr, row, col) && boxPtr->State() == 16 && boxPtr->StateCountdown() <= need_time)
                return false;
        }
        return true;
    }

    // 检查(row, col)是否能种植垫材
    bool _IsFodderSafe(int row, int col, int ExistTime = 50)
    {
        // 检查冰车碾压
        if (!_zomboniVec.empty()) {
            for (const auto& zomboniPtr : _zomboniVec)
                if (Judge::IsWillBeCrushed(zomboniPtr, row - 1, col - 1, ExistTime))
                    return false;
        }
        // 检查巨人锤击
        auto hitCD = _GridHitCD(row, col);
        return _IsAshSafe(row, col, ExistTime) && (hitCD > ExistTime || hitCD == 0);
    }

    // 观测场上的僵尸情况
    void _Observe()
    {
        static int lock = -1;
        // 一帧只观测一次
        if (lock == AGetMainObject()->GameClock())
            return;

        lock = AGetMainObject()->GameClock();
        _gigaVec.clear();
        _boxZombieVec.clear();
        _zomboniVec.clear();
        _minBalloonX = 800;
        for (auto& zombie : aAliveZombieFilter) {
            switch (zombie.Type()) {
            case AGARGANTUAR:
            case AGIGA_GARGANTUAR: {
                _gigaVec.push_back(&zombie);
                break;
            }
            case ABALLOON_ZOMBIE: {
                _minBalloonX = std::min(_minBalloonX, zombie.Abscissa());
                break;
            }
            case AJACK_IN_THE_BOX_ZOMBIE: {
                _boxZombieVec.push_back(&zombie);
                break;
            }
            case AZOMBONI: {
                _zomboniVec.push_back(&zombie);
                break;
            }
            default: {
                break;
            }
            }
        }
    }

    // 生成有破阵危险的位置列表
    void _GetDangerGridVec()
    {
        static int lock = -1;
        // 一帧只观测一次
        if (lock == AGetMainObject()->GameClock()) {
            return;
        }
        lock = AGetMainObject()->GameClock();
        if (_gigaVec.empty())
            return;

        _dangerGridVec.clear();
        // 第一步：生成最靠左的红眼的位置列表
        minGigaX = {800, 800, 800, 800, 800, 800};
        for (const auto& gigaPtr : _gigaVec) {
            int row = gigaPtr->Row();
            int col = (gigaPtr->Abscissa() - 11) / 80;
            col = std::clamp(col, 0, 8);
            minGigaX[row] = std::min(minGigaX[row], gigaPtr->Abscissa() - leftX[row]);
            if (minGigaX[row] < 800) {
                _dangerGridVec.push_back({row + 1, col + 1});
            }
        }
        if (_dangerGridVec.empty()) {
            return;
        }
        // 去重，防止遍历重复的格子
        RemoveDuplicates(_dangerGridVec);
        // 第二步：根据坐标对每行的位置进行排序
        std::ranges::sort(_dangerGridVec, [=](const AGrid& lhs, const AGrid& rhs) {
            return minGigaX[lhs.row - 1] < minGigaX[rhs.row - 1];
        });
    }

    // 返回某行红眼血量总和
    int __GetGigaHp(int row)
    {
        int Totalhp = 0;
        for (const auto& ptr : _gigaVec) {
            if (ptr->Type() == AGIGA_GARGANTUAR && ptr->Row() + 1 == row)
                Totalhp += ptr->Hp();
        }
        return Totalhp;
    };

    // 某格子植物受锤最短用时（不受锤返回0）
    int _GridHitCD(int row, int col, APlantType plant_type = APEASHOOTER)
    {
        int hitCD = 0;
        for (const auto& zombie : _gigaVec) {
            if (zombie->State() == 70 && cresc::JudgeHit(zombie, row, col, plant_type)) {
                float animation_progress = zombie->AnimationPtr()->CirculationRate();
                if (animation_progress >= 0.648)
                    continue;

                int remain_time = int((zombie->SlowCountdown() > 0 ? 414 : 207) * (0.648 - animation_progress)) + 1 + zombie->FreezeCountdown();
                if (remain_time > hitCD)
                    hitCD = remain_time;
            }
        }
        return hitCD;
    }
};

#endif //!__PROCESSOR_H__