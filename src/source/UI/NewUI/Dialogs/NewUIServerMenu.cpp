#include "stdafx.h"

#include "UI/NewUI/Dialogs/NewUIServerMenu.h"

#include <cwctype>

#include "Audio/DSPlaySound.h"
#include "Character/CharacterManager.h"
#include "I18N/All.h"
#include "MUHelper/MuHelper.h"
#include "Network/Server/WSclient.h"
#include "UI/Legacy/UIControls.h"
#include "UI/Legacy/UIManager.h"
#include "UI/NewUI/NewUISystem.h"

using namespace SEASON3B;

namespace
{
    struct UiColor
    {
        float red;
        float green;
        float blue;
        float alpha;
    };

    constexpr int ButtonHeight = 30;
    constexpr int HomeButtonWidth = 176;
    constexpr int HomeLeftX = 18;
    constexpr int HomeRightX = 206;
    constexpr int FullButtonWidth = 220;
    constexpr int FullButtonX = (CNewUIServerMenu::WindowWidth - FullButtonWidth) / 2;
    constexpr int RankingTabWidth = 116;
    constexpr int BackButtonWidth = 150;
    constexpr int BackButtonX = (CNewUIServerMenu::WindowWidth - BackButtonWidth) / 2;
    constexpr int BackButtonY = 314;
    constexpr int CloseButtonSize = 22;
    constexpr int CloseButtonX = CNewUIServerMenu::WindowWidth - CloseButtonSize - 14;
    constexpr int CloseButtonY = 14;
    constexpr int CommandListX = 20;
    constexpr int CommandListY = 76;
    constexpr int CommandListWidth = 360;
    constexpr int CommandListHeight = 186;
    constexpr int CommandRowX = 24;
    constexpr int CommandRowY = 82;
    constexpr int CommandRowWidth = 340;
    constexpr int CommandRowHeight = 54;
    constexpr int CommandRowStride = 60;
    constexpr int CommandScrollTrackX = 372;
    constexpr int CommandScrollTrackY = CommandRowY;
    constexpr int CommandScrollTrackWidth = 6;
    constexpr int CommandScrollTrackHeight = (CommandRowStride * 3) - 6;
    constexpr int CommandScrollHitPadding = 4;
    constexpr int MinimumScrollThumbHeight = 24;

    constexpr UiColor Shadow{0.0f, 0.0f, 0.0f, 0.62f};
    constexpr UiColor OuterBorder{0.24f, 0.31f, 0.37f, 0.98f};
    constexpr UiColor InnerBorder{0.07f, 0.10f, 0.13f, 0.98f};
    constexpr UiColor WindowFill{0.025f, 0.045f, 0.06f, 0.96f};
    constexpr UiColor HeaderFill{0.055f, 0.095f, 0.13f, 0.98f};
    constexpr UiColor ContentBorder{0.12f, 0.20f, 0.26f, 0.98f};
    constexpr UiColor ContentFill{0.012f, 0.026f, 0.036f, 0.90f};
    constexpr UiColor Gold{0.78f, 0.58f, 0.18f, 0.96f};
    constexpr UiColor ButtonBorder{0.18f, 0.31f, 0.40f, 0.98f};
    constexpr UiColor ButtonFill{0.045f, 0.085f, 0.115f, 0.98f};
    constexpr UiColor ButtonHover{0.075f, 0.15f, 0.20f, 0.98f};
    constexpr UiColor ButtonPressed{0.025f, 0.05f, 0.07f, 0.98f};
    constexpr UiColor RowBorder{0.075f, 0.24f, 0.34f, 0.92f};
    constexpr UiColor RowDark{0.012f, 0.035f, 0.05f, 0.88f};
    constexpr UiColor RowLight{0.025f, 0.06f, 0.08f, 0.88f};
    constexpr UiColor SuccessBorder{0.12f, 0.52f, 0.34f, 0.96f};
    constexpr UiColor FailureBorder{0.62f, 0.20f, 0.16f, 0.96f};

    constexpr std::uint8_t ResetEnabled = 1;
    constexpr std::uint8_t ResetLevelMet = 2;
    constexpr std::uint8_t ResetZenMet = 4;
    constexpr std::uint8_t ResetItemMet = 8;
    constexpr std::uint8_t ResetLimitMet = 16;

    void RenderUiRectangle(float x, float y, float width, float height, const UiColor& color)
    {
        glColor4f(color.red, color.green, color.blue, color.alpha);
        RenderColor(x, y, width, height);
        EndRenderColor();
    }

    void RenderUiPanel(float x, float y, float width, float height, const UiColor& border, const UiColor& fill)
    {
        RenderUiRectangle(x, y, width, height, border);
        RenderUiRectangle(x + 1, y + 1, width - 2, height - 2, fill);
    }

}

CNewUIServerMenu::CNewUIServerMenu()
    : m_manager(nullptr),
      m_position{0, 0},
      m_page(Page::Home),
      m_actionCount(0),
      m_pendingAction(-1),
      m_scheduleLoaded(false),
      m_scheduleRequestPending(false),
      m_rankingLoaded(false),
      m_rankingRequestPending(false),
      m_resetRequirementsLoaded(false),
      m_resetRequirementsRequestPending(false),
      m_commandsLoaded(false),
      m_commandsRequestPending(false),
      m_rankingFilter(RankingFilter::Level),
      m_commandScrollOffset(0),
      m_commandScrollDragging(false),
      m_commandScrollDragOffset(0),
      m_scheduleReceivedAt(std::chrono::steady_clock::now()),
      m_scheduleRequestedAt(std::chrono::steady_clock::now()),
      m_rankingRequestedAt(std::chrono::steady_clock::now()),
      m_resetRequirementsRequestedAt(std::chrono::steady_clock::now()),
      m_commandsRequestedAt(std::chrono::steady_clock::now())
{
}

CNewUIServerMenu::~CNewUIServerMenu()
{
    Release();
}

