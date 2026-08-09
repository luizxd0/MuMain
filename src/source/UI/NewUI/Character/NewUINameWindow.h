// NewUINameWindow.h: interface for the CNewUINameWindow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NEWUINAMEWINDOW_H__76B140FF_46CB_4DB6_9DA2_5F84F294D212__INCLUDED_)
#define AFX_NEWUINAMEWINDOW_H__76B140FF_46CB_4DB6_9DA2_5F84F294D212__INCLUDED_

#pragma once

#include "UI/NewUI/NewUIManager.h"

namespace SEASON3B
{
    // item name
    class CNewUINameWindow : public CNewUIObj
    {
    public:
        CNewUINameWindow();
        virtual ~CNewUINameWindow();

        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();

        void SetPos(int x, int y);
        void ShowQuestTracker();

        bool UpdateMouseEvent();
        bool UpdateKeyEvent();
        bool Update();
        bool Render();

        float GetLayerDepth();		// 1.0f

    private:
        void RenderName();

        CNewUIManager* m_pNewUIMng;		// UI manager
        POINT m_Pos;					// window position

        bool m_bShowItemName;
        bool m_bShowMonsterHealthBar;
        bool m_bQuestTrackerVisible;
        bool m_bQuestTrackerExpanded;
        bool m_bQuestTrackerMapOnly;
        bool m_bQuestTrackerDragging;
        POINT m_QuestTrackerPosition;
        POINT m_QuestTrackerDragOffset;
        int m_iQuestTrackerScroll;
        DWORD m_dwQuestTrackerLastRefresh;
        DWORD m_dwMonsterStatusLastRefresh;
        int m_iQuestTrackerWorld;

        void RenderMonsterOverlays();
        void RenderQuestTracker();
        void RefreshQuestTracker(bool refreshMapFilter);
    };
}

#endif // !defined(AFX_NEWUINAMEWINDOW_H__76B140FF_46CB_4DB6_9DA2_5F84F294D212__INCLUDED_)
