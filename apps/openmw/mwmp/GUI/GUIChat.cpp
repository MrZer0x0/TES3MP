#include "GUIChat.hpp"

#include <MyGUI_EditBox.h>
#include <MyGUI_ScrollBar.h>
#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwgui/windowmanagerimp.hpp"
#include "apps/openmw/mwinput/inputmanagerimp.hpp"
#include <MyGUI_InputManager.h>
#include <components/openmw-mp/TimedLog.hpp>

#include "../Networking.hpp"
#include "../Main.hpp"
#include "../LocalPlayer.hpp"
#include "../tts/ChatTtsManager.hpp"

#include "../GUIController.hpp"


namespace mwmp
{
    GUIChat::GUIChat(int x, int y, int w, int h)
            : WindowBase("tes3mp_chat.layout")
            , mHistoryScroll(nullptr)
            , windowState(CHAT_DISABLED)
            , editState(false)
            , historyReviewState(false)
            , mainMenuOpen(false)
            , visibleBeforeMainMenu(false)
            , historyDisplayEnabled(true)
            , delay(3.f)
            , curTime(0.f)
    {
        setCoord(x, y, w, h);

        getWidget(mCommandLine, "edit_Command");
        getWidget(mHistory, "list_History");

        for (size_t i = 0; i < mHistory->getChildCount(); ++i)
        {
            if (MyGUI::ScrollBar* scroll = mHistory->getChildAt(i)->castType<MyGUI::ScrollBar>(false))
            {
                mHistoryScroll = scroll;
                break;
            }
        }
        if (mHistoryScroll)
        {
            mHistoryScroll->setVisible(false);
            mHistoryScroll->setNeedMouseFocus(false);
        }

        // Set up the command line box
        mCommandLine->eventEditSelectAccept +=
                newDelegate(this, &GUIChat::acceptCommand);
        mCommandLine->eventKeyButtonPressed +=
                newDelegate(this, &GUIChat::keyPress);

        setTitle("Chat");

        mHistory->setOverflowToTheLeft(false);
        mHistory->setEditWordWrap(true);
        mHistory->setEditReadOnly(true);
        mHistory->setTextShadow(true);
        mHistory->setTextShadowColour(MyGUI::Colour::Black);

        mHistory->setNeedKeyFocus(false);

        mCommandLine->setVisible(false);
    }

    void GUIChat::onOpen()
    {
        // Give keyboard focus to the combo box whenever the console is
        // turned on
        setEditState(false);

        if (windowState == CHAT_DISABLED)
            windowState = CHAT_ENABLED;
    }

    void GUIChat::onClose()
    {
        setEditState(false);
        setHistoryReviewState(false);
    }

    bool GUIChat::exit()
    {
        //WindowBase::exit();
        return true;
    }

    bool GUIChat::getEditState()
    {
        return editState;
    }

    void GUIChat::acceptCommand(MyGUI::EditBox *_sender)
    {
        const std::string &cm = mCommandLine->getOnlyText();

        // If they enter nothing, then it should be canceled.
        // Otherwise, there's no way of closing without having text.
        if (cm.empty())
        {
            mCommandLine->setCaption("");
            setEditState(false);
            return;
        }

        LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Player: %s", cm.c_str());

        // Add the command to the history, and set the current pointer to
        // the end of the list
        if (mCommandHistory.empty() || mCommandHistory.back() != cm)
            mCommandHistory.push_back(cm);
        mCurrent = mCommandHistory.end();
        mEditString.clear();

        // Reset the command line before the command execution.
        // It prevents the re-triggering of the acceptCommand() event for the same command
        // during the actual command execution
        mCommandLine->setCaption("");
        setEditState(false);
        send(cm);
    }

    void GUIChat::onResChange(int width, int height)
    {
        setCoord(10, 40, width-10, height/2); // Original chat layout, shifted 30 px down.
    }