bool CNewUIServerMenu::Create(CNewUIManager* manager, int x, int y)
{
    if (manager == nullptr)
    {
        return false;
    }

    m_manager = manager;
    m_manager->AddUIObj(INTERFACE_SERVER_MENU, this);
    m_position = {x, y};
    ConfigureBackButton();
    ConfigureCloseButton();
    SetPage(Page::Home);
    Show(false);
    return true;
}

void CNewUIServerMenu::Release()
{
    if (m_manager != nullptr)
    {
        m_manager->RemoveUIObj(this);
        m_manager = nullptr;
    }
}

void CNewUIServerMenu::OpeningProcess()
{
    SetPage(Page::Home);
}

void CNewUIServerMenu::ClosingProcess()
{
    m_pendingAction = -1;
    m_commandScrollDragging = false;
}

void CNewUIServerMenu::ConfigureActionButton(
    int index,
    int x,
    int y,
    int width,
    const wchar_t* const* caption)
{
    auto& button = m_actionButtons[index];
    button.ChangeText(caption);
    button.ChangeTextBackColor(RGBA(255, 255, 255, 0));
    button.ChangeButtonInfo(m_position.x + x, m_position.y + y, width, ButtonHeight);
}

void CNewUIServerMenu::ConfigureBackButton()
{
    m_backButton.ChangeText(&I18N::Game::Back);
    m_backButton.ChangeTextBackColor(RGBA(255, 255, 255, 0));
    m_backButton.ChangeButtonInfo(
        m_position.x + BackButtonX,
        m_position.y + BackButtonY,
        BackButtonWidth,
        ButtonHeight);
}

void CNewUIServerMenu::ConfigureCloseButton()
{
    m_closeButton.ChangeText(nullptr);
    m_closeButton.ChangeTextBackColor(RGBA(255, 255, 255, 0));
    m_closeButton.ChangeButtonInfo(
        m_position.x + CloseButtonX,
        m_position.y + CloseButtonY,
        CloseButtonSize,
        CloseButtonSize);
}

void CNewUIServerMenu::SetPage(Page page)
{
    m_page = page;
    m_pendingAction = -1;
    m_actionCount = 0;

    switch (page)
    {
    case Page::Home:
        ConfigureHomeButtons();
        break;
    case Page::Events:
        ConfigureEventButtons();
        RequestEventSchedule();
        break;
    case Page::Rankings:
        ConfigureRankingButtons();
        RequestRanking(m_rankingFilter);
        break;
    case Page::Reset:
        ConfigureResetButtons();
        RequestResetRequirements();
        break;
    case Page::OfflineLevel:
        ConfigureOfflineLevelButtons();
        break;
    case Page::Commands:
        ConfigureCommandButtons();
        break;
    case Page::AvailableCommands:
        ConfigureAvailableCommandButtons();
        RequestAvailableCommands();
        break;
    }

    m_backButton.ChangeText(page == Page::Home ? &I18N::Game::Close388 : &I18N::Game::Back);
}

void CNewUIServerMenu::ConfigureHomeButtons()
{
    ConfigureActionButton(0, HomeLeftX, 76, HomeButtonWidth, &I18N::Game::Events);
    ConfigureActionButton(1, HomeRightX, 76, HomeButtonWidth, &I18N::Game::Rankings);
    ConfigureActionButton(2, HomeLeftX, 118, HomeButtonWidth, &I18N::Game::Reset);
    ConfigureActionButton(3, HomeRightX, 118, HomeButtonWidth, &I18N::Game::OfflineLevel);
    ConfigureActionButton(4, HomeLeftX, 160, HomeButtonWidth, &I18N::Game::Commands);
    ConfigureActionButton(5, HomeRightX, 160, HomeButtonWidth, &I18N::Game::Quests);
    m_actionCount = 6;
}

void CNewUIServerMenu::ConfigureEventButtons()
{
    ConfigureActionButton(0, FullButtonX, 258, FullButtonWidth, &I18N::Game::Refresh);
    m_actionCount = 1;
}

void CNewUIServerMenu::ConfigureRankingButtons()
{
    ConfigureActionButton(0, 18, 54, RankingTabWidth, &I18N::Game::Level);
    ConfigureActionButton(1, 142, 54, RankingTabWidth, &I18N::Game::Resets);
    ConfigureActionButton(2, 266, 54, RankingTabWidth, &I18N::Game::MasterLevel);
    m_actionCount = 3;
}

void CNewUIServerMenu::ConfigureResetButtons()
{
    ConfigureActionButton(0, HomeLeftX, 260, HomeButtonWidth, &I18N::Game::ResetCharacter);
    ConfigureActionButton(1, HomeRightX, 260, HomeButtonWidth, &I18N::Game::ResetStats);
    m_actionCount = 2;
}

void CNewUIServerMenu::ConfigureOfflineLevelButtons()
{
    ConfigureActionButton(0, FullButtonX, 154, FullButtonWidth, &I18N::Game::MUHelperSettings);
    ConfigureActionButton(1, FullButtonX, 196, FullButtonWidth, &I18N::Game::StartMUHelper);
    ConfigureActionButton(2, FullButtonX, 238, FullButtonWidth, &I18N::Game::ActivateOfflineLevel);
    m_actionCount = 3;
}

void CNewUIServerMenu::ConfigureCommandButtons()
{
    ConfigureActionButton(0, HomeLeftX, 108, HomeButtonWidth, &I18N::Game::MoveMenu);
    ConfigureActionButton(1, HomeRightX, 108, HomeButtonWidth, &I18N::Game::OpenWarehouse);
    ConfigureActionButton(2, FullButtonX, 150, FullButtonWidth, &I18N::Game::AvailableCommands);
    m_actionCount = 3;
}

void CNewUIServerMenu::ConfigureAvailableCommandButtons()
{
    m_actionCount = 0;
}

