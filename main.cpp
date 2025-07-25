/*
 * @Author: TemporalShards
 * @Date: 2024-01-30 16:46:39
 * @Last Modified by: TemporalShards
 * @Last Modified Date: 2025-07-15 12:17:44
 * @Description：FE智能无炮 挂机脚本
 */
#include "processor.h"

#if __has_include("mod/mod.h")
#include "mod/mod.h"
#define HEADER_MOD_EXIST 1
#else
#define HEADER_MOD_EXIST 0
#warning ("Recommend to install the Extension 'Reisen/mod' before running this script")
#endif

#if __AVZ_VERSION__ < 241030
#error ("your AvZ version is too low, may not support this script")
#endif

// #define TEST

static constexpr auto MessageStr = LR"(欢迎使用 FE.智能无炮 挂机脚本!

作者：TemporalShards

本脚本基于AvZ2.8.2 2024_10_30版本开发，不保证对更低版本的兼容性

脚本有3种运行模式：
1.DEMO 在脚本开头添加 '#define DEMO'切换至此模式。
该模式下脚本运行结束后需在返回主界面才能重新载入，不能用于挂机。适用于录制演示视频。

2.TEST 在脚本开头添加 '#define TEST'切换至此模式。
该模式下脚本会长期运行并处于跳帧状态（游戏画面不会刷新），在破阵时结束跳帧并进行SL，适用于长期冲关测试。
注意：该模式下会在 pvz根目录\areplay文件夹 生成用于记录冲关信息的文件。在下一次脚本注入时会读取文件，如果文件不存在会自动创建并清空之前的冲关信息。

3.默认模式：
该模式下脚本会长期运行但不会主动进行跳帧和自动SL，但您可以按键盘上的'T'键进行跳帧，适用于测试脚本稳定性。

三种模式下您都可以通过键盘控制游戏运行，除结束跳帧外，其余按键效果均需要在PvZ窗口为顶层窗口时才会生效
按键功能如下：
A：速度加快一档
D：速度减慢一档
W：速度恢复原速
Q：结束跳帧，选择是否让脚本立即停止运行
其中游戏速度有7个档位：0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0

点击确定结束此对话框并继续运行脚本，点击取消结束脚本的运行。
)";

// 必选的卡片：冰|核|南瓜|大喷|樱桃|辣椒|窝瓜
static const std::vector<int> basicCards = {AICE_SHROOM, ADOOM_SHROOM, APUMPKIN, AFUME_SHROOM, ACHERRY_BOMB, AJALAPENO, ASQUASH};
// 辣椒的最早生效时间
static constexpr int JalaTime = 226;
// 樱桃的最早生效时间
static constexpr int CherryTime = 401;
// 核的最早生效时间
static constexpr int NuclearTime = 1000;
// 窝瓜的最早生效时间
static constexpr int SquashTime = 318;

#ifdef TEST
#include <filesystem>

constexpr auto RELOAD_MODE = AReloadMode::MAIN_UI_OR_FIGHT_UI;
constexpr float GAME_SPEED = 10.0;
constexpr int SELECT_CARDS_INTERVAL = 0;
static constexpr int TARGET_FLAGS = 10000;

ALogger<AFile> Logger("areplay\\log.txt");
constexpr auto GAME_DAT_PATH = "C:\\ProgramData\\PopCap Games\\PlantsVsZombies\\userdata\\game2_14.dat";
constexpr auto TMP_DAT_PATH = "areplay\\tmp.dat";
constexpr auto RECORD_PATH = "areplay\\record.txt";

static int startFlag = 0, endFlag = 0, completedFlags = 0, loadCount = 0;

// 单独用一个txt记录之前冲关的信息，每次注入脚本后读取txt
AOnAfterInject({
    std::ifstream file(RECORD_PATH);
    if (!file.is_open())
        return;

    std::string line;

    while (getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, ':')) {
            if (key == "startFlag") {
                std::string value;
                if (std::getline(iss, value) && !value.empty()) {
                    startFlag = std::stoi(value);
                }
            } else if (key == "endFlag") {
                std::string value;
                if (std::getline(iss, value) && !value.empty()) {
                    endFlag = std::stoi(value);
                }
            } else if (key == "completedFlags") {
                std::string value;
                if (std::getline(iss, value) && !value.empty()) {
                    completedFlags = std::stoi(value);
                }
            } else if (key == "loadCount") {
                std::string value;
                if (std::getline(iss, value) && !value.empty()) {
                    loadCount = std::stoi(value);
                }
            }
        }
    }
    file.close();
});