    void GUIChat::setFont(const std::string &fntName)
    {
        mHistory->setFontName(fntName);
        mCommandLine->setFontName(fntName);
    }

    void GUIChat::print(const std::string &msg, const std::string &color)
    {
        if (historyDisplayEnabled && windowState == CHAT_HIDDENMODE && !mainMenuOpen && !isVisible())
            setVisible(true);

        if(msg.size() == 0)
        {
            clean();
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Chat cleaned");
        }
        else
        {
            mHistory->addText(color + msg);
            if (!historyReviewState)
                scrollHistoryToBottom();
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "%s", msg.c_str());
        }
    }

    void GUIChat::printOK(const std::string &msg)
    {
        print(msg + "\n", "#FF00FF");
    }

    void GUIChat::printError(const std::string &msg)
    {
        print(msg + "\n", "#FF2222");
    }

    void GUIChat::send(const std::string &str)
    {
        LocalPlayer *localPlayer = Main::get().getLocalPlayer();

        Networking *networking = Main::get().getNetworking();

        localPlayer->chatMessage = str;

        // Queue the local character immediately. Do not wait for the server echo:
        // system messages and player messages share ID_CHAT_MESSAGE in TES3MP.
        if (Main::get().getChatTtsManager())
            Main::get().getChatTtsManager()->enqueueLocal(*localPlayer, str);

        networking->getPlayerPacket(ID_CHAT_MESSAGE)->setPlayer(localPlayer);
        networking->getPlayerPacket(ID_CHAT_MESSAGE)->Send();
    }

    void GUIChat::clean()
    {
        mHistory->setCaption("");
        scrollHistoryToBottom();
    }

    void GUIChat::pressedChatMode()
    {
        if (!historyDisplayEnabled)
        {
            MWBase::Environment::get().getWindowManager()->messageBox(
                "Chat messages are hidden. The chat input remains available.");
            return;
        }

        windowState++;
        if (windowState == 3) windowState = 0;

        std::string chatMode = windowState == CHAT_DISABLED ? "Chat hidden" :
                               windowState == CHAT_ENABLED ? "Chat visible" :
                               "Chat appearing when needed";

        LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Switch chat mode to %s", chatMode.c_str());
        MWBase::Environment::get().getWindowManager()->messageBox(chatMode);

        switch (windowState)
        {
            case CHAT_DISABLED:
                setHistoryReviewState(false);
                setVisible(false);
                setEditState(false);
                break;
            case CHAT_ENABLED:
                setVisible(true);
                break;
            default: //CHAT_HIDDENMODE
                setVisible(true);
                curTime = 0;
        }
    }

    void GUIChat::setEditState(bool state)
    {
        if (state && historyReviewState)
            setHistoryReviewState(false);

        editState = state;
        mCommandLine->setVisible(editState);
        if (!historyDisplayEnabled)
            setVisible(editState && !mainMenuOpen);
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(editState ? mCommandLine : nullptr);
    }

    void GUIChat::scrollHistoryToBottom()
    {
        const size_t range = mHistory->getVScrollRange();
        if (range > 0)
            mHistory->setVScrollPosition(range - 1);
        mHistory->setTextCursor(mHistory->getCaption().size());
    }

    void GUIChat::setHistoryReviewState(bool state)
    {
        if (historyReviewState == state)
            return;

        historyReviewState = state;
        if (state)
        {
            editState = false;
            mCommandLine->setVisible(false);
            mMainWidget->setNeedMouseFocus(true);
            mHistory->setNeedMouseFocus(true);
            mHistory->setNeedKeyFocus(true);
            if (mHistoryScroll)
            {
                mHistoryScroll->setVisible(true);
                mHistoryScroll->setNeedMouseFocus(true);
            }
            MWBase::Environment::get().getInputManager()->changeInputMode(true);
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mHistory);
        }
        else
        {
            mMainWidget->setNeedMouseFocus(false);
            mHistory->setNeedMouseFocus(false);
            mHistory->setNeedKeyFocus(false);
            if (mHistoryScroll)
            {
                mHistoryScroll->setVisible(false);
                mHistoryScroll->setNeedMouseFocus(false);
            }
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(nullptr);
            if (!mainMenuOpen)
                MWBase::Environment::get().getInputManager()->changeInputMode(false);
            scrollHistoryToBottom();
            if (windowState == CHAT_HIDDENMODE)
                curTime = 0.f;
        }
    }

    std::string GUIChat::getHistoryText() const
    {
        return mHistory->getCaption().asUTF8();
    }

    void GUIChat::setMainMenuOpen(bool state)
    {
        if (mainMenuOpen == state)
            return;

        mainMenuOpen = state;
        if (state)
        {
            visibleBeforeMainMenu = isVisible();

            // The pause menu already owns keyboard/mouse focus. Close the live
            // chat controls without clearing the focus assigned to menu buttons.
            editState = false;
            mCommandLine->setVisible(false);
            historyReviewState = false;
            mMainWidget->setNeedMouseFocus(false);
            mHistory->setNeedMouseFocus(false);
            mHistory->setNeedKeyFocus(false);
            if (mHistoryScroll)
            {
                mHistoryScroll->setVisible(false);
                mHistoryScroll->setNeedMouseFocus(false);
            }
            setVisible(false);
        }
        else
        {
            if (!historyDisplayEnabled)
                setVisible(editState);
            else if (windowState == CHAT_ENABLED || (windowState == CHAT_HIDDENMODE && visibleBeforeMainMenu))
                setVisible(true);
            else
                setVisible(false);

            if (windowState == CHAT_HIDDENMODE)
                curTime = 0.f;
        }
    }

    void GUIChat::pressedSay()
    {
        if (windowState == CHAT_DISABLED && historyDisplayEnabled)
            return;

        if (!mCommandLine->getVisible())
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Opening chat.");

        if (!historyDisplayEnabled || windowState == CHAT_HIDDENMODE)
        {
            setVisible(true);
            curTime = 0;
        }

        setEditState(true);
    }

    void GUIChat::keyPress(MyGUI::Widget *_sender, MyGUI::KeyCode key, MyGUI::Char _char)
    {
        if (mCommandHistory.empty()) return;

        // Traverse history with up and down arrows
        if (key == MyGUI::KeyCode::ArrowUp)
        {
            // If the user was editing a string, store it for later
            if (mCurrent == mCommandHistory.end())
                mEditString = mCommandLine->getOnlyText();

            if (mCurrent != mCommandHistory.begin())
            {
                --mCurrent;
                mCommandLine->setCaption(*mCurrent);
            }
        }
        else if (key == MyGUI::KeyCode::ArrowDown)
        {
            if (mCurrent != mCommandHistory.end())
            {
                ++mCurrent;

                if (mCurrent != mCommandHistory.end())
                    mCommandLine->setCaption(*mCurrent);
                else
                    // Restore the edit string
                    mCommandLine->setCaption(mEditString);
            }
        }

    }

    void GUIChat::update(float dt)
    {
        if (!mainMenuOpen && windowState == CHAT_HIDDENMODE && !editState && !historyReviewState && isVisible())
        {
            curTime += dt;
            if (curTime >= delay)
            {
                setEditState(false);
                setVisible(false);
            }
        }
    }


    void GUIChat::setHistoryDisplayEnabled(bool enabled)
    {
        historyDisplayEnabled = enabled;
        mHistory->setVisible(enabled);

        if (!enabled)
        {
            // Keep received messages in memory for logging/history, but never draw them.
            // Chat input remains available through the normal Say key.
            windowState = CHAT_ENABLED;
            setHistoryReviewState(false);
            if (!editState)
                setVisible(false);
        }
        else if (!mainMenuOpen && windowState == CHAT_ENABLED)
            setVisible(true);
    }

    void GUIChat::setDelay(float newDelay)
    {
        this->delay = newDelay;
    }
}