bool CNewUIServerMenu::UpdateMouseEvent()
{
    if (!IsVisible())
    {
        return true;
    }

    if (m_closeButton.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(INTERFACE_SERVER_MENU);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_page == Page::AvailableCommands &&
        CheckMouseIn(
            m_position.x + CommandListX,
            m_position.y + CommandListY,
            CommandListWidth,
            CommandListHeight) &&
        MouseWheel != 0)
    {
        ScrollCommands(MouseWheel > 0 ? -1 : 1);
        MouseWheel = 0;
        return false;
    }

    if (m_page == Page::AvailableCommands)
    {
        if (m_commandScrollDragging)
        {
            if (IsRelease(VK_LBUTTON))
            {
                m_commandScrollDragging = false;
            }
            else
            {
                UpdateCommandScrollFromMouse();
            }

            return false;
        }

        if (IsPress(VK_LBUTTON) && CheckMouseIn(
            m_position.x + CommandScrollTrackX - CommandScrollHitPadding,
            m_position.y + CommandScrollTrackY,
            CommandScrollTrackWidth + (CommandScrollHitPadding * 2),
            CommandScrollTrackHeight))
        {
            int thumbY = 0;
            int thumbHeight = 0;
            GetCommandScrollThumb(thumbY, thumbHeight);
            const int thumbTop = m_position.y + CommandScrollTrackY + thumbY;
            m_commandScrollDragOffset = MouseY >= thumbTop && MouseY < thumbTop + thumbHeight
                ? MouseY - thumbTop
                : thumbHeight / 2;
            m_commandScrollDragging = true;
            UpdateCommandScrollFromMouse();
            return false;
        }
    }

    for (int i = 0; i < m_actionCount; ++i)
    {
        if (m_actionButtons[i].UpdateMouseEvent())
        {
            ProcessAction(i);
            PlayBuffer(SOUND_CLICK01);
            return false;
        }
    }

    if (m_backButton.UpdateMouseEvent())
    {
        GoBack();
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return !CheckMouseIn(m_position.x, m_position.y, WindowWidth, WindowHeight);
}

bool CNewUIServerMenu::UpdateKeyEvent()
{
    if (!IsVisible())
    {
        return true;
    }

    if (m_page == Page::AvailableCommands)
    {
        if (IsPress(VK_UP))
        {
            ScrollCommands(-1);
            return false;
        }

        if (IsPress(VK_DOWN))
        {
            ScrollCommands(1);
            return false;
        }
    }

    if (IsPress(VK_ESCAPE))
    {
        GoBack();
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return true;
}

bool CNewUIServerMenu::Update()
{
    const auto now = std::chrono::steady_clock::now();
    if (m_page == Page::Events)
    {
        if (m_scheduleRequestPending && now - m_scheduleRequestedAt > std::chrono::seconds(10))
        {
            m_scheduleRequestPending = false;
        }

        if (m_scheduleLoaded && !m_scheduleRequestPending)
        {
            const bool scheduleExpired = std::any_of(
                m_eventSchedule.begin(),
                m_eventSchedule.end(),
                [this](const EventScheduleEntry& entry)
                {
                    return entry.state == 1 && GetRemainingSeconds(entry) == 0;
                });
            if (scheduleExpired)
            {
                RequestEventSchedule();
            }
        }
    }

    if (m_rankingRequestPending && now - m_rankingRequestedAt > std::chrono::seconds(10))
    {
        m_rankingRequestPending = false;
    }

    if (m_resetRequirementsRequestPending && now - m_resetRequirementsRequestedAt > std::chrono::seconds(10))
    {
        m_resetRequirementsRequestPending = false;
    }

    if (m_commandsRequestPending && now - m_commandsRequestedAt > std::chrono::seconds(10))
    {
        m_commandsRequestPending = false;
        m_commandsLoaded = true;
    }

    return true;
}

void CNewUIServerMenu::ProcessAction(int index)
{
    switch (m_page)
    {
    case Page::Home:
        if (index < 5)
        {
            SetPage(static_cast<Page>(index + 1));
        }
        else
        {
            g_pNewUISystem->Hide(INTERFACE_SERVER_MENU);
            if (auto* nameWindow = g_pNewUISystem->GetUI_NewNameWindow())
            {
                nameWindow->ShowQuestTracker();
            }
        }
        break;
    case Page::Events:
        RequestEventSchedule();
        break;
    case Page::Rankings:
        RequestRanking(static_cast<RankingFilter>(index));
        break;
    case Page::Reset:
        if (ConfirmAction(index))
        {
            SendCommand(index == 0 ? L"/reset" : L"/resetstats");
        }
        break;
    case Page::OfflineLevel:
        if (index == 0)
        {
            g_pNewUISystem->Hide(INTERFACE_SERVER_MENU);
            g_pNewUISystem->Show(INTERFACE_MUHELPER);
        }
        else if (index == 1)
        {
            m_pendingAction = -1;
            if (!MUHelper::g_MuHelper.IsActive())
            {
                MUHelper::g_MuHelper.TriggerStart();
            }
        }
        else if (ConfirmAction(index))
        {
            SendCommand(L"/offlevel");
        }
        break;
    case Page::Commands:
        if (index == 0)
        {
            g_pNewUISystem->Hide(INTERFACE_SERVER_MENU);
            g_pNewUISystem->Toggle(INTERFACE_MOVEMAP);
        }
        else if (index == 1)
        {
            m_pendingAction = -1;
            RequestOpenWarehouse();
        }
        else
        {
            SetPage(Page::AvailableCommands);
        }
        break;
    case Page::AvailableCommands:
        break;
    }
}

void CNewUIServerMenu::GoBack()
{
    if (m_page == Page::Home)
    {
        g_pNewUISystem->Hide(INTERFACE_SERVER_MENU);
    }
    else
    {
        SetPage(m_page == Page::AvailableCommands ? Page::Commands : Page::Home);
    }
}

void CNewUIServerMenu::ScrollCommands(int amount)
{
    const int maximumOffset = std::max(
        0,
        static_cast<int>(m_commandEntries.size()) - VisibleCommandRows);
    m_commandScrollOffset = std::clamp(
        m_commandScrollOffset + amount,
        0,
        maximumOffset);
}

void CNewUIServerMenu::UpdateCommandScrollFromMouse()
{
    int thumbY = 0;
    int thumbHeight = 0;
    GetCommandScrollThumb(thumbY, thumbHeight);
    const int maximumOffset = std::max(
        0,
        static_cast<int>(m_commandEntries.size()) - VisibleCommandRows);
    const int thumbTravel = CommandScrollTrackHeight - thumbHeight;
    if (maximumOffset == 0 || thumbTravel <= 0)
    {
        m_commandScrollOffset = 0;
        return;
    }

    const int requestedThumbY = MouseY - m_commandScrollDragOffset -
        (m_position.y + CommandScrollTrackY);
    const int clampedThumbY = std::clamp(requestedThumbY, 0, thumbTravel);
    m_commandScrollOffset = ((clampedThumbY * maximumOffset) + (thumbTravel / 2)) / thumbTravel;
}

void CNewUIServerMenu::GetCommandScrollThumb(int& thumbY, int& thumbHeight) const
{
    const int commandCount = static_cast<int>(m_commandEntries.size());
    if (commandCount <= 0)
    {
        thumbY = 0;
        thumbHeight = CommandScrollTrackHeight;
        return;
    }

    const int maximumOffset = std::max(0, commandCount - VisibleCommandRows);
    const float visibleRatio = std::min(
        1.0f,
        static_cast<float>(VisibleCommandRows) / static_cast<float>(commandCount));
    thumbHeight = std::max(
        MinimumScrollThumbHeight,
        static_cast<int>(CommandScrollTrackHeight * visibleRatio));
    const int thumbTravel = CommandScrollTrackHeight - thumbHeight;
    thumbY = maximumOffset == 0
        ? 0
        : (thumbTravel * m_commandScrollOffset) / maximumOffset;
}

bool CNewUIServerMenu::ConfirmAction(int index)
{
    if (m_pendingAction != index)
    {
        m_pendingAction = index;
        return false;
    }

    m_pendingAction = -1;
    return true;
}

bool CNewUIServerMenu::SendCommand(const wchar_t* command)
{
    if (SocketClient == nullptr || !SocketClient->IsConnected() ||
        SocketClient->ToGameServer() == nullptr || Hero == nullptr)
    {
        if (g_pSystemLogBox != nullptr)
        {
            g_pSystemLogBox->AddText(I18N::Game::ConnectionLost, TYPE_SYSTEM_MESSAGE);
        }
        return false;
    }

    SocketClient->ToGameServer()->SendPublicChatMessage(Hero->ID, command);
    return true;
}

bool CNewUIServerMenu::RequestEventSchedule()
{
    if (SocketClient == nullptr || !SocketClient->IsConnected())
    {
        m_scheduleRequestPending = false;
        return false;
    }

    const BYTE request[] = {0xC1, 0x04, 0xF5, 0x02};
    SocketClient->Send(request, static_cast<int32_t>(sizeof(request)));
    m_scheduleRequestPending = true;
    m_scheduleRequestedAt = std::chrono::steady_clock::now();
    return true;
}

bool CNewUIServerMenu::RequestRanking(RankingFilter filter)
{
    m_rankingFilter = filter;
    for (int i = 0; i < 3; ++i)
    {
        m_actionButtons[i].ChangeTextColor(
            i == static_cast<int>(filter)
                ? RGBA(246, 209, 73, 255)
                : RGBA(255, 255, 255, 255));
    }

    if (SocketClient == nullptr || !SocketClient->IsConnected())
    {
        m_rankingRequestPending = false;
        return false;
    }

    const BYTE request[] = {0xC1, 0x05, 0xF5, 0x04, static_cast<BYTE>(filter)};
    SocketClient->Send(request, static_cast<int32_t>(sizeof(request)));
    m_rankingRequestPending = true;
    m_rankingRequestedAt = std::chrono::steady_clock::now();
    return true;
}

bool CNewUIServerMenu::RequestOpenWarehouse()
{
    if (SocketClient == nullptr || !SocketClient->IsConnected())
    {
        if (g_pSystemLogBox != nullptr)
        {
            g_pSystemLogBox->AddText(I18N::Game::ConnectionLost, TYPE_SYSTEM_MESSAGE);
        }
        return false;
    }

    const BYTE request[] = {0xC1, 0x04, 0xF5, 0x06};
    SocketClient->Send(request, static_cast<int32_t>(sizeof(request)));
    return true;
}

bool CNewUIServerMenu::RequestResetRequirements()
{
    if (SocketClient == nullptr || !SocketClient->IsConnected())
    {
        m_resetRequirementsRequestPending = false;
        return false;
    }

    const BYTE request[] = {0xC1, 0x04, 0xF5, 0x07};
    SocketClient->Send(request, static_cast<int32_t>(sizeof(request)));
    m_resetRequirementsLoaded = false;
    m_resetRequirementsRequestPending = true;
    m_resetRequirementsRequestedAt = std::chrono::steady_clock::now();
    return true;
}

bool CNewUIServerMenu::RequestAvailableCommands()
{
    m_commandEntries.clear();
    m_commandEntriesReceived.clear();
    m_commandScrollOffset = 0;
    m_commandScrollDragging = false;
    m_commandsLoaded = false;

    if (SocketClient == nullptr || !SocketClient->IsConnected())
    {
        m_commandsRequestPending = false;
        return false;
    }

    const BYTE request[] = {0xC1, 0x04, 0xF5, 0x00};
    SocketClient->Send(request, static_cast<int32_t>(sizeof(request)));
    m_commandsRequestPending = true;
    m_commandsRequestedAt = std::chrono::steady_clock::now();
    return true;
}

void CNewUIServerMenu::SetEventSchedule(
    std::uint8_t bloodCastleState,
    std::uint32_t bloodCastleSeconds,
    std::uint8_t devilSquareState,
    std::uint32_t devilSquareSeconds,
    std::uint8_t chaosCastleState,
    std::uint32_t chaosCastleSeconds)
{
    const auto normalizeState = [](std::uint8_t state) -> std::uint8_t
    {
        return state <= 2 ? state : 0;
    };
    m_eventSchedule = {
        EventScheduleEntry{normalizeState(bloodCastleState), bloodCastleSeconds},
        EventScheduleEntry{normalizeState(devilSquareState), devilSquareSeconds},
        EventScheduleEntry{normalizeState(chaosCastleState), chaosCastleSeconds},
    };
    m_scheduleReceivedAt = std::chrono::steady_clock::now();
    m_scheduleLoaded = true;
    m_scheduleRequestPending = false;
}

void CNewUIServerMenu::SetRankingData(RankingFilter filter, std::vector<RankingEntry> entries)
{
    if (filter != m_rankingFilter)
    {
        return;
    }

    m_rankingEntries = std::move(entries);
    m_rankingLoaded = true;
    m_rankingRequestPending = false;
}

void CNewUIServerMenu::SetResetRequirements(ResetRequirements requirements)
{
    m_resetRequirements = std::move(requirements);
    m_resetRequirementsLoaded = true;
    m_resetRequirementsRequestPending = false;
}

void CNewUIServerMenu::SetAvailableCommand(std::uint8_t index, std::uint8_t count, CommandEntry entry)
{
    if (count == 0)
    {
        m_commandEntries.clear();
        m_commandEntriesReceived.clear();
        m_commandsLoaded = true;
        m_commandsRequestPending = false;
        return;
    }

    if (index >= count)
    {
        return;
    }

    if (m_commandEntries.size() != count)
    {
        m_commandEntries.assign(count, {});
        m_commandEntriesReceived.assign(count, false);
    }

    const bool wasLoaded = m_commandsLoaded;
    m_commandEntries[index] = std::move(entry);
    m_commandEntriesReceived[index] = true;
    m_commandsLoaded = std::all_of(
        m_commandEntriesReceived.begin(),
        m_commandEntriesReceived.end(),
        [](bool received) { return received; });
    m_commandsRequestPending = !m_commandsLoaded;
    if (m_commandsLoaded && !wasLoaded)
    {
        std::stable_sort(
            m_commandEntries.begin(),
            m_commandEntries.end(),
            [](const CommandEntry& left, const CommandEntry& right)
            {
                return std::lexicographical_compare(
                    left.command.begin(),
                    left.command.end(),
                    right.command.begin(),
                    right.command.end(),
                    [](wchar_t leftCharacter, wchar_t rightCharacter)
                    {
                        return std::towlower(leftCharacter) < std::towlower(rightCharacter);
                    });
            });
    }
}

std::uint32_t CNewUIServerMenu::GetRemainingSeconds(const EventScheduleEntry& entry) const
{
    if (entry.state != 1)
    {
        return 0;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - m_scheduleReceivedAt).count();
    if (elapsed <= 0)
    {
        return entry.seconds;
    }

    const auto elapsedSeconds = static_cast<std::uint64_t>(elapsed);
    return elapsedSeconds >= entry.seconds
        ? 0
        : entry.seconds - static_cast<std::uint32_t>(elapsedSeconds);
}

bool CNewUIServerMenu::Render()
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);
    RenderFrame();
    RenderTexts();
    RenderButtons();
    DisableAlphaBlend();
    return true;
}