// 增加容错率的 SL 大法
void SaveLoad(const std::string& reason)
{
    Logger.Error(reason);
    ABackToMain(false);
    AEnterGame(AAsm::GameMode::SURVIVAL_ENDLESS_STAGE_4);
}

AOnEnterFight({
    auto filePath = std::filesystem::path(RECORD_PATH);
    std::ofstream outFile(filePath.c_str());
    if (outFile.is_open()) {
        if (startFlag == 0) {
            startFlag = 2 * AGetMainObject()->CompletedRounds();
        }
        outFile << "startFlag: " << startFlag << std::endl;
        endFlag = 2 * AGetMainObject()->CompletedRounds();
        outFile << "endFlag: " << endFlag << std::endl;
        completedFlags = endFlag - startFlag;
        outFile << "completedFlags: " << completedFlags << std::endl;
        outFile << "loadCount: " << loadCount << std::endl;
        outFile.close();
    }
});

AOnExitFight({
    // 僵尸赢了
    // 直接跳回主界面
    // 这里肯定是不存档的
    if (AGetPvzBase()->GameUi() == AAsm::ZOMBIES_WON) {
        auto str = std::format("第 {} 轮僵尸进家了，进行第 {} 次SL", 2 * AGetMainObject()->CompletedRounds(), loadCount + 1);
        SaveLoad(str);
    }
    if (AGetPvzBase()->GameUi() != AAsm::LEVEL_INTRO) {
        // 直接倒退回前 6f 的存档 SL
        // 防止死亡循环
        ++loadCount;
        std::filesystem::copy(TMP_DAT_PATH + std::to_string(completedFlags % 6),
            GAME_DAT_PATH, std::filesystem::copy_options::overwrite_existing);
        completedFlags = std::clamp(completedFlags - 6, 0, INT_MAX);
    }
});

// 在进入战斗界面之前复制游戏的存档
AOnBeforeScript(std::filesystem::copy(GAME_DAT_PATH, TMP_DAT_PATH + std::to_string(completedFlags % 6), std::filesystem::copy_options::overwrite_existing));

#elifdef DEMO
constexpr auto RELOAD_MODE = AReloadMode::MAIN_UI;
constexpr float GAME_SPEED = 1.75;
constexpr int SELECT_CARDS_INTERVAL = 1;
#else
constexpr auto RELOAD_MODE = AReloadMode::MAIN_UI_OR_FIGHT_UI;
constexpr float GAME_SPEED = 10;
constexpr int SELECT_CARDS_INTERVAL = 1;

#endif

#define HasZombie(zombie, ...) Judge::IsZombieExist([=](AZombie* zombie) { return __VA_ARGS__; })

ALogger<AConsole> consoleLogger;

void SlowLevel();
void VariableLevel();
void WhiteLevel();
void FastLevel();
void TopSpeedLevel();

// 收尾波有红眼时的附加操作
void HasGigaEndWave();
// 在陆地放核，选取lst中格子CD最短的格子
void PutNuclearOnGround(std::vector<AGrid> list);
// wave20的附加操作
void AddOperationAtWave20();

static const std::unordered_map<Level, std::pair<const char*, AOperation>> levelInfo = {
    {Level::Slow, {"慢速", SlowLevel}},
    {Level::Variable, {"变速", VariableLevel}},
    {Level::White, {"纯白", WhiteLevel}},
    {Level::Fast, {"快速", FastLevel}},
    {Level::TopSpeed, {"极速", TopSpeedLevel}},
};

// 是否用冰
static bool isUseIce = false;

Processor processor;

