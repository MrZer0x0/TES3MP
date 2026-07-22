#ifndef MWGUI_QUICKLOOT_H
#define MWGUI_QUICKLOOT_H

#include <array>

#include "layout.hpp"
#include "../mwworld/ptr.hpp"

#include "widgets.hpp"
#include "itemmodel.hpp"
#include "sortfilteritemmodel.hpp"

namespace MyGUI
{
    class TextBox;
    class Widget;
}

namespace MWGui
{
    class QuickLoot : public Layout
    {
    public:
        QuickLoot();
        ~QuickLoot() override;

        void onFrame(float frameDuration);
        void update(float frameDuration);

        void setEnabled(bool enabled);
        void setDelay(float delay);

        bool isVisible() const { return mMainWidget->getVisible() && mQuickLoot->getVisible(); }
        bool isPlaying() const { return mPlaying; }
        void setPlaying(bool playing) { mPlaying = playing; }

        /// Select exactly one previous/next QuickLoot row and consume the wheel event.
        bool handleMouseWheel(int rel);

        /// Activate the selected row. Row 0 opens the regular container window.
        bool activateSelected();

        void clear();

        void setFocusObject(const MWWorld::Ptr& focus);
        void setFocusObjectScreenCoords(float min_x, float min_y, float max_x, float max_y);
        ///< set the screen-space position of the tooltip for focused object

        bool checkOwned();

        void resize();
        void ensureTrapTriggered();

    private:
        static constexpr int sVisibleRows = 6;

        void playOpenAnimation();
        void playCloseAnimation() const;
        void setVisibleAll(bool visible);

        void clearModels();
        int getEntryCount() const;
        void refreshRows();
        void openStandardContainer();

        void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character);
        void onItemSelected(int index);

        MyGUI::Widget* mQuickLoot;
        ItemModel* mModel;
        MyGUI::TextBox* mLabel;
        std::array<MyGUI::TextBox*, sVisibleRows> mRows;
        SortFilterItemModel* mSortModel;

        /// has the current container been "opened"
        bool mOpened;
        bool mShouldOpen;

        bool mHidden;
        bool mPlaying;

        MWWorld::Ptr mFocusObject;
        MWWorld::Ptr mLastFocusObject;

        float mFocusToolTipX;
        float mFocusToolTipY;

        /// Adjust position for a tooltip so that it doesn't leave the screen and does not obscure the mouse cursor
        void position(MyGUI::IntPoint& position, MyGUI::IntSize size, MyGUI::IntSize viewportSize);

        float mDelay;
        float mRemainingDelay; // remaining time until tooltip will show

        int mLastMouseX;
        int mLastMouseY;

        bool mEnabled;
        float mFrameDuration;

        /// Global row: 0 is "open regular inventory", item rows begin at 1.
        int mLastIndex;
        int mVisibleStart;
    };
}
#endif