void CNewUIServerMenu::RenderFrame()
{
    RenderUiPanel(m_position.x, m_position.y, WindowWidth, WindowHeight, OuterBorder, InnerBorder);
    RenderUiRectangle(m_position.x + 3, m_position.y + 3, WindowWidth - 6, WindowHeight - 6, WindowFill);

    RenderUiPanel(m_position.x + 8, m_position.y + 7, WindowWidth - 16, 36, ContentBorder, HeaderFill);
    RenderUiRectangle(m_position.x + 20, m_position.y + 41, WindowWidth - 40, 2, Gold);

    RenderUiPanel(
        m_position.x + 12,
        m_position.y + 49,
        WindowWidth - 24,
        WindowHeight - 98,
        ContentBorder,
        ContentFill);

    RenderUiRectangle(m_position.x + 18, m_position.y + WindowHeight - 43, WindowWidth - 36, 1, ContentBorder);
}

const wchar_t* CNewUIServerMenu::GetTitle() const
{
    switch (m_page)
    {
    case Page::Home: return I18N::Game::ServerMenu;
    case Page::Events: return I18N::Game::EventGuide;
    case Page::Rankings: return I18N::Game::RankingCenter;
    case Page::Reset: return I18N::Game::ResetCenter;
    case Page::OfflineLevel: return I18N::Game::OfflineLeveling;
    case Page::Commands: return I18N::Game::CommandCenter;
    case Page::AvailableCommands: return I18N::Game::AvailableCommands;
    }

    return I18N::Game::ServerMenu;
}