void AScript()
{
    static bool init = false;
    if (!init) {
        auto pvzHwnd = AGetPvzBase()->Hwnd();
        int ret = MessageBoxW(pvzHwnd, MessageStr, L"About Script", MB_ICONINFORMATION | MB_OKCANCEL | MB_APPLMODAL);
        if (ret == 2) {
            init = false;
            ASetGameSpeed(1.0);
            ASetReloadMode(AReloadMode::NONE);
            AMsgBox::Show("您主动结束了脚本的运行！");
            return;
        }
        init = true;
    }
#ifdef TEST
    static bool continueRun = false;
    if (completedFlags >= TARGET_FLAGS && !continueRun) {
        auto pvzHwnd = AGetPvzBase()->Hwnd();
        std::string str = "目标冲关数" + std::to_string(TARGET_FLAGS) + "已完成，请选择是否继续冲关";
        int ret = MessageBoxW(pvzHwnd, AStrToWstr(str).c_str(), L"About Script", MB_ICONINFORMATION | MB_OKCANCEL | MB_APPLMODAL);
        if (ret == 2) {
            continueRun = false;
            ASetGameSpeed(1.0);
            ASetReloadMode(AReloadMode::NONE);
            AMsgBox::Show("您主动结束了脚本的运行！");
            return;
        }
        continueRun = true;
    }
#endif
    ASetReloadMode(RELOAD_MODE);
    AAsm::SetImprovePerformance(true);
#ifdef HEADER_MOD_EXIST
    EnableModsScoped(AccelerateGame, RemoveFog);
#endif
    KeyController(GAME_SPEED);
    ASetInternalLogger(consoleLogger);
    consoleLogger.SetLevel({ALogLevel::DEBUG, ALogLevel::WARNING, ALogLevel::ERROR});

    auto levelType = processor.GetLevelType();
    consoleLogger.Debug("{}轮 {} 阳光: {}", 2 * AGetMainObject()->CompletedRounds(), levelInfo.at(levelType).first, AGetMainObject()->Sun());
    levelInfo.at(levelType).second();

    auto condition = [=] -> bool {
        auto melon_indices = AGetPlantIndices({{1, 3}, {2, 3}, {5, 3}, {6, 3}, {1, 1}, {6, 1}, {4, 4}}, AWINTER_MELON);
        auto gloom_indices = AGetPlantIndices({{1, 2}, {2, 2}, {5, 2}, {6, 2}, {3, 5}, {3, 6}, {4, 5}, {4, 6}}, AGLOOM_SHROOM);
        auto flower_indices = AGetPlantIndices({{3, 1}, {3, 2}, {3, 3}, {3, 4}, {4, 1}, {4, 2}, {4, 3}}, ATWIN_SUNFLOWER);
        auto leaf_indices = AGetPlantIndices({{2, 1}, {5, 1}}, AUMBRELLA_LEAF);
        auto allIndices = MergeVectors<int>(melon_indices, gloom_indices, flower_indices, leaf_indices);
        for (const auto& idx : allIndices) {
            if (idx < 0) {
#ifdef TEST
                auto str = std::format("第 {} 轮 丢失场上永久植物，进行第 {} 次SL", 2 * AGetMainObject()->CompletedRounds(), loadCount + 1);
                SaveLoad(str);
                consoleLogger.Error(str);
#endif
                return false;
            }
        }
        static std::vector<AGrid> ladderList = {{2, 2}, {5, 2}, {2, 3}, {5, 3}};
        for (const auto& [row, col] : ladderList) {
            if (!Judge::IsExistLadder(row, col)) {
#ifdef TEST
                auto str = std::format("第 {} 轮 丢失梯子，进行第 {} 次SL", 2 * AGetMainObject()->CompletedRounds(), loadCount + 1);
                SaveLoad(str);
                consoleLogger.Error(str);
#endif
                return false;
            }
        }
        return (GetAsyncKeyState('Q') & 0x8001) != 0x8001 || AGetMainObject()->Sun() < 200;
    };

#ifdef TEST
    Logger.SetHeaderStyle("");
    Logger.Debug("{}轮 {} 阳光: {}", 2 * AGetMainObject()->CompletedRounds(), levelInfo.at(levelType).first, AGetMainObject()->Sun());
    ASkipTick(condition);
#else
    AConnect(AKey('T'), [=] { ASkipTick(condition); });
#endif
}

