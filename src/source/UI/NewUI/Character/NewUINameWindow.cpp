// NewUINameWindow.cpp: implementation of the CNewUINameWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UI/Chat/Chat.h"
#include "UI/NewUI/Character/NewUINameWindow.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/ZzzInventory.h"
#include "UI/Legacy/UIControls.h"
#include "GameLogic/Events/CSChaosCastle.h"
#include "GameLogic/Items/PersonalShopTitleImp.h"
#include "GameLogic/Events/MatchEvent.h"
#include "World/MapInfra/MapManager.h"
#include "Camera/CameraProjection.h"
#include "Camera/CameraState.h"
#include "Audio/DSPlaySound.h"
#include "Core/Utilities/UsefulDef.h"
#include "GameLogic/Quests/QuestMng.h"
#include "I18N/All.h"

// DevEditor forward declarations (must be at global scope)
#ifdef _EDITOR
extern "C" bool DevEditor_ShouldRenderItemLabels();
#endif

using namespace SEASON3B;

namespace
{
constexpr int GROUND_ITEM_LABEL_BUILD_BUDGET_PER_FRAME = 32;
constexpr int QUEST_TRACKER_WIDTH = 170;
constexpr int QUEST_TRACKER_DEFAULT_X = REFERENCE_WIDTH - QUEST_TRACKER_WIDTH - 6;
constexpr int QUEST_TRACKER_DEFAULT_Y = 112;
constexpr int QUEST_TRACKER_HEADER_HEIGHT = 20;
constexpr int QUEST_TRACKER_FILTER_HEIGHT = 18;
constexpr int QUEST_TRACKER_ROW_HEIGHT = 38;
constexpr int QUEST_TRACKER_MAX_VISIBLE_QUESTS = 5;
constexpr DWORD QUEST_TRACKER_REFRESH_INTERVAL = 2500;
constexpr DWORD MONSTER_STATUS_REFRESH_INTERVAL = 750;

struct UiColor
{
    float red;
    float green;
    float blue;
    float alpha;
};

constexpr UiColor TrackerBorder{0.10f, 0.23f, 0.31f, 0.92f};
constexpr UiColor TrackerHeader{0.018f, 0.040f, 0.052f, 0.90f};
constexpr UiColor TrackerBody{0.006f, 0.014f, 0.020f, 0.76f};
constexpr UiColor TrackerRow{0.012f, 0.027f, 0.036f, 0.88f};
constexpr UiColor TrackerAccent{0.84f, 0.61f, 0.14f, 0.98f};

void DrawUiRectangle(float x, float y, float width, float height, const UiColor& color)
{
    glColor4f(color.red, color.green, color.blue, color.alpha);
    RenderColor(x, y, width, height);
    EndRenderColor();
}

void DrawUiPanel(float x, float y, float width, float height, const UiColor& border, const UiColor& fill)
{
    DrawUiRectangle(x, y, width, height, border);
    DrawUiRectangle(x + 1.f, y + 1.f, width - 2.f, height - 2.f, fill);
}

std::vector<DWORD> GetQuestIndices(bool mapOnly)
{
    auto questIndices = g_QuestMng.GetCurQuestIndexList();
    if (mapOnly)
    {
        questIndices.erase(
            std::remove_if(
                questIndices.begin(),
                questIndices.end(),
                [](DWORD questIndex) { return !g_QuestMng.IsQuestVisibleOnCurrentMap(questIndex); }),
            questIndices.end());
    }

    return questIndices;
}

int GetQuestTrackerHeight(bool expanded, int questCount)
{
    if (!expanded)
        return QUEST_TRACKER_HEADER_HEIGHT;

    const int visibleQuestCount = MIN(questCount, QUEST_TRACKER_MAX_VISIBLE_QUESTS);
    return QUEST_TRACKER_HEADER_HEIGHT + QUEST_TRACKER_FILTER_HEIGHT + (visibleQuestCount > 0
        ? visibleQuestCount * QUEST_TRACKER_ROW_HEIGHT + 8
        : 30);
}

// Draws a segmented monster HP bar, horizontally centered on centerX with its
// top edge at topY. `steps` is the segment count (HP granularity); `scale`
// horizontally compresses the bar (1.0 == original width).
void DrawHealthBar(int centerX, int topY, float health, int steps, float scale)
{
    const float borderHeight = 2.f;                  // vertical inset (unscaled)
    const float borderWidth = 2.f * scale;           // horizontal inset
    const float stepSeparatorWidth = 1.f * scale;    // gap between segments
    const float segmentSpan = 80.f * scale;          // total span of the segment track
    const float widthPerStep = segmentSpan / steps;  // derived: fewer steps -> wider segments
    const float stepsWidth = segmentSpan - 2.f * stepSeparatorWidth;
    const float totalWidth = stepsWidth + borderWidth * 2.f;

    const int x = centerX - (int)(totalWidth / 2);
    const int y = topY;

    // Drop shadow.
    EnableAlphaTest();
    glColor4f(0.f, 0.f, 0.f, 0.5f);
    RenderColor((float)(x + 1), (float)(y + 1), totalWidth, 5.f);

    // Dark backing.
    EnableAlphaBlend();
    glColor3f(0.2f, 0.0f, 0.0f);
    RenderColor((float)x, (float)y, totalWidth, 5.f);

    // Inner track.
    glColor3f(50.f / 255.f, 10.f / 255.f, 0.f);
    RenderColor((float)(x + borderWidth), (float)(y + borderHeight), stepsWidth, 1.f);

    // HealthStatus < 0 is the "HP unknown" sentinel (server sends 0xFF -> -1, and
    // the field is initialized to -1), so render a full bar instead of an empty one.
    const float clampedHealth = (health < 0.f) ? 1.f : health;
    const int stepHP = (int)(clampedHealth * steps);

    // Filled health segments.
    glColor3f(250.f / 255.f, 10.f / 255.f, 0.f);
    for (int k = 0; k < stepHP; ++k)
    {
        RenderColor(
            (float)(x + borderWidth + (k * widthPerStep)),
            (float)(y + borderHeight),
            widthPerStep - stepSeparatorWidth,
            2.f);
    }
    DisableAlphaBlend();
}

void DrawMonsterOverlay(int centerX, int topY, const CHARACTER& character)
{
    constexpr float width = 126.f;
    constexpr float healthBarWidth = 64.f;
    const float clampedHealth = character.HealthStatus < 0.f
        ? 1.f
        : std::clamp(character.HealthStatus, 0.f, 1.f);

    EnableAlphaTest();

    wchar_t monsterName[MAX_MONSTER_NAME + 1]{};
    ReduceStringByPixel(monsterName, MAX_MONSTER_NAME + 1, character.ID, static_cast<int>(width));
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(255, 220, 128, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->RenderText(centerX, topY + 1, monsterName, static_cast<int>(width), 0, RT3_WRITE_CENTER);

    wchar_t details[64]{};
    if (character.MonsterMaxHealth == 0)
        mu_swprintf(details, I18N::Game::MonsterLevelAndUnknownHealth, character.MonsterLevel);
    else
        mu_swprintf(
            details,
            I18N::Game::MonsterLevelAndHealth,
            character.MonsterLevel,
            character.MonsterHealth,
            character.MonsterMaxHealth);
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(225, 225, 225, 255);
    g_pRenderText->RenderText(centerX, topY + 11, details, static_cast<int>(width), 0, RT3_WRITE_CENTER);

    const float healthX = centerX - (healthBarWidth / 2.f);
    DrawUiRectangle(healthX, static_cast<float>(topY + 22), healthBarWidth, 2.f, {0.16f, 0.025f, 0.02f, 0.96f});
    if (clampedHealth > 0.f)
        DrawUiRectangle(healthX, static_cast<float>(topY + 22), healthBarWidth * clampedHealth, 2.f, {0.94f, 0.05f, 0.025f, 1.f});
    DisableAlphaBlend();
}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SEASON3B::CNewUINameWindow::CNewUINameWindow()
{
    m_pNewUIMng = NULL;
    m_Pos.x = m_Pos.y = 0;

    m_bShowItemName = false;
    m_bShowMonsterHealthBar = false;
    m_bQuestTrackerVisible = true;
    m_bQuestTrackerExpanded = true;
    m_bQuestTrackerMapOnly = false;
    m_bQuestTrackerDragging = false;
    m_QuestTrackerPosition = {QUEST_TRACKER_DEFAULT_X, QUEST_TRACKER_DEFAULT_Y};
    m_QuestTrackerDragOffset = {0, 0};
    m_iQuestTrackerScroll = 0;
    m_dwQuestTrackerLastRefresh = 0;
    m_dwMonsterStatusLastRefresh = 0;
    m_iQuestTrackerWorld = -1;
}

SEASON3B::CNewUINameWindow::~CNewUINameWindow()
{
    Release();
}

bool SEASON3B::CNewUINameWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (NULL == pNewUIMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_NAME_WINDOW, this);

    SetPos(x, y);

    Show(true);

    return true;
}

void SEASON3B::CNewUINameWindow::Release()
{
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

void SEASON3B::CNewUINameWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
}

void SEASON3B::CNewUINameWindow::ShowQuestTracker()
{
    m_bQuestTrackerVisible = true;
    m_bQuestTrackerDragging = false;
}

bool SEASON3B::CNewUINameWindow::UpdateMouseEvent()
{
    if (!m_bQuestTrackerVisible)
        return true;

    const auto questIndices = GetQuestIndices(m_bQuestTrackerMapOnly);
    const int trackerHeight = GetQuestTrackerHeight(
        m_bQuestTrackerExpanded,
        static_cast<int>(questIndices.size()));
    m_QuestTrackerPosition.x = std::clamp<int>(
        static_cast<int>(m_QuestTrackerPosition.x),
        0,
        REFERENCE_WIDTH - QUEST_TRACKER_WIDTH);
    m_QuestTrackerPosition.y = std::clamp<int>(
        static_cast<int>(m_QuestTrackerPosition.y),
        0,
        MAX(0, REFERENCE_HEIGHT - trackerHeight));
    const int trackerX = m_QuestTrackerPosition.x;
    const int trackerY = m_QuestTrackerPosition.y;

    if (m_bQuestTrackerDragging)
    {
        if (IsRelease(VK_LBUTTON))
        {
            m_bQuestTrackerDragging = false;
        }
        else
        {
            m_QuestTrackerPosition.x = std::clamp<int>(
                static_cast<int>(MouseX - m_QuestTrackerDragOffset.x),
                0,
                REFERENCE_WIDTH - QUEST_TRACKER_WIDTH);
            m_QuestTrackerPosition.y = std::clamp<int>(
                static_cast<int>(MouseY - m_QuestTrackerDragOffset.y),
                0,
                MAX(0, REFERENCE_HEIGHT - trackerHeight));
        }

        return false;
    }

    const int closeX = trackerX + QUEST_TRACKER_WIDTH - 20;
    if (CheckMouseIn(closeX, trackerY + 2, 17, 16) && IsPress(VK_LBUTTON))
    {
        m_bQuestTrackerVisible = false;
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    const int arrowX = trackerX + QUEST_TRACKER_WIDTH - 39;
    if (CheckMouseIn(arrowX, trackerY + 2, 17, 16) && IsPress(VK_LBUTTON))
    {
        m_bQuestTrackerExpanded = !m_bQuestTrackerExpanded;
        const int expandedHeight = GetQuestTrackerHeight(
            m_bQuestTrackerExpanded,
            static_cast<int>(questIndices.size()));
        m_QuestTrackerPosition.y = MIN(
            m_QuestTrackerPosition.y,
            MAX(0, REFERENCE_HEIGHT - expandedHeight));
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_bQuestTrackerExpanded)
    {
        const int filterY = trackerY + QUEST_TRACKER_HEADER_HEIGHT + 2;
        if (CheckMouseIn(trackerX + 5, filterY, 45, 14) && IsPress(VK_LBUTTON))
        {
            m_bQuestTrackerMapOnly = false;
            m_iQuestTrackerScroll = 0;
            PlayBuffer(SOUND_CLICK01);
            return false;
        }

        if (CheckMouseIn(trackerX + 52, filterY, 60, 14) && IsPress(VK_LBUTTON))
        {
            m_bQuestTrackerMapOnly = true;
            m_iQuestTrackerScroll = 0;
            RefreshQuestTracker(true);
            PlayBuffer(SOUND_CLICK01);
            return false;
        }

        if (CheckMouseIn(trackerX, trackerY, QUEST_TRACKER_WIDTH, trackerHeight) && MouseWheel != 0)
        {
            const int maximumScroll = MAX(
                0,
                static_cast<int>(questIndices.size()) - QUEST_TRACKER_MAX_VISIBLE_QUESTS);
            m_iQuestTrackerScroll = std::clamp(
                m_iQuestTrackerScroll + (MouseWheel > 0 ? -1 : 1),
                0,
                maximumScroll);
            MouseWheel = 0;
            return false;
        }
    }

    if (CheckMouseIn(trackerX, trackerY, QUEST_TRACKER_WIDTH - 40, QUEST_TRACKER_HEADER_HEIGHT)
        && IsPress(VK_LBUTTON))
    {
        m_bQuestTrackerDragging = true;
        m_QuestTrackerDragOffset = {MouseX - trackerX, MouseY - trackerY};
        return false;
    }

    if (CheckMouseIn(trackerX, trackerY, QUEST_TRACKER_WIDTH, trackerHeight))
        return false;

    return true;
}

bool SEASON3B::CNewUINameWindow::UpdateKeyEvent()
{
    if (SEASON3B::IsPress(VK_MENU) == true)
    {
        m_bShowItemName = !m_bShowItemName;
    }

    if (SEASON3B::IsPress(VK_F8) == true)
    {
        m_bShowMonsterHealthBar = !m_bShowMonsterHealthBar;
        m_dwMonsterStatusLastRefresh = 0;
    }

    if (SEASON3B::IsPress(VK_F7) == true
        && SEASON3B::IsPress(VK_SHIFT) == false
        && SEASON3B::IsRepeat(VK_SHIFT) == false)
    {
        m_bQuestTrackerVisible = !m_bQuestTrackerVisible;
        m_bQuestTrackerDragging = false;
        PlayBuffer(SOUND_CLICK01);
    }

    return true;
}

bool SEASON3B::CNewUINameWindow::Update()
{
    const DWORD now = GetTickCount();
    const bool worldChanged = m_iQuestTrackerWorld != gMapManager.WorldActive;
    if (worldChanged)
    {
        m_iQuestTrackerWorld = gMapManager.WorldActive;
        m_dwQuestTrackerLastRefresh = 0;
        g_QuestMng.ClearMapQuestIndexList();
    }

    if (m_dwQuestTrackerLastRefresh == 0 || now - m_dwQuestTrackerLastRefresh >= QUEST_TRACKER_REFRESH_INTERVAL)
    {
        RefreshQuestTracker(true);
        m_dwQuestTrackerLastRefresh = now;
    }

    if (m_bShowMonsterHealthBar &&
        (m_dwMonsterStatusLastRefresh == 0 || now - m_dwMonsterStatusLastRefresh >= MONSTER_STATUS_REFRESH_INTERVAL) &&
        SocketClient != nullptr && SocketClient->IsConnected())
    {
        const BYTE request[] = {0xC1, 0x04, 0xF5, 0x0B};
        SocketClient->Send(request, static_cast<int32_t>(sizeof(request)));
        m_dwMonsterStatusLastRefresh = now;
    }

    return true;
}

bool SEASON3B::CNewUINameWindow::Render()
{
    EnableAlphaTest();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    RenderName();
    RenderTimes();
    matchEvent::RenderMatchTimes();
    UI::Chat::RenderBooleans();
    RenderMonsterOverlays();
    RenderQuestTracker();
    DrawPersonalShopTitleImp();
    DisableAlphaBlend();
    return true;
}

void SEASON3B::CNewUINameWindow::RenderName()
{
    if (g_bGMObservation == true)
    {
        for (int i = 0; i < MAX_CHARACTERS_CLIENT; i++)
        {
            CHARACTER* c = &CharactersClient[i];
            OBJECT* o = &c->Object;
            if (o->Live && o->Kind == KIND_PLAYER)
            {
                if (IsShopTitleVisible(c) == false)
                {
                    UI::Chat::CreateChat(c->ID, L"", c);
                }
            }
        }
    }

#ifndef GUILD_WAR_EVENT
    if (gMapManager.InChaosCastle() == true && (SelectedNpc != -1 || SelectedCharacter != -1))
    {
        return;
    }
#endif//GUILD_WAR_EVENT

    if (SelectedItem != -1 || SelectedNpc != -1 || SelectedCharacter != -1)
    {
        if (SelectedNpc != -1)
        {
            CHARACTER* c = &CharactersClient[SelectedNpc];
            OBJECT* o = &c->Object;
            UI::Chat::CreateChat(c->ID, L"", c);
        }
        else if (SelectedCharacter != -1)
        {
            CHARACTER* c = &CharactersClient[SelectedCharacter];

            OBJECT* o = &c->Object;
            if (o->Kind == KIND_MONSTER)
            {
                g_pRenderText->SetTextColor(255, 230, 200, 255);
                g_pRenderText->SetBgColor(100, 0, 0, 255);
                g_pRenderText->RenderText(320, 2, c->ID, 0, 0, RT3_WRITE_CENTER);

                if (c->HealthStatus > 0)
                {
                    // Full-width bar centered under the selected monster's name.
                    DrawHealthBar(320, 15, c->HealthStatus, 20, 1.f);
                }
            }
            else
#ifdef ASG_ADD_GENS_SYSTEM
#ifndef PBG_MOD_STRIFE_GENSMARKRENDER
                if (!::IsStrifeMap(World) || Hero->m_byGensInfluence == c->m_byGensInfluence)
#endif //PBG_MOD_STRIFE_GENSMARKRENDER
#endif	// ASG_ADD_GENS_SYSTEM
                {
                    if (IsShopTitleVisible(c) == false)
                    {
                        UI::Chat::CreateChat(c->ID, L"", c);
                    }
                }
        }
        else if (SelectedItem != -1)
        {
#ifdef _EDITOR
            if (DevEditor_ShouldRenderItemLabels())
#endif
                RenderItemName(SelectedItem, &Items[SelectedItem].Object, &Items[SelectedItem].Item, false);
        }
    }

    if (m_bShowItemName || SEASON3B::IsRepeat(VK_MENU))
    {
        SetGroundItemLabelBuildBudget(GROUND_ITEM_LABEL_BUILD_BUDGET_PER_FRAME);

#ifdef _EDITOR
        bool renderLabels = DevEditor_ShouldRenderItemLabels();
#else
        bool renderLabels = true;
#endif

        if (renderLabels)
        {
            for (int i = 0; i < MAX_ITEMS; i++)
            {
                OBJECT* o = &Items[i].Object;
                if (o->Live)
                {
                    if (o->Visible && i != SelectedItem)
                    {
                        RenderItemName(i, o, &Items[i].Item, true);
                    }
                }
            }
        }
    }
}

void SEASON3B::CNewUINameWindow::RenderMonsterOverlays()
{
    if (!m_bShowMonsterHealthBar)
        return;

    for (int i = 0; i < MAX_CHARACTERS_CLIENT; i++)
    {
        CHARACTER* c = &CharactersClient[i];
        OBJECT* o = &c->Object;

        if (!o->Live || !o->Visible || o->Alpha <= 0.f || c->Dead > 0 || o->Kind != KIND_MONSTER)
            continue;

        vec3_t Position;
        Vector(o->Position[0], o->Position[1], o->Position[2] + o->BoundingBoxMax[2] + 60.f, Position);

        int ScreenX, ScreenY;
        vec3_t transformPos;
        VectorTransform(Position, g_Camera.Matrix, transformPos);
        if (transformPos[2] >= 0)
            continue;

        CameraProjection::WorldToScreen(g_Camera, Position, &ScreenX, &ScreenY);

        if (ScreenX < -100 || ScreenY < -100
            || ScreenX > (REFERENCE_WIDTH + 100)
            || ScreenY > (REFERENCE_HEIGHT + 100))
            continue;

        DrawMonsterOverlay(ScreenX, ScreenY - 13, *c);
    }
}

void SEASON3B::CNewUINameWindow::RefreshQuestTracker(bool refreshMapFilter)
{
    if (SocketClient == nullptr || !SocketClient->IsConnected() || SocketClient->ToGameServer() == nullptr)
        return;

    const auto questIndices = g_QuestMng.GetCurQuestIndexList();
    for (DWORD questIndex : questIndices)
    {
        SocketClient->ToGameServer()->SendQuestStateRequest(
            static_cast<uint16_t>(LOWORD(questIndex)),
            static_cast<uint16_t>(HIWORD(questIndex)));
    }

    if (refreshMapFilter)
    {
        const BYTE request[] = {0xC1, 0x04, 0xF5, 0x09};
        SocketClient->Send(request, static_cast<int32_t>(sizeof(request)));
    }
}

void SEASON3B::CNewUINameWindow::RenderQuestTracker()
{
    if (!m_bQuestTrackerVisible)
        return;

    const auto questIndices = GetQuestIndices(m_bQuestTrackerMapOnly);
    const int maximumScroll = MAX(
        0,
        static_cast<int>(questIndices.size()) - QUEST_TRACKER_MAX_VISIBLE_QUESTS);
    m_iQuestTrackerScroll = std::clamp(m_iQuestTrackerScroll, 0, maximumScroll);
    const int visibleQuestCount = MIN(
        static_cast<int>(questIndices.size()) - m_iQuestTrackerScroll,
        QUEST_TRACKER_MAX_VISIBLE_QUESTS);
    const int trackerHeight = GetQuestTrackerHeight(
        m_bQuestTrackerExpanded,
        static_cast<int>(questIndices.size()));
    const int trackerX = m_QuestTrackerPosition.x;
    const int trackerY = m_QuestTrackerPosition.y;

    EnableAlphaTest();
    DrawUiPanel(
        static_cast<float>(trackerX),
        static_cast<float>(trackerY),
        static_cast<float>(QUEST_TRACKER_WIDTH),
        static_cast<float>(trackerHeight),
        TrackerBorder,
        m_bQuestTrackerExpanded ? TrackerBody : TrackerHeader);
    DrawUiRectangle(
        static_cast<float>(trackerX + 1),
        static_cast<float>(trackerY + 1),
        static_cast<float>(QUEST_TRACKER_WIDTH - 2),
        static_cast<float>(QUEST_TRACKER_HEADER_HEIGHT - 2),
        TrackerHeader);
    DrawUiRectangle(
        static_cast<float>(trackerX + 7),
        static_cast<float>(trackerY + QUEST_TRACKER_HEADER_HEIGHT - 2),
        static_cast<float>(QUEST_TRACKER_WIDTH - 14),
        1.f,
        TrackerAccent);

    wchar_t header[96]{};
    if (m_bQuestTrackerMapOnly)
    {
        mu_swprintf(
            header,
            I18N::Game::QuestTrackerTitle,
            gMapManager.GetMapName(gMapManager.WorldActive));
    }
    else
    {
        mu_swprintf(header, L"%ls", I18N::Game::AllQuestsTitle);
    }

    wchar_t reducedHeader[96]{};
    ReduceStringByPixel(reducedHeader, 96, header, QUEST_TRACKER_WIDTH - 52);
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(246, 209, 73, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->RenderText(
        trackerX + 8,
        trackerY + 5,
        reducedHeader,
        QUEST_TRACKER_WIDTH - 52,
        0,
        RT3_SORT_LEFT);

    DrawUiPanel(
        static_cast<float>(trackerX + QUEST_TRACKER_WIDTH - 39),
        static_cast<float>(trackerY + 2),
        17.f,
        16.f,
        TrackerBorder,
        TrackerBody);
    DrawUiPanel(
        static_cast<float>(trackerX + QUEST_TRACKER_WIDTH - 20),
        static_cast<float>(trackerY + 2),
        17.f,
        16.f,
        TrackerBorder,
        TrackerBody);
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(235, 235, 235, 255);
    g_pRenderText->RenderText(
        trackerX + QUEST_TRACKER_WIDTH - 31,
        trackerY + 5,
        m_bQuestTrackerExpanded ? L"^" : L"v",
        12,
        0,
        RT3_WRITE_CENTER);
    g_pRenderText->RenderText(
        trackerX + QUEST_TRACKER_WIDTH - 12,
        trackerY + 5,
        L"x",
        12,
        0,
        RT3_WRITE_CENTER);

    if (!m_bQuestTrackerExpanded)
    {
        DisableAlphaBlend();
        return;
    }

    const int filterY = trackerY + QUEST_TRACKER_HEADER_HEIGHT + 2;
    DrawUiPanel(
        static_cast<float>(trackerX + 5),
        static_cast<float>(filterY),
        45.f,
        14.f,
        m_bQuestTrackerMapOnly ? TrackerBorder : TrackerAccent,
        m_bQuestTrackerMapOnly ? TrackerBody : TrackerRow);
    DrawUiPanel(
        static_cast<float>(trackerX + 52),
        static_cast<float>(filterY),
        60.f,
        14.f,
        m_bQuestTrackerMapOnly ? TrackerAccent : TrackerBorder,
        m_bQuestTrackerMapOnly ? TrackerRow : TrackerBody);
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(225, 225, 225, 255);
    g_pRenderText->RenderText(trackerX + 27, filterY + 3, I18N::Game::QuestTrackerAll, 41, 0, RT3_WRITE_CENTER);
    g_pRenderText->RenderText(trackerX + 82, filterY + 3, I18N::Game::QuestTrackerThisMap, 56, 0, RT3_WRITE_CENTER);

    if (questIndices.size() > QUEST_TRACKER_MAX_VISIBLE_QUESTS)
    {
        wchar_t range[32]{};
        mu_swprintf(
            range,
            L"%d-%d/%u",
            m_iQuestTrackerScroll + 1,
            m_iQuestTrackerScroll + visibleQuestCount,
            static_cast<unsigned>(questIndices.size()));
        g_pRenderText->SetTextColor(175, 185, 190, 255);
        g_pRenderText->RenderText(trackerX + 116, filterY + 3, range, 47, 0, RT3_SORT_RIGHT);
    }

    if (visibleQuestCount == 0)
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(190, 200, 205, 255);
        g_pRenderText->RenderText(
            trackerX + 7,
            trackerY + QUEST_TRACKER_HEADER_HEIGHT + QUEST_TRACKER_FILTER_HEIGHT + 9,
            m_bQuestTrackerMapOnly
                ? I18N::Game::NoActiveQuestsForThisMap
                : I18N::Game::NoActiveQuests,
            QUEST_TRACKER_WIDTH - 14,
            0,
            RT3_SORT_CENTER);
        DisableAlphaBlend();
        return;
    }

    for (int questNumber = 0; questNumber < visibleQuestCount; ++questNumber)
    {
        const DWORD questIndex = questIndices[m_iQuestTrackerScroll + questNumber];
        const int rowY = trackerY + QUEST_TRACKER_HEADER_HEIGHT + QUEST_TRACKER_FILTER_HEIGHT + 3 +
            (questNumber * QUEST_TRACKER_ROW_HEIGHT);
        DrawUiPanel(
            static_cast<float>(trackerX + 5),
            static_cast<float>(rowY),
            static_cast<float>(QUEST_TRACKER_WIDTH - 10),
            static_cast<float>(QUEST_TRACKER_ROW_HEIGHT - 4),
            TrackerBorder,
            TrackerRow);
        DrawUiRectangle(
            static_cast<float>(trackerX + 6),
            static_cast<float>(rowY + 1),
            2.f,
            static_cast<float>(QUEST_TRACKER_ROW_HEIGHT - 6),
            TrackerAccent);

        const wchar_t* subject = g_QuestMng.GetSubject(questIndex);
        wchar_t reducedSubject[96]{};
        ReduceStringByPixel(
            reducedSubject,
            96,
            subject != nullptr ? subject : L"Quest",
            QUEST_TRACKER_WIDTH - 24);
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(236, 205, 111, 255);
        g_pRenderText->RenderText(
            trackerX + 12,
            rowY + 4,
            reducedSubject,
            QUEST_TRACKER_WIDTH - 21,
            0,
            RT3_SORT_LEFT);

        SRequestRewardText requestLines[16]{};
        int renderedRequests = 0;
        if (g_QuestMng.GetRequestReward(questIndex) != nullptr)
        {
            g_QuestMng.GetRequestRewardText(requestLines, 16, questIndex);
            for (const auto& requestLine : requestLines)
            {
                if (requestLine.m_eRequestReward != RRC_REQUEST || requestLine.m_szText[0] == L'\0')
                    continue;

                wchar_t reducedRequest[96]{};
                ReduceStringByPixel(reducedRequest, 96, requestLine.m_szText, QUEST_TRACKER_WIDTH - 25);
                g_pRenderText->SetFont(g_hFont);
                g_pRenderText->SetTextColor(requestLine.m_dwColor);
                g_pRenderText->RenderText(
                    trackerX + 13,
                    rowY + 15 + (renderedRequests * 10),
                    reducedRequest,
                    QUEST_TRACKER_WIDTH - 22,
                    0,
                    RT3_SORT_LEFT);
                if (++renderedRequests == 2)
                    break;
            }
        }

        if (renderedRequests == 0)
        {
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(185, 195, 200, 255);
            g_pRenderText->RenderText(
                trackerX + 13,
                rowY + 17,
                I18N::Game::LoadingQuestObjective,
                QUEST_TRACKER_WIDTH - 22,
                0,
                RT3_SORT_LEFT);
        }
    }

    DisableAlphaBlend();
}

float SEASON3B::CNewUINameWindow::GetLayerDepth()
{
    return 1.0f;
}