void CNewUIServerMenu::RenderWrappedText(const wchar_t* text, int y, DWORD color, int maxLines)
{
    wchar_t lines[4][MAX_TEXT_LENGTH]{};
    const int lineCount = DivideStringByPixel(
        &lines[0][0],
        maxLines < 4 ? maxLines : 4,
        MAX_TEXT_LENGTH,
        text,
        WindowWidth - 40,
        true,
        '#');

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(color);
    for (int i = 0; i < lineCount; ++i)
    {
        g_pRenderText->RenderText(
            m_position.x + 20,
            m_position.y + y + (i * 14),
            lines[i],
            WindowWidth - 40,
            0,
            RT3_SORT_CENTER);
    }
}

void CNewUIServerMenu::RenderTexts()
{
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(255, 221, 120, 255);
    g_pRenderText->RenderText(m_position.x, m_position.y + 18, GetTitle(), WindowWidth, 0, RT3_SORT_CENTER);

    const DWORD normal = RGBA(225, 225, 225, 255);
    const DWORD accent = RGBA(246, 209, 73, 255);
    const DWORD warning = RGBA(255, 150, 100, 255);

    switch (m_page)
    {
    case Page::Home:
        RenderWrappedText(I18N::Game::ChooseAService, 54, normal, 1);
        break;
    case Page::Events:
        RenderWrappedText(I18N::Game::LiveEventScheduleFromServer, 58, normal, 1);
        RenderEventScheduleRow(0, I18N::Game::BloodCastle, 86);
        RenderEventScheduleRow(1, I18N::Game::DevilSquare, 133);
        RenderEventScheduleRow(2, I18N::Game::ChaosCastle, 180);
        break;
    case Page::Rankings:
        RenderRankingTable();
        break;
    case Page::Reset:
        RenderResetRequirements();
        if (m_pendingAction >= 0)
        {
            RenderWrappedText(I18N::Game::ClickTheSameActionAgainToConfirm, 296, warning, 1);
        }
        break;
    case Page::OfflineLevel:
        RenderWrappedText(I18N::Game::OfflineLevelingUsesYourCurrentMUHelperSetup, 58, normal, 2);
        RenderWrappedText(I18N::Game::OfflineLevelingDisconnectWarning, 86, warning, 2);
        RenderWrappedText(I18N::Game::OfflineLevelingLoginStop, 114, normal, 2);
        RenderWrappedText(
            MUHelper::g_MuHelper.IsActive() ? I18N::Game::MUHelperIsRunning : I18N::Game::MUHelperIsStopped,
            138,
            MUHelper::g_MuHelper.IsActive() ? accent : normal,
            1);
        if (m_pendingAction >= 0)
        {
            RenderWrappedText(I18N::Game::ClickTheSameActionAgainToConfirm, 278, warning, 2);
        }
        break;
    case Page::Commands:
        RenderWrappedText(I18N::Game::TheServerValidatesAccessAndCosts, 70, normal, 2);
        break;
    case Page::AvailableCommands:
        RenderAvailableCommands();
        break;
    }
}