void PutNuclearOnGround(std::vector<AGrid> list)
{
    ACoLaunch([=, list = std::move(list)] -> ACoroutine {
        co_await [=, list = std::move(list)] mutable {
            std::ranges::sort(list, [=](auto lhs, auto rhs) { return cresc::GridCD(lhs) < cresc::GridCD(rhs); });
            NuclearGrid = list.front();
            return Judge::IsGridUsable(NuclearGrid) && AIsSeedUsable(ADOOM_SHROOM);
        };
        AGetInternalLogger()->Debug("N {}", NuclearGrid);
        UseCard(ADOOM_SHROOM, NuclearGrid.row, NuclearGrid.col);
    });
}

void HasGigaEndWave()
{
    ACoLaunch([=] -> ACoroutine {
        co_await [=] {
            // 首先确定核炸哪个半场
            int main_row = processor.GetHpMaxRow({1, 6});
            int coast_row = main_row == 1 ? 2 : 5;
            std::vector<AGrid> grids = {{coast_row, 7}, {coast_row, 8}, {main_row, 7}, {main_row, 8}};
            std::ranges::sort(grids, [=](auto lhs, auto rhs) { return cresc::GridCD(lhs) < cresc::GridCD(rhs); });
            NuclearGrid = grids.front();
            return Judge::IsGridUsable(NuclearGrid) && AIsSeedUsable(ADOOM_SHROOM);
        };
        if (processor.GetHpMaxRow({1, 6}) == -1) {
            co_return; // 1，6路无红眼，不放核
        }
        AGetInternalLogger()->Debug("N {}", NuclearGrid);
        UseCard(ADOOM_SHROOM, NuclearGrid.row, NuclearGrid.col);
    });
}

void SlowLevel()
{
    std::vector<int> othercards = {ALILY_PAD, AM_DOOM_SHROOM, cresc::TypeExist(ABALLOON_ZOMBIE) ? ABLOVER : APUFF_SHROOM};
    ASelectCards(MergeVectors<int>(basicCards, othercards), SELECT_CARDS_INTERVAL);
    for (int wave : {1, 5, 10, 14, 18}) {
        AConnect(ATime(wave, CherryTime), [=] {
            if (ANowWave() == 10) {
                auto place = processor.SquashRow() == 6 ? PlaceType::UP : PlaceType::DOWN;
                processor.UseCherry(place);
            } else {
                processor.UseCherry();
            }
        });
    }
    for (int wave : {2, 6, 11, 15}) {
        AConnect(ATime(wave, JalaTime - 100), [=] {
            if (AGetZombieTypeList()[AGIGA_GARGANTUAR] && ANowWave() == 11) {
                int row = minGigaX[0] <= minGigaX[5] ? 1 : 6;
                processor.UseJalapeno(row);
            } else {
                processor.UseJalapeno();
            }
        });
        AConnect(ATime(wave, SquashTime), [=] { processor.UseSquash(); });
    }
    for (int wave : {3, 7, 12, 16}) {
        AConnect(ATime(wave, NuclearTime - 100), [=]() -> ACoroutine {
            processor.UseNuclear();
            co_await [=] { return CheckPlant(NuclearGrid, ADOOM_SHROOM); };
            int m_doom_cd = std::max(cresc::CardCD(AM_DOOM_SHROOM), cresc::CardCD(ALILY_PAD));
            int delay_time = m_doom_cd > 210 + 100 ? m_doom_cd - 100 : 210;
            isUseIce = Judge::IsSeedUsable(AICE_SHROOM, delay_time);
            if (isUseIce) {
                co_await ANowDelayTime(210);
                processor.UseIce();
            }
        });
    }
    for (int wave : {4, 8, 13, 17}) {
        AConnect(ATime(wave, -100), [=] -> ACoroutine {
            int time = isUseIce ? NuclearTime : 415;
            co_await ATime(wave, time - 320 - 100);
            processor.UseCopyNuclear();
        });
    }
    for (int wave : {9}) {
        AConnect(ATime(wave, CherryTime), [=] -> ACoroutine {
            processor.UseCherry();
            processor.UseJalapeno();
            co_await [=] { return CheckPlant(jalapenoGrid, AJALAPENO); };
            if (Judge::IsRefreshZombie(wave, AGIGA_GARGANTUAR)) {
                HasGigaEndWave();
            }
            int row = jalapenoGrid.row;
            co_await [=] { return !HasZombie(z, z->Row() + 1 == row && z->Abscissa() >= 6.5 * 80) && AIsSeedUsable(ASQUASH) && Judge::IsGridUsable(row, 9); };
            processor.UseSquash(row, false);
        });
    }
    for (int wave : {19}) {
        AConnect(ATime(wave, JalaTime - 100), [=]() -> ACoroutine {
            processor.UseJalapeno();
            if (Judge::IsRefreshZombie(wave, AGIGA_GARGANTUAR)) {
                co_await [=] { return CheckPlant(jalapenoGrid, AJALAPENO); };
                HasGigaEndWave();
            }
        });
        AConnect(ATime(wave, SquashTime), [=] { processor.UseSquash(); });
    }
    for (int wave : {20}) {
        AConnect(ATime(wave, 400), [=]() -> ACoroutine {
            processor.UseCopyNuclear();
            // 这个时间点是为了借助信仰提高稳定性）
            co_await ATime(wave, 1437);
            if (Judge::IsRefreshZombie(wave, AGIGA_GARGANTUAR)) {
                AddOperationAtWave20();
            }
        });
    }
    for (int wave : {10, 20}) {
        AConnect(ATime(wave, 400 - 100), [=] { processor.UseIce(); });
    }
}

