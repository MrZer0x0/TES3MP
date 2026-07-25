#include "dialogue.hpp"

#include <MyGUI_LanguageManager.h>
#include <MyGUI_Window.h>
#include <MyGUI_ProgressBar.h>
#include <MyGUI_ScrollBar.h>
#include <MyGUI_Button.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_InputManager.h>

#include <algorithm>
#include <cmath>

#include <components/debug/debuglog.hpp>
#include <components/widgets/list.hpp>
#include <components/translation/translation.hpp>

/*
    Start of tes3mp addition

    Include additional headers for multiplayer purposes
*/
#include "../mwmp/Main.hpp"
#include "../mwmp/Networking.hpp"
#include "../mwmp/ObjectList.hpp"
#include <components/openmw-mp/TimedLog.hpp>
/*
    End of tes3mp addition
*/

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "bookpage.hpp"
#include "textcolours.hpp"

#include "journalbooks.hpp" // to_utf8_span

namespace MWGui
{

    class ResponseCallback : public MWBase::DialogueManager::ResponseCallback
    {
    public:
        ResponseCallback(DialogueWindow* win, bool needMargin=true)
            : mWindow(win)
            , mNeedMargin(needMargin)
        {

        }

        void addResponse(const std::string& title, const std::string& text) override
        {
            mWindow->addResponse(title, text, mNeedMargin);
        }

        void updateTopics()
        {
            mWindow->updateTopics();
        }

    private:
        DialogueWindow* mWindow;
        bool mNeedMargin;
    };