void CNewUIServerMenu::RenderEventScheduleRow(int index, const wchar_t* name, int y)
{
    RenderUiPanel(
        m_position.x + 24,
        m_position.y + y,
        WindowWidth - 48,
        36,
        RowBorder,
        index % 2 == 0 ? RowDark : RowLight);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(246, 209, 73, 255);
    g_pRenderText->RenderText(m_position.x + 38, m_position.y + y + 11, name, 130, 0, RT3_SORT_LEFT);

    const wchar_t* status = nullptr;
    wchar_t countdown[96]{};
    if (!m_scheduleLoaded)
    {
        status = m_scheduleRequestPending ? I18N::Game::LoadingEventSchedule : I18N::Game::ScheduleUnavailable;
    }
    else
    {
        const auto& entry = m_eventSchedule[index];
        if (entry.state == 2)
        {
            status = I18N::Game::EntranceIsOpen;
        }
        else if (entry.state == 0)
        {
            status = I18N::Game::ScheduleUnavailable;
        }
        else
        {
            const std::uint32_t seconds = GetRemainingSeconds(entry);
            mu_swprintf(
                countdown,
                L"%ls %02u:%02u:%02u",
                I18N::Game::StartsIn,
                seconds / 3600,
                (seconds % 3600) / 60,
                seconds % 60);
            status = countdown;
        }
    }

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(225, 225, 225, 255);
    g_pRenderText->RenderText(m_position.x + 170, m_position.y + y + 11, status, 192, 0, RT3_SORT_RIGHT);
}

const wchar_t* CNewUIServerMenu::GetRankingValueTitle() const
{
    switch (m_rankingFilter)
    {
    case RankingFilter::Resets: return I18N::Game::Resets;
    case RankingFilter::MasterLevel: return I18N::Game::MasterLevel;
    default: return I18N::Game::Level;
    }
}