void VariableLevel()
{
    std::vector<int> othercards = {ALILY_PAD, AM_DOOM_SHROOM, cresc::TypeExist(ABALLOON_ZOMBIE) ? ABLOVER : APUFF_SHROOM};
    ASelectCards(MergeVectors<int>(basicCards, othercards), SELECT_CARDS_INTERVAL);
    for (int wave : {1, 5, 10, 14}) {
        AConnect(ATime(wave, CherryTime), [=] {
            if (wave == 10) {
                auto place = processor.SquashRow() == 6 ? PlaceType::UP : PlaceType::DOWN;
                processor.UseCherry(place);
            } else {
                processor.UseCherry();
            }
        });
    }
    for (int wave : {2, 6, 11, 15}) {
        AConnect(ATime(wave, JalaTime - 100), [=] {
            if (AGetZombieTypeList()[AGIGA_GARGANTUAR] && ANowWave() == 11) {
                int row = minGigaX[0] <= minGigaX[5] ? 1 : 6;
                processor.UseJalapeno(row);
            } else {
                processor.UseJalapeno();
            }
        });
        AConnect(ATime(wave, SquashTime), [=] { processor.UseSquash(); });
    }
    for (int wave : {3, 7, 12}) {
        AConnect(ATime(wave, NuclearTime - 100), [=] -> ACoroutine {
            processor.UseNuclear();
            co_await [=] { return CheckPlant(NuclearGrid, ADOOM_SHROOM); };
            int m_doom_cd = std::max(cresc::CardCD(AM_DOOM_SHROOM), cresc::CardCD(ALILY_PAD));
            int delay_time = m_doom_cd > 210 + 100 ? m_doom_cd - 100 : 210;
            isUseIce = Judge::IsSeedUsable(AICE_SHROOM, delay_time);
            if (isUseIce) {
                co_await ANowDelayTime(210);
                processor.UseIce();
            }
        });
    }
    for (int wave : {4, 8, 13}) {
        AConnect(ATime(wave, -100), [=] -> ACoroutine {
            int time = isUseIce ? NuclearTime : 415;
            co_await ATime(wave, time - 320 - 100);
            processor.UseCopyNuclear();
        });
    }
    for (int wave : {9}) {
        AConnect(ATime(wave, CherryTime), [=] -> ACoroutine {
            processor.UseCherry();
            processor.UseJalapeno();
            co_await [=] { return CheckPlant(jalapenoGrid, AJALAPENO); };
            if (Judge::IsRefreshZombie(wave, AGIGA_GARGANTUAR)) {
                HasGigaEndWave();
            }
            int row = jalapenoGrid.row;
            co_await [=] { return !HasZombie(z, z->Row() + 1 == row && z->Abscissa() >= 6.5 * 80) && AIsSeedUsable(ASQUASH) && Judge::IsGridUsable(row, 9); };
            processor.UseSquash(row, false);
        });
    }
    // wave17核弹炸两波
    for (int wave : {17}) {
        AConnect(ATime(wave, 401 - 100), [=] { processor.UseNuclear(); });
    }
    // 出怪有冰车w18就用核，否则w20复制核CD可能不够
    if (AGetZombieTypeList()[AZOMBONI]) {
        for (auto wave : {18}) {
            AConnect(ATime(wave, 0), [=] { processor.UseCopyNuclear(); });
        }
    }
    for (int wave : {19}) {
        AConnect(ATime(wave, CherryTime), [=] -> ACoroutine {
            processor.UseCherry();
            co_await [=] { return CheckPlant(cherryGrid, ACHERRY_BOMB); };
            int row = processor.CherryPos() == PlaceType::UP ? 6 : 1;
            processor.UseSquash(row, false);
            if (Judge::IsRefreshZombie(wave, AGIGA_GARGANTUAR)) {
                HasGigaEndWave();
                processor.UseCherry();
            }
        });
    }
    for (int wave : {20}) {
        AConnect(ATime(wave, 400), [=] -> ACoroutine {
            processor.UseCopyNuclear();
            // 这个时间点是为了借助信仰提高稳定性）
            co_await ATime(wave, 1437);
            AddOperationAtWave20();
        });
    }
    for (int wave : {10, 20}) {
        AConnect(ATime(wave, 400 - 100), [=] { processor.UseIce(); });
    }
}

