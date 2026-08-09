#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

namespace SEASON3B
{
    class CNewUIServerMenu : public CNewUIObj
    {
    public:
        enum class Page
        {
            Home,
            Events,
            Rankings,
            Reset,
            OfflineLevel,
            Commands,
            AvailableCommands,
        };

        enum class RankingFilter : std::uint8_t
        {
            Level,
            Resets,
            MasterLevel,
        };

        struct RankingEntry
        {
            std::wstring name;
            std::uint8_t characterClass = 0;
            std::uint16_t level = 0;
            std::uint16_t resets = 0;
            std::uint16_t masterLevel = 0;
        };

        struct ResetRequirements
        {
            std::uint8_t flags = 0;
            std::uint16_t currentLevel = 0;
            std::uint16_t requiredLevel = 0;
            std::uint16_t currentResets = 0;
            std::uint16_t nextReset = 0;
            std::uint16_t resetLimit = 0;
            std::uint32_t currentZen = 0;
            std::uint32_t requiredZen = 0;
            std::uint32_t rewardPoints = 0;
            std::uint16_t currentItems = 0;
            std::uint16_t requiredItems = 0;
            std::wstring itemName;
        };

        struct CommandEntry
        {
            std::wstring command;
            std::wstring name;
            std::wstring description;
        };

        CNewUIServerMenu();
        ~CNewUIServerMenu() override;

        bool Create(CNewUIManager* manager, int x, int y);
        void Release();

        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        bool Update() override;
        bool Render() override;

        float GetLayerDepth() override;
        float GetKeyEventOrder() override;

        void OpeningProcess();
        void ClosingProcess();

        void SetEventSchedule(
            std::uint8_t bloodCastleState,
            std::uint32_t bloodCastleSeconds,
            std::uint8_t devilSquareState,
            std::uint32_t devilSquareSeconds,
            std::uint8_t chaosCastleState,
            std::uint32_t chaosCastleSeconds);
        void SetRankingData(RankingFilter filter, std::vector<RankingEntry> entries);
        void SetResetRequirements(ResetRequirements requirements);
        void SetAvailableCommand(std::uint8_t index, std::uint8_t count, CommandEntry entry);

        static constexpr int WindowWidth = 400;
        static constexpr int WindowHeight = 350;

    private:
        static constexpr int MaxActions = 6;
        static constexpr int VisibleCommandRows = 3;

        struct EventScheduleEntry
        {
            std::uint8_t state = 0;
            std::uint32_t seconds = 0;
        };

        void SetPage(Page page);
        void ConfigureActionButton(int index, int x, int y, int width, const wchar_t* const* caption);
        void ConfigureBackButton();
        void ConfigureCloseButton();
        void ConfigureHomeButtons();
        void ConfigureEventButtons();
        void ConfigureRankingButtons();
        void ConfigureResetButtons();
        void ConfigureOfflineLevelButtons();
        void ConfigureCommandButtons();
        void ConfigureAvailableCommandButtons();
        void ProcessAction(int index);
        void GoBack();
        void ScrollCommands(int amount);
        void UpdateCommandScrollFromMouse();
        void GetCommandScrollThumb(int& thumbY, int& thumbHeight) const;
        bool ConfirmAction(int index);
        bool SendCommand(const wchar_t* command);
        bool RequestEventSchedule();
        bool RequestRanking(RankingFilter filter);
        bool RequestOpenWarehouse();
        bool RequestResetRequirements();
        bool RequestAvailableCommands();
        std::uint32_t GetRemainingSeconds(const EventScheduleEntry& entry) const;

        void RenderFrame();
        void RenderTexts();
        void RenderButtons();
        void RenderButton(CNewUIButton& button, const wchar_t* caption, bool selected = false);
        void RenderCommandScrollBar();
        void RenderEventScheduleRow(int index, const wchar_t* name, int y);
        void RenderRankingTable();
        void RenderResetRequirements();
        void RenderAvailableCommands();
        void RenderCommandRow(const CommandEntry& entry, int y, int rowIndex);
        void RenderRequirementRow(const wchar_t* label, const wchar_t* value, int y, bool met);
        void RenderWrappedText(const wchar_t* text, int y, DWORD color, int maxLines = 3);
        const wchar_t* GetActionCaption(int index) const;
        const wchar_t* GetTitle() const;
        const wchar_t* GetRankingValueTitle() const;

        CNewUIManager* m_manager;
        POINT m_position;
        Page m_page;
        int m_actionCount;
        int m_pendingAction;
        bool m_scheduleLoaded;
        bool m_scheduleRequestPending;
        bool m_rankingLoaded;
        bool m_rankingRequestPending;
        bool m_resetRequirementsLoaded;
        bool m_resetRequirementsRequestPending;
        bool m_commandsLoaded;
        bool m_commandsRequestPending;
        RankingFilter m_rankingFilter;
        int m_commandScrollOffset;
        bool m_commandScrollDragging;
        int m_commandScrollDragOffset;
        std::chrono::steady_clock::time_point m_scheduleReceivedAt;
        std::chrono::steady_clock::time_point m_scheduleRequestedAt;
        std::chrono::steady_clock::time_point m_rankingRequestedAt;
        std::chrono::steady_clock::time_point m_resetRequirementsRequestedAt;
        std::chrono::steady_clock::time_point m_commandsRequestedAt;
        std::array<EventScheduleEntry, 3> m_eventSchedule;
        std::vector<RankingEntry> m_rankingEntries;
        ResetRequirements m_resetRequirements;
        std::vector<CommandEntry> m_commandEntries;
        std::vector<bool> m_commandEntriesReceived;
        std::array<CNewUIButton, MaxActions> m_actionButtons;
        CNewUIButton m_backButton;
        CNewUIButton m_closeButton;
    };
}