void CNewUIServerMenu::RenderRankingTable()
{
    constexpr int TableX = 18;
    constexpr int TableWidth = 364;
    constexpr int HeaderY = 94;
    constexpr int HeaderHeight = 22;
    constexpr int FirstRowY = 118;
    constexpr int RowHeight = 18;

    RenderUiPanel(
        m_position.x + TableX,
        m_position.y + HeaderY,
        TableWidth,
        HeaderHeight,
        Gold,
        HeaderFill);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(246, 209, 73, 255);
    g_pRenderText->RenderText(m_position.x + 24, m_position.y + HeaderY + 6, L"#", 25, 0, RT3_SORT_CENTER);
    g_pRenderText->RenderText(m_position.x + 56, m_position.y + HeaderY + 6, I18N::Game::Player, 136, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(m_position.x + 200, m_position.y + HeaderY + 6, I18N::Game::Class, 112, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(m_position.x + 317, m_position.y + HeaderY + 6, GetRankingValueTitle(), 55, 0, RT3_SORT_RIGHT);

    if (!m_rankingLoaded || m_rankingRequestPending)
    {
        RenderWrappedText(I18N::Game::LoadingRanking, 165, RGBA(225, 225, 225, 255), 1);
        return;
    }

    if (m_rankingEntries.empty())
    {
        RenderWrappedText(I18N::Game::NoRankingEntries, 165, RGBA(225, 225, 225, 255), 1);
        return;
    }

    g_pRenderText->SetFont(g_hFont);
    for (std::size_t index = 0; index < m_rankingEntries.size(); ++index)
    {
        const auto& entry = m_rankingEntries[index];
        const int rowY = FirstRowY + (static_cast<int>(index) * RowHeight);
        const int y = m_position.y + rowY + 5;
        const UiColor& border = index < 3 ? Gold : RowBorder;
        RenderUiPanel(
            m_position.x + TableX,
            m_position.y + rowY,
            TableWidth,
            RowHeight - 1,
            border,
            index % 2 == 0 ? RowDark : RowLight);
        const auto clientClass = gCharacterManager.ChangeServerClassTypeToClientClassType(
            static_cast<SERVER_CLASS_TYPE>(entry.characterClass));
        const wchar_t* className = gCharacterManager.GetCharacterClassText(clientClass);
        const std::uint16_t value = m_rankingFilter == RankingFilter::Resets
            ? entry.resets
            : (m_rankingFilter == RankingFilter::MasterLevel ? entry.masterLevel : entry.level);

        wchar_t rank[8]{};
        wchar_t rankingValue[16]{};
        mu_swprintf(rank, L"%u", static_cast<unsigned>(index + 1));
        mu_swprintf(rankingValue, L"%u", static_cast<unsigned>(value));

        g_pRenderText->SetTextColor(index < 3 ? 255 : 225, index < 3 ? 221 : 225, index < 3 ? 120 : 225, 255);
        g_pRenderText->RenderText(m_position.x + 24, y, rank, 25, 0, RT3_SORT_CENTER);
        g_pRenderText->RenderText(m_position.x + 56, y, entry.name.c_str(), 136, 0, RT3_SORT_LEFT);
        g_pRenderText->RenderText(m_position.x + 200, y, className, 112, 0, RT3_SORT_LEFT);
        g_pRenderText->RenderText(m_position.x + 317, y, rankingValue, 55, 0, RT3_SORT_RIGHT);
    }
}

void CNewUIServerMenu::RenderResetRequirements()
{
    if (!m_resetRequirementsLoaded)
    {
        RenderWrappedText(
            m_resetRequirementsRequestPending
                ? I18N::Game::LoadingResetRequirements
                : I18N::Game::ResetSystemUnavailable,
            150,
            RGBA(225, 225, 225, 255),
            1);
        return;
    }

    if ((m_resetRequirements.flags & ResetEnabled) == 0)
    {
        RenderWrappedText(I18N::Game::ResetSystemUnavailable, 150, RGBA(255, 150, 100, 255), 1);
        return;
    }

    RenderUiPanel(
        m_position.x + 72,
        m_position.y + 60,
        256,
        34,
        Gold,
        HeaderFill);

    wchar_t nextReset[64]{};
    wchar_t resetLimit[64]{};
    mu_swprintf(nextReset, L"%ls: %u", I18N::Game::NextReset, m_resetRequirements.nextReset);
    if (m_resetRequirements.resetLimit == 0)
    {
        mu_swprintf(resetLimit, L"%ls: %ls", I18N::Game::ResetLimit, I18N::Game::Unlimited);
    }
    else
    {
        mu_swprintf(resetLimit, L"%ls: %u", I18N::Game::ResetLimit, m_resetRequirements.resetLimit);
    }

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(246, 209, 73, 255);
    g_pRenderText->RenderText(m_position.x + 82, m_position.y + 72, nextReset, 116, 0, RT3_SORT_CENTER);
    g_pRenderText->RenderText(m_position.x + 202, m_position.y + 72, resetLimit, 116, 0, RT3_SORT_CENTER);

    wchar_t level[64]{};
    wchar_t zen[64]{};
    wchar_t item[96]{};
    wchar_t reward[64]{};
    mu_swprintf(level, L"%u / %u", m_resetRequirements.currentLevel, m_resetRequirements.requiredLevel);
    mu_swprintf(zen, L"%u / %u", m_resetRequirements.currentZen, m_resetRequirements.requiredZen);
    if (m_resetRequirements.requiredItems == 0)
    {
        mu_swprintf(item, L"%ls", I18N::Game::None);
    }
    else
    {
        mu_swprintf(
            item,
            L"%ls  %u / %u",
            m_resetRequirements.itemName.c_str(),
            m_resetRequirements.currentItems,
            m_resetRequirements.requiredItems);
    }
    mu_swprintf(reward, L"+%u", m_resetRequirements.rewardPoints);

    RenderRequirementRow(
        I18N::Game::RequiredLevel,
        level,
        104,
        (m_resetRequirements.flags & ResetLevelMet) != 0);
    RenderRequirementRow(
        I18N::Game::RequiredZen,
        zen,
        139,
        (m_resetRequirements.flags & ResetZenMet) != 0);
    RenderRequirementRow(
        I18N::Game::RequiredItem,
        item,
        174,
        (m_resetRequirements.flags & ResetItemMet) != 0);
    RenderRequirementRow(I18N::Game::RewardPoints, reward, 209, true);

    const bool ready = (m_resetRequirements.flags &
        (ResetLevelMet | ResetZenMet | ResetItemMet | ResetLimitMet)) ==
        (ResetLevelMet | ResetZenMet | ResetItemMet | ResetLimitMet);
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(ready ? 95 : 255, ready ? 220 : 135, ready ? 150 : 105, 255);
    g_pRenderText->RenderText(
        m_position.x + 40,
        m_position.y + 245,
        ready ? I18N::Game::ReadyToReset : I18N::Game::RequirementsNotMet,
        320,
        0,
        RT3_SORT_CENTER);
}

void CNewUIServerMenu::RenderRequirementRow(const wchar_t* label, const wchar_t* value, int y, bool met)
{
    RenderUiPanel(
        m_position.x + 40,
        m_position.y + y,
        320,
        28,
        met ? SuccessBorder : FailureBorder,
        RowDark);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(225, 225, 225, 255);
    g_pRenderText->RenderText(m_position.x + 52, m_position.y + y + 9, label, 120, 0, RT3_SORT_LEFT);
    g_pRenderText->SetTextColor(met ? 100 : 255, met ? 225 : 145, met ? 155 : 105, 255);
    g_pRenderText->RenderText(m_position.x + 176, m_position.y + y + 9, value, 172, 0, RT3_SORT_RIGHT);
}

void CNewUIServerMenu::RenderAvailableCommands()
{
    RenderWrappedText(I18N::Game::CommandsAvailableToThisCharacter, 58, RGBA(225, 225, 225, 255), 1);
    if (!m_commandsLoaded)
    {
        RenderWrappedText(I18N::Game::LoadingCommands, 160, RGBA(225, 225, 225, 255), 1);
        return;
    }

    if (m_commandEntries.empty())
    {
        RenderWrappedText(I18N::Game::NoCommandsAvailable, 160, RGBA(225, 225, 225, 255), 1);
        return;
    }

    const int maximumOffset = std::max(
        0,
        static_cast<int>(m_commandEntries.size()) - VisibleCommandRows);
    m_commandScrollOffset = std::clamp(m_commandScrollOffset, 0, maximumOffset);
    const std::size_t firstIndex = static_cast<std::size_t>(m_commandScrollOffset);
    const std::size_t lastIndex = std::min(
        firstIndex + VisibleCommandRows,
        m_commandEntries.size());
    for (std::size_t index = firstIndex; index < lastIndex; ++index)
    {
        RenderCommandRow(
            m_commandEntries[index],
            CommandRowY + (static_cast<int>(index - firstIndex) * CommandRowStride),
            static_cast<int>(index));
    }

    RenderCommandScrollBar();

    wchar_t position[48]{};
    mu_swprintf(
        position,
        L"%u - %u / %u",
        static_cast<unsigned int>(firstIndex + 1),
        static_cast<unsigned int>(lastIndex),
        static_cast<unsigned int>(m_commandEntries.size()));
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(246, 209, 73, 255);
    g_pRenderText->RenderText(
        m_position.x + 140,
        m_position.y + 274,
        position,
        120,
        0,
        RT3_SORT_CENTER);
}

void CNewUIServerMenu::RenderCommandRow(const CommandEntry& entry, int y, int rowIndex)
{
    RenderUiPanel(
        m_position.x + CommandRowX,
        m_position.y + y,
        CommandRowWidth,
        CommandRowHeight,
        RowBorder,
        rowIndex % 2 == 0 ? RowDark : RowLight);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(246, 209, 73, 255);
    g_pRenderText->RenderText(m_position.x + 34, m_position.y + y + 8, entry.command.c_str(), 128, 0, RT3_SORT_LEFT);
    g_pRenderText->SetTextColor(225, 225, 225, 255);
    g_pRenderText->RenderText(m_position.x + 164, m_position.y + y + 8, entry.name.c_str(), 188, 0, RT3_SORT_RIGHT);

    wchar_t lines[2][MAX_TEXT_LENGTH]{};
    const int lineCount = DivideStringByPixel(
        &lines[0][0],
        2,
        MAX_TEXT_LENGTH,
        entry.description.c_str(),
        CommandRowWidth - 20,
        true,
        '#');
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(185, 200, 212, 255);
    for (int line = 0; line < lineCount; ++line)
    {
        g_pRenderText->RenderText(
            m_position.x + 34,
            m_position.y + y + 26 + (line * 12),
            lines[line],
            CommandRowWidth - 20,
            0,
            RT3_SORT_LEFT);
    }
}

void CNewUIServerMenu::RenderCommandScrollBar()
{
    RenderUiPanel(
        m_position.x + CommandScrollTrackX,
        m_position.y + CommandScrollTrackY,
        CommandScrollTrackWidth,
        CommandScrollTrackHeight,
        ContentBorder,
        RowDark);

    int thumbY = 0;
    int thumbHeight = 0;
    GetCommandScrollThumb(thumbY, thumbHeight);

    RenderUiRectangle(
        m_position.x + CommandScrollTrackX + 1,
        m_position.y + CommandScrollTrackY + thumbY + 1,
        CommandScrollTrackWidth - 2,
        thumbHeight - 2,
        Gold);
}

void CNewUIServerMenu::RenderButtons()
{
    for (int i = 0; i < m_actionCount; ++i)
    {
        const bool selected = m_page == Page::Rankings && i == static_cast<int>(m_rankingFilter);
        RenderButton(m_actionButtons[i], GetActionCaption(i), selected);
    }

    RenderButton(
        m_backButton,
        m_page == Page::Home ? I18N::Game::Close388 : I18N::Game::Back);
    RenderButton(m_closeButton, L"X");
}

void CNewUIServerMenu::RenderButton(CNewUIButton& button, const wchar_t* caption, bool selected)
{
    const POINT& position = button.GetPos();
    const POINT& size = button.GetSize();
    const BUTTON_STATE state = button.GetBTState();
    const UiColor& border = selected ? Gold : ButtonBorder;
    const UiColor& fill = state == BUTTON_STATE_DOWN
        ? ButtonPressed
        : (state == BUTTON_STATE_OVER ? ButtonHover : ButtonFill);

    RenderUiRectangle(position.x + 2, position.y + 3, size.x, size.y, Shadow);
    RenderUiPanel(position.x, position.y, size.x, size.y, border, fill);
    RenderUiRectangle(
        position.x + 6,
        position.y + 3,
        size.x - 12,
        1,
        selected || state == BUTTON_STATE_OVER ? Gold : ContentBorder);

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(selected ? 255 : 232, selected ? 215 : 232, selected ? 105 : 232, 255);
    const int textY = static_cast<int>(position.y) +
        std::max(3, (static_cast<int>(size.y) - 12) / 2) +
        (state == BUTTON_STATE_DOWN ? 1 : 0);
    g_pRenderText->RenderText(
        position.x,
        textY,
        caption,
        size.x,
        0,
        RT3_SORT_CENTER);
}

const wchar_t* CNewUIServerMenu::GetActionCaption(int index) const
{
    switch (m_page)
    {
    case Page::Home:
    {
        const wchar_t* captions[] = {
            I18N::Game::Events,
            I18N::Game::Rankings,
            I18N::Game::Reset,
            I18N::Game::OfflineLevel,
            I18N::Game::Commands,
            I18N::Game::Quests,
        };
        return captions[index];
    }
    case Page::Events:
        return I18N::Game::Refresh;
    case Page::Rankings:
    {
        const wchar_t* captions[] = {
            I18N::Game::Level,
            I18N::Game::Resets,
            I18N::Game::MasterLevel,
        };
        return captions[index];
    }
    case Page::Reset:
    {
        const wchar_t* captions[] = {
            I18N::Game::ResetCharacter,
            I18N::Game::ResetStats,
        };
        return captions[index];
    }
    case Page::OfflineLevel:
    {
        const wchar_t* captions[] = {
            I18N::Game::MUHelperSettings,
            I18N::Game::StartMUHelper,
            I18N::Game::ActivateOfflineLevel,
        };
        return captions[index];
    }
    case Page::Commands:
    {
        const wchar_t* captions[] = {
            I18N::Game::MoveMenu,
            I18N::Game::OpenWarehouse,
            I18N::Game::AvailableCommands,
        };
        return captions[index];
    }
    case Page::AvailableCommands:
        return L"";
    }

    return L"";
}

float CNewUIServerMenu::GetLayerDepth()
{
    return 5.1f;
}

float CNewUIServerMenu::GetKeyEventOrder()
{
    return 10.f;
}