void WhiteLevel()
{
    std::vector<int> othercards = {ALILY_PAD, AM_ICE_SHROOM, cresc::TypeExist(ABALLOON_ZOMBIE) ? ABLOVER : APUFF_SHROOM};
    ASelectCards(MergeVectors<int>(basicCards, othercards), SELECT_CARDS_INTERVAL);
    for (int wave : {1, 6, 11, 16}) {
        AConnect(ATime(wave, CherryTime), [=] { processor.UseCherry(); });
    }
    for (int wave : {2, 7, 12, 17}) {
        AConnect(ATime(wave, JalaTime - 100), [=] { processor.UseJalapeno(); });
        AConnect(ATime(wave, SquashTime), [=] { processor.UseSquash(); });
    }
    for (int wave : {4, 9, 14, 19}) {
        AConnect(ATime(wave, NuclearTime - 100), [=] { processor.UseNuclear(); });
    }
    for (int wave : {3, 8, 13, 18}) {
        AConnect(ATime(wave, -200), [=] { processor.UseCopyIce(); });
    }
    for (int wave : {5, 15}) {
        AConnect(ATime(wave, -99), [=] { processor.UseIce(); });
    }
    for (int wave : {10, 20}) {
        AConnect(ATime(wave, 400 - 100), [=] { processor.UseIce(); });
    }
}