    PersuasionDialog::PersuasionDialog(ResponseCallback* callback)
        : WindowModal("openmw_persuasion_dialog.layout")
        , mCallback(callback)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mAdmireButton, "AdmireButton");
        getWidget(mIntimidateButton, "IntimidateButton");
        getWidget(mTauntButton, "TauntButton");
        getWidget(mBribe10Button, "Bribe10Button");
        getWidget(mBribe100Button, "Bribe100Button");
        getWidget(mBribe1000Button, "Bribe1000Button");
        getWidget(mGoldLabel, "GoldLabel");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onCancel);
        mAdmireButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mIntimidateButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mTauntButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mBribe10Button->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mBribe100Button->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mBribe1000Button->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
    }

    void PersuasionDialog::onCancel(MyGUI::Widget *sender)
    {
        setVisible(false);
    }

    void PersuasionDialog::onPersuade(MyGUI::Widget *sender)
    {
        MWBase::MechanicsManager::PersuasionType type;
        if (sender == mAdmireButton) type = MWBase::MechanicsManager::PT_Admire;
        else if (sender == mIntimidateButton) type = MWBase::MechanicsManager::PT_Intimidate;
        else if (sender == mTauntButton) type = MWBase::MechanicsManager::PT_Taunt;
        else if (sender == mBribe10Button)
            type = MWBase::MechanicsManager::PT_Bribe10;
        else if (sender == mBribe100Button)
            type = MWBase::MechanicsManager::PT_Bribe100;
        else /*if (sender == mBribe1000Button)*/
            type = MWBase::MechanicsManager::PT_Bribe1000;

        MWBase::Environment::get().getDialogueManager()->persuade(type, mCallback.get());
        mCallback->updateTopics();

        setVisible(false);
    }

    void PersuasionDialog::onOpen()
    {
        center();

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mBribe10Button->setEnabled (playerGold >= 10);
        mBribe100Button->setEnabled (playerGold >= 100);
        mBribe1000Button->setEnabled (playerGold >= 1000);

        mGoldLabel->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));
        WindowModal::onOpen();
    }

    MyGUI::Widget* PersuasionDialog::getDefaultKeyFocus()
    {
        return mAdmireButton;
    }

    // --------------------------------------------------------------------------------------------------

    Response::Response(const std::string &text, const std::string &title, bool needMargin)
        : mTitle(title), mNeedMargin(needMargin)
    {
        mText = text;
    }

    void Response::write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks) const
    {
        typesetter->sectionBreak(mNeedMargin ? 9 : 0);

        if (mTitle != "")
        {
            const MyGUI::Colour& headerColour = MWBase::Environment::get().getWindowManager()->getTextColours().header;
            BookTypesetter::Style* title = typesetter->createStyle("", headerColour, false);
            typesetter->write(title, to_utf8_span(mTitle.c_str()));
            typesetter->sectionBreak();
        }

        typedef std::pair<size_t, size_t> Range;
        std::map<Range, intptr_t> hyperLinks;

        // We need this copy for when @# hyperlinks are replaced
        std::string text = mText;

        size_t pos_end = std::string::npos;
        for(;;)
        {
            size_t pos_begin = text.find('@');
            if (pos_begin != std::string::npos)
                pos_end = text.find('#', pos_begin);

            if (pos_begin != std::string::npos && pos_end != std::string::npos)
            {
                std::string link = text.substr(pos_begin + 1, pos_end - pos_begin - 1);
                const char specialPseudoAsteriskCharacter = 127;
                std::replace(link.begin(), link.end(), specialPseudoAsteriskCharacter, '*');
                std::string topicName = MWBase::Environment::get().getWindowManager()->
                        getTranslationDataStorage().topicStandardForm(link);

                std::string displayName = link;
                while (displayName[displayName.size()-1] == '*')
                    displayName.erase(displayName.size()-1, 1);

                text.replace(pos_begin, pos_end+1-pos_begin, displayName);

                if (topicLinks.find(Misc::StringUtils::lowerCase(topicName)) != topicLinks.end())
                    hyperLinks[std::make_pair(pos_begin, pos_begin+displayName.size())] = intptr_t(topicLinks[Misc::StringUtils::lowerCase(topicName)]);
            }
            else
                break;
        }

        typesetter->addContent(to_utf8_span(text.c_str()));

        if (hyperLinks.size() && MWBase::Environment::get().getWindowManager()->getTranslationDataStorage().hasTranslation())
        {
            const TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();

            BookTypesetter::Style* style = typesetter->createStyle("", textColours.normal, false);
            size_t formatted = 0; // points to the first character that is not laid out yet
            for (auto& hyperLink : hyperLinks)
            {
                intptr_t topicId = hyperLink.second;
                BookTypesetter::Style* hotStyle = typesetter->createHotStyle (style, textColours.link,
                                                                              textColours.linkOver, textColours.linkPressed,
                                                                              topicId);
                if (formatted < hyperLink.first.first)
                    typesetter->write(style, formatted, hyperLink.first.first);
                typesetter->write(hotStyle, hyperLink.first.first, hyperLink.first.second);
                formatted = hyperLink.first.second;
            }
            if (formatted < text.size())
                typesetter->write(style, formatted, text.size());
        }
        else
        {
            std::vector<KeywordSearchT::Match> matches;
            keywordSearch->highlightKeywords(text.begin(), text.end(), matches);

            std::string::const_iterator i = text.begin ();
            for (KeywordSearchT::Match& match : matches)
            {
                if (i != match.mBeg)
                    addTopicLink (typesetter, 0, i - text.begin (), match.mBeg - text.begin ());

                addTopicLink (typesetter, match.mValue, match.mBeg - text.begin (), match.mEnd - text.begin ());

                i = match.mEnd;
            }
            if (i != text.end ())
                addTopicLink (typesetter, 0, i - text.begin (), text.size ());
        }
    }

    void Response::addTopicLink(BookTypesetter::Ptr typesetter, intptr_t topicId, size_t begin, size_t end) const
    {
        const TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();

        BookTypesetter::Style* style = typesetter->createStyle("", textColours.normal, false);


        if (topicId)
            style = typesetter->createHotStyle (style, textColours.link, textColours.linkOver, textColours.linkPressed, topicId);
        typesetter->write (style, begin, end);
    }

    Message::Message(const std::string& text)
    {
        mText = text;
    }

    void Message::write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks) const
    {
        const MyGUI::Colour& textColour = MWBase::Environment::get().getWindowManager()->getTextColours().notify;
        BookTypesetter::Style* title = typesetter->createStyle("", textColour, false);
        typesetter->sectionBreak(9);
        typesetter->write(title, to_utf8_span(mText.c_str()));
    }

    // --------------------------------------------------------------------------------------------------

    void Choice::activated()
    {
        MWBase::Environment::get().getWindowManager()->playSound("Menu Click");
        eventChoiceActivated(mChoiceId);
    }

    void Topic::activated()
    {
        MWBase::Environment::get().getWindowManager()->playSound("Menu Click");
        eventTopicActivated(mTopicId);
    }

    void Goodbye::activated()
    {
        MWBase::Environment::get().getWindowManager()->playSound("Menu Click");
        eventActivated();
    }

    // --------------------------------------------------------------------------------------------------

    DialogueWindow::DialogueWindow()
        : WindowBase("openmw_dialogue_window.layout")
        , mIsCompanion(false)
        , mGoodbye(false)
        , mPersuasionDialog(new ResponseCallback(this))
        , mHistoryWasDragged(false)
        , mDialogueCameraActive(false)
        , mNpcHealthTimer(0.f)
        , mNpcHealthAlpha(1.f)
        , mCallback(new ResponseCallback(this))
        , mGreetingCallback(new ResponseCallback(this, false))
    {
        // Centre dialog
        center();

        mPersuasionDialog.setVisible(false);

        // History view
        getWidget(mHistory, "History");
        mHistory->setNeedMouseFocus(true);
        mHistory->eventMouseWheel += MyGUI::newDelegate(this, &DialogueWindow::onMouseWheel);
        mHistory->eventMouseButtonPressed += MyGUI::newDelegate(this, &DialogueWindow::onHistoryDragStart);
        mHistory->eventMouseDrag += MyGUI::newDelegate(this, &DialogueWindow::onHistoryDrag);

        // Answers and topics/actions lists
        getWidget(mChoicesList, "ChoicesList");
        mChoicesList->eventItemSelected += MyGUI::newDelegate(this, &DialogueWindow::onChoiceListItem);
        getWidget(mTopicsList, "TopicsList");
        mTopicsList->eventItemSelected += MyGUI::newDelegate(this, &DialogueWindow::onSelectListItem);

        getWidget(mNpcName, "NpcName");
        getWidget(mNpcHealthBar, "NpcHealth");
        getWidget(mNpcHealthText, "NpcHealthText");
        getWidget(mChoicesLabel, "ChoicesLabel");
        getWidget(mTopicsLabel, "TopicsLabel");

        getWidget(mGoodbyeButton, "ByeButton");
        mGoodbyeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DialogueWindow::onByeClicked);
        getWidget(mUpButton, "UpButton");
        getWidget(mDownButton, "DownButton");
        getWidget(mSelectButton, "SelectButton");
        mUpButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DialogueWindow::onNavigateUp);
        mDownButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DialogueWindow::onNavigateDown);
        mSelectButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DialogueWindow::onNavigateSelect);

        getWidget(mDispositionBar, "Disposition");
        getWidget(mDispositionText,"DispositionText");
        getWidget(mScrollBar, "VScroll");

        mScrollBar->eventScrollChangePosition += MyGUI::newDelegate(this, &DialogueWindow::onScrollbarMoved);

        BookPage::ClickCallback callback = std::bind (&DialogueWindow::notifyLinkClicked, this, std::placeholders::_1);
        mHistory->adviseLinkClicked(callback);

        mMainWidget->castType<MyGUI::Window>()->eventWindowChangeCoord += MyGUI::newDelegate(this, &DialogueWindow::onWindowResize);
        updateChoicePane();
    }

    DialogueWindow::~DialogueWindow()
    {
        deleteLater();
        for (Link* link : mLinks)
            delete link;
        for (const auto& link : mTopicLinks)
            delete link.second;
        for (auto history : mHistoryContents)
            delete history;
    }

    void DialogueWindow::onTradeComplete()
    {
        addResponse("", MyGUI::LanguageManager::getInstance().replaceTags("#{sBarterDialog5}"));
    }

    bool DialogueWindow::exit()
    {
        if ((MWBase::Environment::get().getDialogueManager()->isInChoice()))
        {
            return false;
        }
        else
        {
            stopDialogueCamera();
            resetReference();
            MWBase::Environment::get().getDialogueManager()->goodbyeSelected();
            mTopicsList->scrollToTop();
            return true;
        }
    }

    void DialogueWindow::onOpen()
    {
        positionDialogueWindow();
        startDialogueCamera();
        selectInitialItem();
    }

    void DialogueWindow::onResChange(int width, int height)
    {
        positionDialogueWindow();
        mChoicesList->adjustSize();
        mTopicsList->adjustSize();
        updateChoicePane();
        updateHistory();
    }

    bool DialogueWindow::handleKeyPress(MyGUI::KeyCode key, bool repeat)
    {
        if (mPersuasionDialog.isVisible())
            return false;

        switch (key.getValue())
        {
            case MyGUI::KeyCode::W:
            case MyGUI::KeyCode::ArrowUp:
                return moveSelection(-1);
            case MyGUI::KeyCode::S:
            case MyGUI::KeyCode::ArrowDown:
                return moveSelection(1);
            case MyGUI::KeyCode::E:
            case MyGUI::KeyCode::Return:
            case MyGUI::KeyCode::NumpadEnter:
                if (repeat)
                    return true;
                return activateSelection();
            default:
                return false;
        }
    }

    void DialogueWindow::positionDialogueWindow()
    {
        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntSize size = mMainWidget->getSize();
        size.width = std::min(720, std::max(620, view.width - 16));
        size.height = std::min(400, std::max(330, static_cast<int>(view.height * 0.43f)));
        mMainWidget->setSize(size);

        const int x = std::max(8, (view.width - size.width) / 2);
        // Keep the panel close to the lower edge so the actor's upper body remains unobstructed.
        const int y = std::max(4, view.height - size.height - 6);
        mMainWidget->setPosition(x, y);
    }

    void DialogueWindow::startDialogueCamera()
    {
        if (mDialogueCameraActive)
            return;
        if (mPtr.isEmpty() || !Settings::Manager::getBool("cinematic dialogue camera", "GUI"))
            return;
        MWBase::Environment::get().getWorld()->setDialogueCameraTarget(mPtr);
        mDialogueCameraActive = true;
    }

    void DialogueWindow::stopDialogueCamera()
    {
        if (!mDialogueCameraActive)
            return;
        MWBase::Environment::get().getWorld()->clearDialogueCameraTarget();
        mDialogueCameraActive = false;
    }

    bool DialogueWindow::moveSelection(int direction)
    {
        if (mChoicesList->getVisible() && mChoicesList->getEnabled() && mChoicesList->getItemCount() > 0)
            return mChoicesList->selectNext(direction, true);
        if (mTopicsList->getVisible() && mTopicsList->getEnabled() && mTopicsList->getItemCount() > 0)
            return mTopicsList->selectNext(direction, true);
        if (mGoodbyeButton->getEnabled())
        {
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mGoodbyeButton);
            return true;
        }
        return false;
    }

    bool DialogueWindow::activateSelection()
    {
        if (mChoicesList->getVisible() && mChoicesList->getEnabled() && mChoicesList->activateSelected())
            return true;
        if (mTopicsList->getVisible() && mTopicsList->getEnabled() && mTopicsList->activateSelected())
            return true;
        if (mGoodbyeButton->getEnabled())
        {
            onByeClicked(mGoodbyeButton);
            return true;
        }
        return false;
    }

    void DialogueWindow::selectInitialItem()
    {
        if (mChoicesList->getVisible() && mChoicesList->getEnabled() && mChoicesList->getItemCount() > 0)
        {
            mTopicsList->clearSelection();
            if (mChoicesList->getSelectedIndex() < 0)
                mChoicesList->selectNext(1, true);
            return;
        }
        mChoicesList->clearSelection();
        if (mTopicsList->getEnabled() && mTopicsList->getItemCount() > 0)
        {
            if (mTopicsList->getSelectedIndex() < 0)
                mTopicsList->selectNext(1, true);
            return;
        }
        if (mGoodbyeButton->getEnabled())
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mGoodbyeButton);
    }

    void DialogueWindow::onNavigateUp(MyGUI::Widget* sender)
    {
        moveSelection(-1);
    }

    void DialogueWindow::onNavigateDown(MyGUI::Widget* sender)
    {
        moveSelection(1);
    }

    void DialogueWindow::onNavigateSelect(MyGUI::Widget* sender)
    {
        activateSelection();
    }

    void DialogueWindow::onChoiceListItem(const std::string& choice, int id)
    {
        if (id < 0 || static_cast<std::size_t>(id) >= mChoices.size())
            return;
        onChoiceActivated(mChoices[static_cast<std::size_t>(id)].second);
    }

    void DialogueWindow::updateChoicePane()
    {
        const int rightX = mTopicsList->getLeft();
        const int rightWidth = mTopicsList->getWidth();
        const int contentBottom = std::max(132, mSelectButton->getTop() - 8);

        mChoicesList->clear();
        for (const auto& choice : mChoices)
            mChoicesList->addItem(choice.first);
        mChoicesList->adjustSize();

        const bool hasChoices = !mChoices.empty();
        mChoicesLabel->setVisible(hasChoices);
        mChoicesList->setVisible(hasChoices);
        mChoicesList->setEnabled(hasChoices);

        // A question is modal: while answers are displayed, hide all ordinary
        // topics and services (barter, persuasion, training, travel, etc.).
        mTopicsLabel->setVisible(!hasChoices);
        mTopicsList->setVisible(!hasChoices);

        // DialogueWindow is created while WindowManager builds the GUI, before
        // DialogueManager is guaranteed to be installed in Environment. Do not
        // dereference it from the constructor-time updateChoicePane() call.
        MWBase::DialogueManager* dialogueManager = MWBase::Environment::get().getDialogueManager();
        const bool inChoice = dialogueManager != nullptr && dialogueManager->isInChoice();
        mTopicsList->setEnabled(!hasChoices && !inChoice && !mGoodbye);

        if (hasChoices)
        {
            mChoicesLabel->setCoord(rightX, 44, rightWidth, 18);
            mChoicesList->setCoord(rightX, 68, rightWidth, std::max(80, contentBottom - 68));
            mTopicsList->clearSelection();
        }
        else
        {
            mTopicsLabel->setCoord(rightX, 44, rightWidth, 18);
            mTopicsList->setCoord(rightX, 72, rightWidth, std::max(80, contentBottom - 72));
            mTopicsList->adjustSize();
        }
    }

    void DialogueWindow::onHistoryDragStart(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id)
    {
        if (id != MyGUI::MouseButton::Left)
            return;
        mHistoryDragStart = MyGUI::IntPoint(left, top);
        mHistoryLastDragPosition = mHistoryDragStart;
        mHistoryWasDragged = false;
    }

    void DialogueWindow::onHistoryDrag(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id)
    {
        if (id != MyGUI::MouseButton::Left || !mScrollBar->getVisible())
            return;

        const MyGUI::IntPoint current(left, top);
        const MyGUI::IntPoint total = current - mHistoryDragStart;
        if (std::abs(total.left) > 4 || std::abs(total.top) > 4)
            mHistoryWasDragged = true;

        if (mHistoryWasDragged)
        {
            const int delta = current.top - mHistoryLastDragPosition.top;
            const int maxPosition = std::max(0, static_cast<int>(mScrollBar->getScrollRange()) - 1);
            const int position = std::max(0, std::min(maxPosition, static_cast<int>(mScrollBar->getScrollPosition()) - delta));
            mScrollBar->setScrollPosition(position);
            onScrollbarMoved(mScrollBar, position);
        }
        mHistoryLastDragPosition = current;
    }

    void DialogueWindow::onWindowResize(MyGUI::Window* _sender)
    {
        // if the window has only been moved, not resized, we don't need to update
        if (mCurrentWindowSize == _sender->getSize()) return;

        mChoicesList->adjustSize();
        mTopicsList->adjustSize();
        updateChoicePane();
        updateHistory();
        updateTopicFormat();
        mCurrentWindowSize = _sender->getSize();
    }

    void DialogueWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (!mScrollBar->getVisible())
            return;
        mScrollBar->setScrollPosition(std::min(static_cast<int>(mScrollBar->getScrollRange()-1),
                                               std::max(0, static_cast<int>(mScrollBar->getScrollPosition() - _rel*0.3))));
        onScrollbarMoved(mScrollBar, mScrollBar->getScrollPosition());
    }

    void DialogueWindow::onByeClicked(MyGUI::Widget* _sender)
    {
        if (exit())
        {
            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Dialogue);
        }
    }

    void DialogueWindow::onSelectListItem(const std::string& topic, int id)
    {
        /*
            Start of tes3mp change (major)

            Instead of activating a list item here, send an ObjectDialogueChoice packet to the server
            and let it decide whether the list item gets activated
        */
        sendDialogueChoicePacket(topic);
        return;
        /*
            End of tes3mp change (major)
        */

        MWBase::DialogueManager* dialogueManager = MWBase::Environment::get().getDialogueManager();

        if (mGoodbye || dialogueManager->isInChoice())
            return;

        const MWWorld::Store<ESM::GameSetting> &gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        const std::string sPersuasion = gmst.find("sPersuasion")->mValue.getString();
        const std::string sCompanionShare = gmst.find("sCompanionShare")->mValue.getString();
        const std::string sBarter = gmst.find("sBarter")->mValue.getString();
        const std::string sSpells = gmst.find("sSpells")->mValue.getString();
        const std::string sTravel = gmst.find("sTravel")->mValue.getString();
        const std::string sSpellMakingMenuTitle = gmst.find("sSpellMakingMenuTitle")->mValue.getString();
        const std::string sEnchanting = gmst.find("sEnchanting")->mValue.getString();
        const std::string sServiceTrainingTitle = gmst.find("sServiceTrainingTitle")->mValue.getString();
        const std::string sRepair = gmst.find("sRepair")->mValue.getString();

        if (topic != sPersuasion && topic != sCompanionShare && topic != sBarter 
         && topic != sSpells && topic != sTravel && topic != sSpellMakingMenuTitle 
         && topic != sEnchanting && topic != sServiceTrainingTitle && topic != sRepair)
        {
            onTopicActivated(topic);
            if (mGoodbyeButton->getEnabled())
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mGoodbyeButton);
        }
        else if (topic == sPersuasion)
            mPersuasionDialog.setVisible(true);
        else if (topic == sCompanionShare)
            MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Companion, mPtr);
        else if (!dialogueManager->checkServiceRefused(mCallback.get()))
        {
            if (topic == sBarter && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Barter))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Barter, mPtr);
            else if (topic == sSpells && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Spells))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellBuying, mPtr);
            else if (topic == sTravel && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Travel))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Travel, mPtr);
            else if (topic == sSpellMakingMenuTitle && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Spellmaking))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellCreation, mPtr);
            else if (topic == sEnchanting && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Enchanting))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Enchanting, mPtr);
            else if (topic == sServiceTrainingTitle && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Training))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Training, mPtr);
            else if (topic == sRepair && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Repair))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_MerchantRepair, mPtr);
        }
        else
            updateTopics();
    }

    /*
        Start of tes3mp addition

        A different event that should be used in multiplayer when clicking on choices
        in the dialogue screen, sending DialogueChoice packets to the server so they can
        be approved or denied
    */
    void DialogueWindow::sendDialogueChoicePacket(const std::string& topic)
    {
        mwmp::ObjectList* objectList = mwmp::Main::get().getNetworking()->getObjectList();
        objectList->reset();
        objectList->packetOrigin = mwmp::CLIENT_GAMEPLAY;
        objectList->addObjectDialogueChoice(mPtr, topic);
        objectList->sendObjectDialogueChoice();
    }
    /*
        End of tes3mp addition
    */

    /*
        Start of tes3mp addition

        Make it possible to activate any dialogue choice from elsewhere in the code
    */
    void DialogueWindow::activateDialogueChoice(unsigned char dialogueChoiceType, std::string topic)
    {
        if (dialogueChoiceType == mwmp::DialogueChoiceType::TOPIC)
        {
            onTopicActivated(topic);
        }
        else if (dialogueChoiceType == mwmp::DialogueChoiceType::PERSUASION)
            mPersuasionDialog.setVisible(true);
        else if (dialogueChoiceType == mwmp::DialogueChoiceType::COMPANION_SHARE)
            MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Companion, mPtr);
        else
        {
            MWBase::DialogueManager* dialogueManager = MWBase::Environment::get().getDialogueManager();

            if (dialogueChoiceType == mwmp::DialogueChoiceType::BARTER && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Barter))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Barter, mPtr);
            else if (dialogueChoiceType == mwmp::DialogueChoiceType::SPELLS && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Spells))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellBuying, mPtr);
            else if (dialogueChoiceType == mwmp::DialogueChoiceType::TRAVEL && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Travel))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Travel, mPtr);
            else if (dialogueChoiceType == mwmp::DialogueChoiceType::SPELLMAKING && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Spellmaking))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellCreation, mPtr);
            else if (dialogueChoiceType == mwmp::DialogueChoiceType::ENCHANTING && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Enchanting))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Enchanting, mPtr);
            else if (dialogueChoiceType == mwmp::DialogueChoiceType::TRAINING && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Training))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Training, mPtr);
            else if (dialogueChoiceType == mwmp::DialogueChoiceType::REPAIR && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Repair))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_MerchantRepair, mPtr);
        }
    }
    /*
        End of tes3mp addition
    */

    /*
        Start of tes3mp addition

        Make it possible to get the Ptr of the actor involved in the dialogue
    */
    MWWorld::Ptr DialogueWindow::getPtr()
    {
        return mPtr;
    }
    /*
        End of tes3mp addition
    */

    void DialogueWindow::setPtr(const MWWorld::Ptr& actor)
    {
        if (!actor.getClass().isActor())
        {
            Log(Debug::Warning) << "Warning: can not talk with non-actor object.";
            return;
        }

        bool sameActor = (mPtr == actor);
        if (!sameActor)
        {
            // The history is not reset here
            mKeywords.clear();
            mTopicsList->clear();
            for (Link* link : mLinks)
                mDeleteLater.push_back(link); // Links are not deleted right away to prevent issues with event handlers
            mLinks.clear();
        }

        mPtr = actor;
        mGoodbye = false;
        mTopicsList->setEnabled(true);
        if (!sameActor)
        {
            mNpcHealthTimer = 0.f;
            mNpcHealthAlpha = 1.f;
            mNpcHealthBar->setAlpha(1.f);
            mNpcHealthBar->setVisible(true);
            mNpcHealthText->setVisible(true);
        }

        if (!MWBase::Environment::get().getDialogueManager()->startDialogue(actor, mGreetingCallback.get()))
        {
            // No greetings found. The dialogue window should not be shown.
            // If this is a companion, we must show the companion window directly (used by BM_bear_be_unique).
            stopDialogueCamera();
            MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Dialogue);
            mPtr = MWWorld::Ptr();
            if (isCompanion(actor))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Companion, actor);
            return;
        }

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mGoodbyeButton);

        const std::string actorName = mPtr.getClass().getName(mPtr);
        setTitle(actorName);
        updateActorStatus();

        updateTopics();
        updateTopicsPane(); // force update for new services

        updateDisposition();
        restock();
        startDialogueCamera();
        selectInitialItem();
    }

    void DialogueWindow::restock()
    {
        MWMechanics::CreatureStats &sellerStats = mPtr.getClass().getCreatureStats(mPtr);
        float delay = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fBarterGoldResetDelay")->mValue.getFloat();

        // Gold is restocked every 24h
        if (MWBase::Environment::get().getWorld()->getTimeStamp() >= sellerStats.getLastRestockTime() + delay)
        {
            /*
                Start of tes3mp change (major)

                Instead of restocking the NPC's gold pool or last restock time here, send a packet about them to the server
            */
            /*
            sellerStats.setGoldPool(mPtr.getClass().getBaseGold(mPtr));

            sellerStats.setLastRestockTime(MWBase::Environment::get().getWorld()->getTimeStamp());
            */
            mwmp::ObjectList* objectList = mwmp::Main::get().getNetworking()->getObjectList();
            objectList->reset();
            objectList->packetOrigin = mwmp::CLIENT_GAMEPLAY;
            objectList->addObjectMiscellaneous(mPtr, mPtr.getClass().getBaseGold(mPtr), MWBase::Environment::get().getWorld()->getTimeStamp().getHour(),
                MWBase::Environment::get().getWorld()->getTimeStamp().getDay());
            objectList->sendObjectMiscellaneous();
            /*
                End of tes3mp change (major)
            */
        }
    }

    void DialogueWindow::deleteLater()
    {
        for (Link* link : mDeleteLater)
            delete link;
        mDeleteLater.clear();
    }

    void DialogueWindow::onClose()
    {
        if (MWBase::Environment::get().getWindowManager()->containsMode(GM_Dialogue))
            return;
        stopDialogueCamera();
        // Reset history
        for (DialogueText* text : mHistoryContents)
            delete text;
        mHistoryContents.clear();
    }

    bool DialogueWindow::setKeywords(std::list<std::string> keyWords)
    {
        if (mKeywords == keyWords && isCompanion() == mIsCompanion)
            return false;
        mIsCompanion = isCompanion();
        mKeywords = keyWords;
        updateTopicsPane();
        return true;
    }

    void DialogueWindow::updateTopicsPane()
    {
        mTopicsList->clear();
        for (auto& linkPair : mTopicLinks)
            mDeleteLater.push_back(linkPair.second);
        mTopicLinks.clear();
        mKeywordSearch.clear();

        int services = mPtr.getClass().getServices(mPtr);

        bool travel = (mPtr.getTypeName() == typeid(ESM::NPC).name() && !mPtr.get<ESM::NPC>()->mBase->getTransport().empty())
                || (mPtr.getTypeName() == typeid(ESM::Creature).name() && !mPtr.get<ESM::Creature>()->mBase->getTransport().empty());

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        if (mPtr.getTypeName() == typeid(ESM::NPC).name())
            mTopicsList->addItem(gmst.find("sPersuasion")->mValue.getString());

        if (services & ESM::NPC::AllItems)
            mTopicsList->addItem(gmst.find("sBarter")->mValue.getString());

        if (services & ESM::NPC::Spells)
            mTopicsList->addItem(gmst.find("sSpells")->mValue.getString());

        if (travel)
            mTopicsList->addItem(gmst.find("sTravel")->mValue.getString());

        if (services & ESM::NPC::Spellmaking)
            mTopicsList->addItem(gmst.find("sSpellmakingMenuTitle")->mValue.getString());

        if (services & ESM::NPC::Enchanting)
            mTopicsList->addItem(gmst.find("sEnchanting")->mValue.getString());

        if (services & ESM::NPC::Training)
            mTopicsList->addItem(gmst.find("sServiceTrainingTitle")->mValue.getString());

        if (services & ESM::NPC::Repair)
            mTopicsList->addItem(gmst.find("sRepair")->mValue.getString());

        if (isCompanion())
            mTopicsList->addItem(gmst.find("sCompanionShare")->mValue.getString());

        if (mTopicsList->getItemCount() > 0)
            mTopicsList->addSeparator();


        for(const auto& keyword : mKeywords)
        {
            std::string topicId = Misc::StringUtils::lowerCase(keyword);
            mTopicsList->addItem(keyword);

            Topic* t = new Topic(keyword);
            /*
                Start of tes3mp change (major)

                Instead of running DialogueWindow::onSelectListItem() when clicking a highlighted topic, run
                onSendDialoguePacket() so the server can approve or deny a dialogue choice
            */
            //t->eventTopicActivated += MyGUI::newDelegate(this, &DialogueWindow::onTopicActivated);
            t->eventTopicActivated += MyGUI::newDelegate(this, &DialogueWindow::sendDialogueChoicePacket);
            /*
                End of tes3mp change (major)
            */
            
            mTopicLinks[topicId] = t;

            mKeywordSearch.seed(topicId, intptr_t(t));
        }
        mTopicsList->adjustSize();

        updateHistory();
        // The topics list has been regenerated so topic formatting needs to be updated
        updateTopicFormat();
        selectInitialItem();
    }

    void DialogueWindow::updateHistory(bool scrollbar)
    {
        if (!scrollbar && mScrollBar->getVisible())
        {
            mHistory->setSize(mHistory->getSize()+MyGUI::IntSize(mScrollBar->getWidth(),0));
            mScrollBar->setVisible(false);
        }
        if (scrollbar && !mScrollBar->getVisible())
        {
            mHistory->setSize(mHistory->getSize()-MyGUI::IntSize(mScrollBar->getWidth(),0));
            mScrollBar->setVisible(true);
        }

        BookTypesetter::Ptr typesetter = BookTypesetter::create (mHistory->getWidth(), std::numeric_limits<int>::max());

        for (DialogueText* text : mHistoryContents)
            text->write(typesetter, &mKeywordSearch, mTopicLinks);

        mChoices = MWBase::Environment::get().getDialogueManager()->getChoices();
        mGoodbye = MWBase::Environment::get().getDialogueManager()->isGoodbye();
        updateChoicePane();

        TypesetBook::Ptr book = typesetter->complete();
        mHistory->showPage(book, 0);
        size_t viewHeight = mHistory->getParent()->getHeight();
        if (!scrollbar && book->getSize().second > viewHeight)
            updateHistory(true);
        else if (scrollbar)
        {
            mHistory->setSize(MyGUI::IntSize(mHistory->getWidth(), book->getSize().second));
            size_t range = book->getSize().second - viewHeight;
            mScrollBar->setScrollRange(range);
            mScrollBar->setScrollPosition(range-1);
            mScrollBar->setTrackSize(static_cast<int>(viewHeight / static_cast<float>(book->getSize().second) * mScrollBar->getLineSize()));
            onScrollbarMoved(mScrollBar, range-1);
        }
        else
        {
            // no scrollbar
            onScrollbarMoved(mScrollBar, 0);
        }

        bool goodbyeEnabled = !MWBase::Environment::get().getDialogueManager()->isInChoice() || mGoodbye;
        bool goodbyeWasEnabled = mGoodbyeButton->getEnabled();
        mGoodbyeButton->setEnabled(goodbyeEnabled);
        if (goodbyeEnabled && !goodbyeWasEnabled)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mGoodbyeButton);

        bool topicsEnabled = !MWBase::Environment::get().getDialogueManager()->isInChoice() && !mGoodbye;
        mTopicsList->setEnabled(topicsEnabled);
        selectInitialItem();
    }

    void DialogueWindow::notifyLinkClicked (TypesetBook::InteractiveId link)
    {
        if (mHistoryWasDragged)
        {
            mHistoryWasDragged = false;
            return;
        }
        reinterpret_cast<Link*>(link)->activated();
    }

    void DialogueWindow::onTopicActivated(const std::string &topicId)
    {
        if (mGoodbye)
            return;

        MWBase::Environment::get().getDialogueManager()->keywordSelected(topicId, mCallback.get());
        updateTopics();
    }

    void DialogueWindow::onChoiceActivated(int id)
    {
        if (mGoodbye)
        {
            onGoodbyeActivated();
            return;
        }
        MWBase::Environment::get().getDialogueManager()->questionAnswered(id, mCallback.get());
        updateTopics();
    }

    void DialogueWindow::onGoodbyeActivated()
    {
        stopDialogueCamera();
        MWBase::Environment::get().getDialogueManager()->goodbyeSelected();
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Dialogue);
        resetReference();
    }

    void DialogueWindow::onScrollbarMoved(MyGUI::ScrollBar *sender, size_t pos)
    {
        mHistory->setPosition(0, static_cast<int>(pos) * -1);
    }

    void DialogueWindow::addResponse(const std::string &title, const std::string &text, bool needMargin)
    {
        mHistoryContents.push_back(new Response(text, title, needMargin));
        updateHistory();
    }

    void DialogueWindow::addMessageBox(const std::string& text)
    {
        mHistoryContents.push_back(new Message(text));
        updateHistory();
    }

    void DialogueWindow::updateActorStatus()
    {
        if (mPtr.isEmpty() || !mPtr.getClass().isActor())
        {
            mNpcHealthBar->setVisible(false);
            mNpcHealthText->setVisible(false);
            return;
        }

        MWMechanics::CreatureStats& stats = mPtr.getClass().getCreatureStats(mPtr);
        const int level = stats.getLevel();
        const int maximumHealth = std::max(1, static_cast<int>(std::lround(stats.getHealth().getModified())));
        const int currentHealth = std::max(0, std::min(maximumHealth,
            static_cast<int>(std::lround(stats.getHealth().getCurrent()))));

        mNpcName->setCaption(mPtr.getClass().getName(mPtr) + " - "
            + MyGUI::utility::toString(level) + " lvl");
        mNpcHealthBar->setProgressRange(static_cast<size_t>(maximumHealth));
        mNpcHealthBar->setProgressPosition(static_cast<size_t>(currentHealth));
        mNpcHealthText->setCaption(MyGUI::utility::toString(currentHealth) + " / "
            + MyGUI::utility::toString(maximumHealth));
        const bool healthVisible = !stats.isDead() && mNpcHealthAlpha > 0.001f;
        mNpcHealthBar->setVisible(healthVisible);
        mNpcHealthText->setVisible(healthVisible);
    }

    void DialogueWindow::updateDisposition()
    {
        bool dispositionVisible = false;
        if (!mPtr.isEmpty() && mPtr.getClass().isNpc())
        {
            dispositionVisible = true;
            mDispositionBar->setProgressRange(100);
            mDispositionBar->setProgressPosition(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr));
            mDispositionText->setCaption(MyGUI::utility::toString(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr))+std::string("/100"));
        }

        mDispositionBar->setVisible(dispositionVisible);
        mDispositionText->setVisible(dispositionVisible);
    }

    void DialogueWindow::onReferenceUnavailable()
    {
        stopDialogueCamera();
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Dialogue);
    }

    void DialogueWindow::onFrame(float dt)
    {
        checkReferenceAvailable();
        if (mPtr.isEmpty())
            return;

        updateActorStatus();

        // Show the dialogue health bar for three seconds, then fade it out.
        mNpcHealthTimer += dt;
        constexpr float healthHoldTime = 3.f;
        constexpr float healthFadeTime = 0.65f;
        float targetAlpha = 1.f;
        if (mNpcHealthTimer > healthHoldTime)
            targetAlpha = std::max(0.f, 1.f - (mNpcHealthTimer - healthHoldTime) / healthFadeTime);
        if (std::abs(targetAlpha - mNpcHealthAlpha) > 0.001f)
        {
            mNpcHealthAlpha = targetAlpha;
            mNpcHealthBar->setAlpha(mNpcHealthAlpha);
            if (mNpcHealthAlpha <= 0.001f)
            {
                mNpcHealthBar->setVisible(false);
                mNpcHealthText->setVisible(false);
            }
        }

        updateDisposition();
        deleteLater();

        if (mChoices != MWBase::Environment::get().getDialogueManager()->getChoices()
                || mGoodbye != MWBase::Environment::get().getDialogueManager()->isGoodbye())
            updateHistory();
    }

    void DialogueWindow::updateTopicFormat()
    {
        if (!Settings::Manager::getBool("color topic enable", "GUI"))
            return;

        std::string specialColour = Settings::Manager::getString("color topic specific", "GUI");
        std::string oldColour = Settings::Manager::getString("color topic exhausted", "GUI");

        for (const std::string& keyword : mKeywords)
        {
            int flag = MWBase::Environment::get().getDialogueManager()->getTopicFlag(keyword);
            MyGUI::Button* button = mTopicsList->getItemWidget(keyword);
            if (!button)
                continue;

            if (!specialColour.empty() && flag & MWBase::DialogueManager::TopicType::Specific)
                button->getSubWidgetText()->setTextColour(MyGUI::Colour::parse(specialColour));
            else if (!oldColour.empty() && flag & MWBase::DialogueManager::TopicType::Exhausted)
                button->getSubWidgetText()->setTextColour(MyGUI::Colour::parse(oldColour));
        }

        const int selected = mTopicsList->getSelectedIndex();
        if (selected >= 0)
            mTopicsList->setSelectedIndex(selected, false);
    }

    void DialogueWindow::updateTopics()
    {
        // Topic formatting needs to be updated regardless of whether the topic list has changed
        if (!setKeywords(MWBase::Environment::get().getDialogueManager()->getAvailableTopics()))
            updateTopicFormat();
    }

    bool DialogueWindow::isCompanion()
    {
        return isCompanion(mPtr);
    }

    bool DialogueWindow::isCompanion(const MWWorld::Ptr& actor)
    {
        if (actor.isEmpty())
            return false;

        return !actor.getClass().getScript(actor).empty()
                && actor.getRefData().getLocals().getIntVar(actor.getClass().getScript(actor), "companion");
    }

}