void FastLevel()
{
    std::vector<int> othercards = {ALILY_PAD, ASPIKEWEED, cresc::TypeExist(ABALLOON_ZOMBIE) ? ABLOVER : APUFF_SHROOM};
    ASelectCards(MergeVectors<int>(basicCards, othercards), SELECT_CARDS_INTERVAL);
    for (int wave : {1, 5, 9, 11, 15, 19}) {
        AConnect(ATime(wave, CherryTime), [=] -> ACoroutine {
            processor.UseCherry();
            if (wave == 9 || wave == 19) {
                co_await [=] { return CheckPlant(cherryGrid, ACHERRY_BOMB); };
                int row = processor.CherryPos() == PlaceType::UP ? 6 : 1;
                processor.UseSquash(row);
            }
        });
    }
    for (int wave : {2, 6, 12, 16}) {
        AConnect(ATime(wave, JalaTime - 100), [=] { processor.UseJalapeno(); });
        AConnect(ATime(wave, SquashTime), [=] { processor.UseSquash(); });
    }
    for (int wave : {3, 7, 10, 13, 17, 19, 20}) {
        AConnect(ATime(wave, 1), [=] {
            processor.UseSpikeweed(1);
            processor.UseSpikeweed(6);
        });
        AConnect(ATime(wave, -99), [=] {
            if (Judge::IsSeedUsable(AICE_SHROOM, 1000) && ARangeIn(wave, {3, 7, 17})) {
                processor.UseIce();
            }
        });
    }
    for (int wave : {4, 8, 14, 18}) {
        AConnect(ATime(wave, NuclearTime - 100), [=] { processor.UseNuclear(); });
    }
    for (int wave : {10, 20}) {
        AConnect(ATime(wave, 400 - 100), [=] { processor.UseIce(); });
    }
}

void TopSpeedLevel()
{
    std::vector<int> othercards = {ASPIKEWEED, AM_ICE_SHROOM, cresc::TypeExist(ABALLOON_ZOMBIE) ? ABLOVER : APUFF_SHROOM};
    ASelectCards(MergeVectors<int>(basicCards, othercards), SELECT_CARDS_INTERVAL);
    for (int wave : {1, 10}) {
        AConnect(ATime(wave, 688 - 100), [=] -> ACoroutine {
            PutNuclearOnGround({{2, 9}, {5, 9}, {2, 8}, {5, 8}});
            co_await [=] { return CheckPlant(NuclearGrid, ADOOM_SHROOM); };
            int row = NuclearGrid.row >= 5 ? 1 : 6;
            processor.UseSpikeweed(row);
        });
    }
    for (int wave : {2, 6, 11, 15, 19}) {
        AConnect(ATime(wave, CherryTime), [=] -> ACoroutine {
            processor.UseCherry();
            co_await [=] { return CheckPlant(cherryGrid, ACHERRY_BOMB); };
            if (wave == 19) {
                int row = processor.CherryPos() == PlaceType::UP ? 6 : 1;
                processor.UseSpikeweed(row);
            }
        });
    }
    for (int wave : {3, 7, 12, 16}) {
        AConnect(ATime(wave, JalaTime - 100), [=] { processor.UseJalapeno(); });
        AConnect(ATime(wave, SquashTime), [=] { processor.UseSquash(); });
    }
    for (int wave : {4, 5, 8, 9, 13, 14, 17, 18, 20}) {
        AConnect(ATime(wave, 1), [=] {
            processor.UseSpikeweed(1);
            processor.UseSpikeweed(6);
        });
    }
    for (int wave : {4, 8, 14, 18}) {
        AConnect(ATime(wave, -200), [=] { processor.UseCopyIce(); });
    }
    for (int wave : {5, 9, 13, 17}) {
        AConnect(ATime(wave, -99), [=] { processor.UseIce(); });
    }
    for (int wave : {10}) {
        AConnect(ATime(wave, 400 - 100 - 320), [=] { processor.UseCopyIce(); });
    }
    for (int wave : {20}) {
        AConnect(ATime(wave, 400 - 100), [=] { processor.UseIce(); });
    }
}

void AddOperationAtWave20()
{
    ACoLaunch([=] -> ACoroutine {
        co_await [=] { return AIsSeedUsable(AJALAPENO); };
        processor.UseJalapeno(processor.GetHpMaxRow({1, 6}));
        co_await [=] { return CheckPlant(jalapenoGrid, AJALAPENO); };
        NuclearGrid.row = jalapenoGrid.row == 1 ? 5 : 2;
        int otherRow = NuclearGrid.row == 5 ? 6 : 1;
        PutNuclearOnGround({{NuclearGrid.row, 8}, {NuclearGrid.row, 7}, {otherRow, 7}});
        co_await [=] { return CheckPlant(NuclearGrid, ADOOM_SHROOM); };
        processor.UseSquash(processor.GetHpMaxRow({1, 6}), false);
    });
}