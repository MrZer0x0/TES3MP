#include "hud.hpp"

#include <MyGUI_RenderManager.h>
#include <MyGUI_ProgressBar.h>
#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ScrollView.h>

#include <cmath>
#include <iomanip>
#include <sstream>
#include <limits>

/*
    Start of tes3mp addition

    Include additional headers for multiplayer purposes
*/
#include "../mwmp/Main.hpp"
#include "../mwmp/Networking.hpp"
#include "../mwmp/ObjectList.hpp"
#include "../mwworld/cellstore.hpp"
/*
    End of tes3mp addition
*/

#include <components/settings/settings.hpp>
#include <components/openmw-mp/TimedLog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "inventorywindow.hpp"
#include "spellicons.hpp"
#include "itemmodel.hpp"
#include "draganddrop.hpp"

#include "itemwidget.hpp"

namespace MWGui
{

    /**
     * Makes it possible to use ItemModel::moveItem to move an item from an inventory to the world.
     */
    class WorldItemModel : public ItemModel
    {
    public:
        WorldItemModel(float left, float top) : mLeft(left), mTop(top) {}
        virtual ~WorldItemModel() override {}
        MWWorld::Ptr copyItem (const ItemStack& item, size_t count, bool /*allowAutoEquip*/) override
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();

            MWWorld::Ptr dropped;
            if (world->canPlaceObject(mLeft, mTop))
                dropped = world->placeObject(item.mBase, mLeft, mTop, count);
            else
                dropped = world->dropObjectOnGround(world->getPlayerPtr(), item.mBase, count);
            dropped.getCellRef().setOwner("");

            /*
                Start of tes3mp addition

                Send an ID_OBJECT_PLACE packet every time an object is dropped into the world from
                the inventory screen
            */
            mwmp::ObjectList *objectList = mwmp::Main::get().getNetworking()->getObjectList();
            objectList->reset();
            objectList->packetOrigin = mwmp::CLIENT_GAMEPLAY;
            objectList->addObjectPlace(dropped, true);
            objectList->sendObjectPlace();
            /*
                End of tes3mp addition
            */

            /*
                Start of tes3mp change (major)

                Instead of actually keeping this object as is, delete it after sending the packet
                and wait for the server to send it back with a unique mpNum of its own
            */
            MWBase::Environment::get().getWorld()->deleteObject(dropped);
            /*
                End of tes3mp change (major)
            */

            return dropped;
        }

        void removeItem (const ItemStack& item, size_t count) override { throw std::runtime_error("removeItem not implemented"); }
        ModelIndex getIndex (ItemStack item) override { throw std::runtime_error("getIndex not implemented"); }
        void update() override {}
        size_t getItemCount() override { return 0; }
        ItemStack getItem (ModelIndex index) override { throw std::runtime_error("getItem not implemented"); }

    private:
        // Where to drop the item
        float mLeft;
        float mTop;
    };


    HUD::HUD(CustomMarkerCollection &customMarkers, DragAndDrop* dragAndDrop, MWRender::LocalMap* localMapRender)
        : WindowBase("openmw_hud.layout")
        , LocalMapBase(customMarkers, localMapRender, Settings::Manager::getBool("local map hud fog of war", "Map"))
        , mHealth(nullptr)
        , mMagicka(nullptr)
        , mStamina(nullptr)
        , mDrowning(nullptr)
        , mHealthText(nullptr)
        , mMagickaText(nullptr)
        , mStaminaText(nullptr)
        , mFpsBox(nullptr)
        , mPingBox(nullptr)
        , mEnemyHealthFrame(nullptr)
        , mHealthBorder(nullptr)
        , mMagickaBorder(nullptr)
        , mFatigueBorder(nullptr)
        , mEnemyHealthBorder(nullptr)
        , mWeapStatusBorder(nullptr)
        , mSpellStatusBorder(nullptr)
        , mWeapImage(nullptr)
        , mSpellImage(nullptr)
        , mWeapStatus(nullptr)
        , mSpellStatus(nullptr)
        , mEffectBox(nullptr)
        , mMinimap(nullptr)
        , mCrosshair(nullptr)
        , mCellNameBox(nullptr)
        , mGameTimeBox(nullptr)
        , mDrowningFrame(nullptr)
        , mDrowningBorder(nullptr)
        , mDrowningFlash(nullptr)
        , mHealthManaStaminaBaseLeft(0)
        , mWeapBoxBaseLeft(0)
        , mSpellBoxBaseLeft(0)
        , mMinimapBoxBaseRight(0)
        , mEffectBoxBaseRight(0)
        , mDragAndDrop(dragAndDrop)
        , mCellNameTimer(0.0f)
        , mWeaponSpellTimer(0.f)
        , mGameTimeUpdateTimer(0.f)
        , mMapVisible(true)
        , mWeaponVisible(true)
        , mSpellVisible(true)
        , mWorldMouseOver(false)
        , mEnemyActorId(-1)
        , mEnemyHealthTimer(-1)
        , mFpsUpdateTimer(0.f)
        , mPingUpdateTimer(0.f)
        , mFpsAccumulatedTime(0.f)
        , mFpsFrameCount(0)
        , mHealthVisibilityTimer(7.f)
        , mMagickaVisibilityTimer(7.f)
        , mFatigueVisibilityTimer(7.f)
        , mLastHealthCurrent(std::numeric_limits<int>::min())
        , mLastHealthModified(std::numeric_limits<int>::min())
        , mLastMagickaCurrent(std::numeric_limits<int>::min())
        , mLastMagickaModified(std::numeric_limits<int>::min())
        , mLastFatigueCurrent(std::numeric_limits<int>::min())
        , mLastFatigueModified(std::numeric_limits<int>::min())
        , mIsDrowning(false)
        , mDrowningFlashTheta(0.f)
    {
        mMainWidget->setSize(MyGUI::RenderManager::getInstance().getViewSize());

        // Energy bars
        getWidget(mHealthFrame, "HealthFrame");
        getWidget(mMagickaFrame, "MagickaFrame");
        getWidget(mFatigueFrame, "FatigueFrame");
        getWidget(mHealth, "Health");
        getWidget(mMagicka, "Magicka");
        getWidget(mStamina, "Stamina");
        getWidget(mEnemyHealthFrame, "EnemyHealthFrame");
        getWidget(mEnemyHealthBorder, "EnemyHealthBorder");
        getWidget(mEnemyHealth, "EnemyHealth");
        getWidget(mHealthBorder, "HealthBorder");
        getWidget(mMagickaBorder, "MagickaBorder");
        getWidget(mFatigueBorder, "FatigueBorder");
        getWidget(mHealthText, "HealthText");
        getWidget(mMagickaText, "MagickaText");
        getWidget(mStaminaText, "StaminaText");
        getWidget(mFpsBox, "FpsText");
        getWidget(mPingBox, "PingText");
        mHealthManaStaminaBaseLeft = mHealthFrame->getLeft();

        mHealthFrame->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onHMSClicked);
        mMagickaFrame->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onHMSClicked);
        mFatigueFrame->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onHMSClicked);

        mHealthFrame->setVisible(false);
        mMagickaFrame->setVisible(false);
        mFatigueFrame->setVisible(false);

        //Drowning bar
        getWidget(mDrowningFrame, "DrowningFrame");
        getWidget(mDrowningBorder, "DrowningBorder");
        getWidget(mDrowning, "Drowning");
        getWidget(mDrowningFlash, "Flash");
        mDrowning->setProgressRange(200);

        const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

        // Item and spell images and status bars
        getWidget(mWeapBox, "WeapBox");
        getWidget(mWeapImage, "WeapImage");
        getWidget(mWeapStatusBorder, "WeapStatusBorder");
        getWidget(mWeapStatus, "WeapStatus");
        mWeapBoxBaseLeft = mWeapBox->getLeft();
        mWeapBox->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onWeaponClicked);

        getWidget(mSpellBox, "SpellBox");
        getWidget(mSpellImage, "SpellImage");
        getWidget(mSpellStatusBorder, "SpellStatusBorder");
        getWidget(mSpellStatus, "SpellStatus");
        mSpellBoxBaseLeft = mSpellBox->getLeft();
        mSpellBox->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onMagicClicked);

        getWidget(mSneakBox, "SneakBox");
        mSneakBoxBaseLeft = mSneakBox->getLeft();

        getWidget(mEffectBox, "EffectBox");
        mEffectBoxBaseRight = viewSize.width - mEffectBox->getRight();

        getWidget(mMinimapBox, "MiniMapBox");
        mMinimapBoxBaseRight = viewSize.width - mMinimapBox->getRight();
        getWidget(mMinimap, "MiniMap");
        getWidget(mCompass, "Compass");
        getWidget(mMinimapButton, "MiniMapButton");
        mMinimapButton->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onMapClicked);

        getWidget(mCellNameBox, "CellName");
        getWidget(mWeaponSpellBox, "WeaponSpellName");
        getWidget(mGameTimeBox, "GameTime");

        getWidget(mCrosshair, "Crosshair");

        cacheOriginalCoord(mEnemyHealthBorder);
        cacheOriginalCoord(mHealthBorder);
        cacheOriginalCoord(mMagickaBorder);
        cacheOriginalCoord(mFatigueBorder);
        cacheOriginalCoord(mSpellStatusBorder);
        cacheOriginalCoord(mWeapStatusBorder);
        cacheOriginalCoord(mDrowningBorder);

        updateCenteredBar(mHealthBorder, mHealth, 1.f, 2, 2, mHealthText);
        updateCenteredBar(mMagickaBorder, mMagicka, 1.f, 2, 2, mMagickaText);
        updateCenteredBar(mFatigueBorder, mStamina, 1.f, 2, 2, mStaminaText);
        updateCenteredBar(mSpellStatusBorder, mSpellStatus, 0.f, 2, 2);
        updateCenteredBar(mWeapStatusBorder, mWeapStatus, 0.f, 2, 2);
        updateCenteredBar(mDrowningBorder, mDrowning, 1.f, 2, 2);
        updatePingPosition();

        LocalMapBase::init(mMinimap, mCompass);

        mMainWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onWorldClicked);
        mMainWidget->eventMouseMove += MyGUI::newDelegate(this, &HUD::onWorldMouseOver);
        mMainWidget->eventMouseLostFocus += MyGUI::newDelegate(this, &HUD::onWorldMouseLostFocus);

        mSpellIcons = new SpellIcons();
    }

    HUD::~HUD()
    {
        mMainWidget->eventMouseLostFocus.clear();
        mMainWidget->eventMouseMove.clear();
        mMainWidget->eventMouseButtonClick.clear();

        delete mSpellIcons;
    }

    void HUD::setValue(const std::string& id, const MWMechanics::DynamicStat<float>& value)
    {
        int current = static_cast<int>(value.getCurrent());
        int modified = static_cast<int>(value.getModified());

        // Fatigue can be negative
        if (id != "FBar")
            current = std::max(0, current);

        MyGUI::Widget* w;
        std::string valStr = MyGUI::utility::toString(current) + " / " + MyGUI::utility::toString(modified);
        if (id == "HBar")
        {
            mHealth->setProgressRange(std::max(0, modified));
            mHealth->setProgressPosition(std::max(0, current));
            if (mHealthText)
                mHealthText->setCaption(valStr);
            getWidget(w, "HealthFrame");
            w->setUserString("Caption_HealthDescription", "#{sHealthDesc}" + valStr);
            updateCenteredBar(mHealthBorder, mHealth, modified > 0 ? static_cast<float>(current) / static_cast<float>(modified) : 0.f, 2, 2, mHealthText);

            if (current != mLastHealthCurrent || modified != mLastHealthModified)
            {
                showAutoHideWidget(mHealthFrame, mHealthVisibilityTimer, getVisibilityTimeout(current, modified));
                mLastHealthCurrent = current;
                mLastHealthModified = modified;
            }
        }
        else if (id == "MBar")
        {
            mMagicka->setProgressRange(std::max(0, modified));
            mMagicka->setProgressPosition(std::max(0, current));
            if (mMagickaText)
                mMagickaText->setCaption(valStr);
            getWidget(w, "MagickaFrame");
            w->setUserString("Caption_HealthDescription", "#{sMagDesc}" + valStr);
            updateCenteredBar(mMagickaBorder, mMagicka, modified > 0 ? static_cast<float>(current) / static_cast<float>(modified) : 0.f, 2, 2, mMagickaText);

            if (current != mLastMagickaCurrent || modified != mLastMagickaModified)
            {
                showAutoHideWidget(mMagickaFrame, mMagickaVisibilityTimer, getVisibilityTimeout(current, modified));
                mLastMagickaCurrent = current;
                mLastMagickaModified = modified;
            }
        }
        else if (id == "FBar")
        {
            mStamina->setProgressRange(std::max(0, modified));
            mStamina->setProgressPosition(std::max(0, current));
            if (mStaminaText)
                mStaminaText->setCaption(valStr);
            getWidget(w, "FatigueFrame");
            w->setUserString("Caption_HealthDescription", "#{sFatDesc}" + valStr);
            updateCenteredBar(mFatigueBorder, mStamina, modified != 0 ? static_cast<float>(current) / static_cast<float>(modified) : 0.f, 2, 2, mStaminaText);

            if (current != mLastFatigueCurrent || modified != mLastFatigueModified)
            {
                showAutoHideWidget(mFatigueFrame, mFatigueVisibilityTimer, getVisibilityTimeout(current, modified));
                mLastFatigueCurrent = current;
                mLastFatigueModified = modified;
            }
        }
    }

    void HUD::setDrowningTimeLeft(float time, float maxTime)
    {
        size_t progress = static_cast<size_t>(time / maxTime * 200);
        mDrowning->setProgressPosition(progress);
        updateCenteredBar(mDrowningBorder, mDrowning, progress / 200.f, 2, 2);

        bool isDrowning = (progress == 0);
        if (isDrowning && !mIsDrowning) // Just started drowning
            mDrowningFlashTheta = 0.0f; // Start out on bright red every time.

        mDrowningFlash->setVisible(isDrowning);
        mIsDrowning = isDrowning;
    }

    void HUD::setDrowningBarVisible(bool visible)
    {
        mDrowningFrame->setVisible(visible);
        mDrowningFrame->setAlpha(1.f);
        if (!visible && mDrowningBorder)
            mDrowningBorder->setVisible(false);
    }

    void HUD::onWorldClicked(MyGUI::Widget* _sender)
    {
        if (!MWBase::Environment::get().getWindowManager ()->isGuiMode ())
            return;

        MWBase::WindowManager *winMgr = MWBase::Environment::get().getWindowManager();
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            // drop item into the gameworld
            MWBase::Environment::get().getWorld()->breakInvisibility(
                        MWMechanics::getPlayer());

            MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            MyGUI::IntPoint cursorPosition = MyGUI::InputManager::getInstance().getMousePosition();
            float mouseX = cursorPosition.left / float(viewSize.width);
            float mouseY = cursorPosition.top / float(viewSize.height);

            WorldItemModel drop (mouseX, mouseY);
            mDragAndDrop->drop(&drop, nullptr);

            winMgr->changePointer("arrow");
        }
        else
        {
            GuiMode mode = winMgr->getMode();

            if (!winMgr->isConsoleMode() && (mode != GM_Container) && (mode != GM_Inventory))
                return;

            MWWorld::Ptr object = MWBase::Environment::get().getWorld()->getFacedObject();

            if (winMgr->isConsoleMode())
                winMgr->setConsoleSelectedObject(object);
            else //if ((mode == GM_Container) || (mode == GM_Inventory))
            {
                // pick up object
                if (!object.isEmpty())
                /*
                    Start of tes3mp change (major)

                    Disable unilateral picking up of objects on this client

                    Instead, send an ID_OBJECT_ACTIVATE packet every time an item is made to pick up
                    an item here, and expect the server's reply to our packet to cause the actual
                    picking up of items
                */
                    //winMgr->getInventoryWindow()->pickUpObject(object);
                {
                    mwmp::ObjectList *objectList = mwmp::Main::get().getNetworking()->getObjectList();
                    objectList->reset();
                    objectList->packetOrigin = mwmp::CLIENT_GAMEPLAY;
                    objectList->addObjectActivate(object, MWMechanics::getPlayer());
                    objectList->sendObjectActivate();
                }
                /*
                    End of tes3mp change (major)
                */
            }
        }
    }

    void HUD::onWorldMouseOver(MyGUI::Widget* _sender, int x, int y)
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            mWorldMouseOver = false;

            MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            MyGUI::IntPoint cursorPosition = MyGUI::InputManager::getInstance().getMousePosition();
            float mouseX = cursorPosition.left / float(viewSize.width);
            float mouseY = cursorPosition.top / float(viewSize.height);

            MWBase::World* world = MWBase::Environment::get().getWorld();

            // if we can't drop the object at the wanted position, show the "drop on ground" cursor.
            bool canDrop = world->canPlaceObject(mouseX, mouseY);

            if (!canDrop)
                MWBase::Environment::get().getWindowManager()->changePointer("drop_ground");
            else
                MWBase::Environment::get().getWindowManager()->changePointer("arrow");

        }
        else
        {
            MWBase::Environment::get().getWindowManager()->changePointer("arrow");
            mWorldMouseOver = true;
        }
    }

    void HUD::onWorldMouseLostFocus(MyGUI::Widget* _sender, MyGUI::Widget* _new)
    {
        MWBase::Environment::get().getWindowManager()->changePointer("arrow");
        mWorldMouseOver = false;
    }

    void HUD::onHMSClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Stats);
    }

    void HUD::onMapClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Map);
    }

    void HUD::onWeaponClicked(MyGUI::Widget* _sender)
    {
        const MWWorld::Ptr& player = MWMechanics::getPlayer();
        if (player.getClass().getNpcStats(player).isWerewolf())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sWerewolfRefusal}");
            return;
        }

        MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Inventory);
    }

    void HUD::onMagicClicked(MyGUI::Widget* _sender)
    {
        const MWWorld::Ptr& player = MWMechanics::getPlayer();
        if (player.getClass().getNpcStats(player).isWerewolf())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sWerewolfRefusal}");
            return;
        }

        MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Magic);
    }

    void HUD::setCellName(const std::string& cellName)
    {
        if (mCellName != cellName)
        {
            mCellNameTimer = 5.0f;
            mCellName = cellName;

            mCellNameBox->setCaptionWithReplacing("#{sCell=" + mCellName + "}");
            mCellNameBox->setVisible(mMapVisible);
        }
    }

    void HUD::cacheOriginalCoord(MyGUI::Widget* widget)
    {
        if (!widget)
            return;

        widget->setUserString("FullLeft", MyGUI::utility::toString(widget->getLeft()));
        widget->setUserString("FullTop", MyGUI::utility::toString(widget->getTop()));
        widget->setUserString("FullWidth", MyGUI::utility::toString(widget->getWidth()));
        widget->setUserString("FullHeight", MyGUI::utility::toString(widget->getHeight()));
    }

    void HUD::updateCenteredBar(MyGUI::Widget* frame, MyGUI::ProgressBar* bar, float ratio, int paddingX, int paddingY, MyGUI::TextBox* text)
    {
        if (!frame || !bar)
            return;

        ratio = std::max(0.f, std::min(1.f, ratio));

        const int fullLeft = MyGUI::utility::parseInt(frame->getUserString("FullLeft"));
        const int fullTop = MyGUI::utility::parseInt(frame->getUserString("FullTop"));
        const int fullWidth = MyGUI::utility::parseInt(frame->getUserString("FullWidth"));
        const int fullHeight = MyGUI::utility::parseInt(frame->getUserString("FullHeight"));

        frame->setCoord(fullLeft, fullTop, fullWidth, fullHeight);
        frame->setVisible(true);

        const int innerAreaWidth = std::max(0, fullWidth - paddingX * 2);
        const int innerHeight = std::max(0, fullHeight - paddingY * 2);
        const int barWidth = static_cast<int>(std::lround(static_cast<double>(innerAreaWidth) * ratio));
        const int barLeft = paddingX + (innerAreaWidth - barWidth) / 2;

        bar->setCoord(barLeft, paddingY, barWidth, innerHeight);
        bar->setVisible(barWidth > 0);

        if (text)
        {
            text->setCoord(0, 0, fullWidth, fullHeight);
            text->setVisible(true);
        }
    }

    float HUD::getVisibilityTimeout(int current, int modified) const
    {
        return (modified > 0 && current >= modified) ? 7.f : 30.f;
    }

    void HUD::updatePingPosition()
    {
        if (!mFpsBox || !mPingBox)
            return;

        const int gap = 10;
        mPingBox->setPosition(mFpsBox->getLeft() + mFpsBox->getTextSize().width + gap, mPingBox->getTop());
    }

    void HUD::showAutoHideWidget(MyGUI::Widget* widget, float& timer, float timeout)
    {
        timer = timeout;
        if (widget)
        {
            widget->setVisible(true);
            widget->setAlpha(1.f);
        }
    }

    void HUD::updateAutoHideWidget(MyGUI::Widget* widget, float& timer, float dt)
    {
        if (!widget || !widget->getVisible())
            return;

        timer -= dt;
        if (timer <= 0.f)
        {
            widget->setVisible(false);
            widget->setAlpha(1.f);
            return;
        }

        const float fadeDuration = 0.35f;
        if (timer < fadeDuration)
            widget->setAlpha(std::max(0.f, timer / fadeDuration));
        else
            widget->setAlpha(1.f);
    }

    void HUD::onFrame(float dt)
    {
        LocalMapBase::onFrame(dt);


        if (mGameTimeBox)
        {
            mGameTimeUpdateTimer -= dt;
            if (mGameTimeUpdateTimer <= 0.f)
            {
                const float gameHour = MWBase::Environment::get().getWorld()->getTimeStamp().getHour();
                int hours = static_cast<int>(std::floor(gameHour)) % 24;
                int minutes = static_cast<int>(std::floor((gameHour - std::floor(gameHour)) * 60.f + 0.5f));
                if (minutes >= 60)
                {
                    minutes = 0;
                    hours = (hours + 1) % 24;
                }

                std::ostringstream stream;
                stream << std::setfill('0') << std::setw(2) << hours << ':'
                       << std::setfill('0') << std::setw(2) << minutes;
                mGameTimeBox->setCaption(stream.str());

                mGameTimeUpdateTimer = 0.2f;
            }
        }

        mCellNameTimer -= dt;
        mWeaponSpellTimer -= dt;
        if (mCellNameTimer < 0)
            mCellNameBox->setVisible(false);
        if (mWeaponSpellTimer < 0)
            mWeaponSpellBox->setVisible(false);

        mFpsAccumulatedTime += dt;
        ++mFpsFrameCount;
        mFpsUpdateTimer -= dt;
        if (mFpsBox && mFpsUpdateTimer <= 0.f)
        {
            const float safeTime = std::max(0.0001f, mFpsAccumulatedTime);
            const int fps = static_cast<int>(std::lround(static_cast<double>(mFpsFrameCount) / safeTime));
            mFpsBox->setCaption(MyGUI::utility::toString(fps));
            mFpsUpdateTimer = 0.25f;
            mFpsAccumulatedTime = 0.f;
            mFpsFrameCount = 0;
            updatePingPosition();
        }

        if (mPingBox)
        {
            mPingUpdateTimer -= dt;
            if (mPingUpdateTimer <= 0.f)
            {
                int ping = -1;
                if (mwmp::Main::isInitialized() && mwmp::Main::get().getNetworking())
                    ping = mwmp::Main::get().getNetworking()->getAvgPing();

                if (ping >= 0)
                    mPingBox->setCaption(MyGUI::utility::toString(ping) + " ms");
                else
                    mPingBox->setCaption("-- ms");

                updatePingPosition();
                mPingUpdateTimer = 0.5f;
            }
        }

        updateAutoHideWidget(mHealthFrame, mHealthVisibilityTimer, dt);
        updateAutoHideWidget(mMagickaFrame, mMagickaVisibilityTimer, dt);
        updateAutoHideWidget(mFatigueFrame, mFatigueVisibilityTimer, dt);

        mEnemyHealthTimer -= dt;
        if (mEnemyHealthFrame->getVisible() && mEnemyHealthTimer < 0)
        {
            mEnemyHealthFrame->setVisible(false);
            mEnemyHealth->setVisible(false);
            mWeaponSpellBox->setPosition(mWeaponSpellBox->getPosition() + MyGUI::IntPoint(0,20));
        }

        if (mIsDrowning)
            mDrowningFlashTheta += dt * osg::PI*2;

        mSpellIcons->updateWidgets(mEffectBox, true);

        if (mEnemyActorId != -1 && mEnemyHealthFrame->getVisible())
        {
            updateEnemyHealthBar();
        }

        if (mIsDrowning)
        {
            float intensity = (cos(mDrowningFlashTheta) + 2.0f) / 3.0f;

            mDrowningFlash->setAlpha(intensity);
        }
    }

    void HUD::setSelectedSpell(const std::string& spellId, int successChancePercent)
    {
        const ESM::Spell* spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);

        std::string spellName = spell->mName;
        if (spellName != mSpellName && mSpellVisible)
        {
            mWeaponSpellTimer = 5.0f;
            mSpellName = spellName;
            mWeaponSpellBox->setCaption(mSpellName);
            mWeaponSpellBox->setVisible(true);
        }

        mSpellStatus->setProgressRange(100);
        mSpellStatus->setProgressPosition(successChancePercent);
        updateCenteredBar(mSpellStatusBorder, mSpellStatus, successChancePercent / 100.f, 2, 2);
        showAutoHideWidget(mMagickaFrame, mMagickaVisibilityTimer, std::max(30.f, mMagickaVisibilityTimer));

        mSpellBox->setUserString("ToolTipType", "Spell");
        mSpellBox->setUserString("Spell", spellId);

        // use the icon of the first effect
        const ESM::MagicEffect* effect =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(spell->mEffects.mList.front().mEffectID);

        std::string icon = effect->mIcon;
        int slashPos = icon.rfind('\\');
        icon.insert(slashPos+1, "b_");
        icon = MWBase::Environment::get().getWindowManager()->correctIconPath(icon);

        mSpellImage->setSpellIcon(icon);
    }

    void HUD::setSelectedEnchantItem(const MWWorld::Ptr& item, int chargePercent)
    {
        std::string itemName = item.getClass().getName(item);
        if (itemName != mSpellName && mSpellVisible)
        {
            mWeaponSpellTimer = 5.0f;
            mSpellName = itemName;
            mWeaponSpellBox->setCaption(mSpellName);
            mWeaponSpellBox->setVisible(true);
        }

        mSpellStatus->setProgressRange(100);
        mSpellStatus->setProgressPosition(chargePercent);
        updateCenteredBar(mSpellStatusBorder, mSpellStatus, chargePercent / 100.f, 2, 2);
        showAutoHideWidget(mMagickaFrame, mMagickaVisibilityTimer, std::max(30.f, mMagickaVisibilityTimer));

        mSpellBox->setUserString("ToolTipType", "ItemPtr");
        mSpellBox->setUserData(MWWorld::Ptr(item));

        mSpellImage->setItem(item);
    }

    void HUD::setSelectedWeapon(const MWWorld::Ptr& item, int durabilityPercent)
    {
        std::string itemName = item.getClass().getName(item);
        if (itemName != mWeaponName && mWeaponVisible)
        {
            mWeaponSpellTimer = 5.0f;
            mWeaponName = itemName;
            mWeaponSpellBox->setCaption(mWeaponName);
            mWeaponSpellBox->setVisible(true);
        }

        mWeapBox->clearUserStrings();
        mWeapBox->setUserString("ToolTipType", "ItemPtr");
        mWeapBox->setUserData(MWWorld::Ptr(item));

        mWeapStatus->setProgressRange(100);
        mWeapStatus->setProgressPosition(durabilityPercent);
        updateCenteredBar(mWeapStatusBorder, mWeapStatus, durabilityPercent / 100.f, 2, 2);
        showAutoHideWidget(mFatigueFrame, mFatigueVisibilityTimer, std::max(30.f, mFatigueVisibilityTimer));

        mWeapImage->setItem(item);
    }

    void HUD::unsetSelectedSpell()
    {
        std::string spellName = "#{sNone}";
        if (spellName != mSpellName && mSpellVisible)
        {
            mWeaponSpellTimer = 5.0f;
            mSpellName = spellName;
            mWeaponSpellBox->setCaptionWithReplacing(mSpellName);
            mWeaponSpellBox->setVisible(true);
        }

        mSpellStatus->setProgressRange(100);
        mSpellStatus->setProgressPosition(0);
        updateCenteredBar(mSpellStatusBorder, mSpellStatus, 0.f, 2, 2);
        showAutoHideWidget(mMagickaFrame, mMagickaVisibilityTimer, std::max(30.f, mMagickaVisibilityTimer));
        mSpellImage->setItem(MWWorld::Ptr());
        mSpellBox->clearUserStrings();
    }

    void HUD::unsetSelectedWeapon()
    {
        std::string itemName = "#{sSkillHandtohand}";
        if (itemName != mWeaponName && mWeaponVisible)
        {
            mWeaponSpellTimer = 5.0f;
            mWeaponName = itemName;
            mWeaponSpellBox->setCaptionWithReplacing(mWeaponName);
            mWeaponSpellBox->setVisible(true);
        }

        mWeapStatus->setProgressRange(100);
        mWeapStatus->setProgressPosition(0);
        updateCenteredBar(mWeapStatusBorder, mWeapStatus, 0.f, 2, 2);
        showAutoHideWidget(mFatigueFrame, mFatigueVisibilityTimer, std::max(30.f, mFatigueVisibilityTimer));

        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();

        mWeapImage->setItem(MWWorld::Ptr());
        std::string icon = (player.getClass().getNpcStats(player).isWerewolf()) ? "icons\\k\\tx_werewolf_hand.dds" : "icons\\k\\stealth_handtohand.dds";
        mWeapImage->setIcon(icon);

        mWeapBox->clearUserStrings();
        mWeapBox->setUserString("ToolTipType", "Layout");
        mWeapBox->setUserString("ToolTipLayout", "HandToHandToolTip");
        mWeapBox->setUserString("Caption_HandToHandText", itemName);
        mWeapBox->setUserString("ImageTexture_HandToHandImage", icon);
    }

    void HUD::setCrosshairVisible(bool visible)
    {
        mCrosshair->setVisible (visible);
    }
    
    void HUD::setCrosshairOwned(bool owned)
    {
        if(owned)
        {
            mCrosshair->changeWidgetSkin("HUD_Crosshair_Owned");
        }
        else
        {
            mCrosshair->changeWidgetSkin("HUD_Crosshair");
        }
    }
    
    void HUD::setHmsVisible(bool visible)
    {
        if (!visible)
        {
            mHealthFrame->setVisible(false);
            mMagickaFrame->setVisible(false);
            mFatigueFrame->setVisible(false);
        }
        else
        {
            showAutoHideWidget(mHealthFrame, mHealthVisibilityTimer, getVisibilityTimeout(mHealth->getProgressPosition(), mHealth->getProgressRange()));
            showAutoHideWidget(mMagickaFrame, mMagickaVisibilityTimer, getVisibilityTimeout(mMagicka->getProgressPosition(), mMagicka->getProgressRange()));
            showAutoHideWidget(mFatigueFrame, mFatigueVisibilityTimer, getVisibilityTimeout(mStamina->getProgressPosition(), mStamina->getProgressRange()));

            const float healthRatio = mHealth->getProgressRange() > 0
                ? static_cast<float>(mHealth->getProgressPosition()) / static_cast<float>(mHealth->getProgressRange())
                : 0.f;
            const float magickaRatio = mMagicka->getProgressRange() > 0
                ? static_cast<float>(mMagicka->getProgressPosition()) / static_cast<float>(mMagicka->getProgressRange())
                : 0.f;
            const float fatigueRatio = mStamina->getProgressRange() > 0
                ? static_cast<float>(mStamina->getProgressPosition()) / static_cast<float>(mStamina->getProgressRange())
                : 0.f;

            updateCenteredBar(mHealthBorder, mHealth, healthRatio, 2, 2, mHealthText);
            updateCenteredBar(mMagickaBorder, mMagicka, magickaRatio, 2, 2, mMagickaText);
            updateCenteredBar(mFatigueBorder, mStamina, fatigueRatio, 2, 2, mStaminaText);
        }
        updatePositions();
    }

    void HUD::setWeapVisible(bool visible)
    {
        mWeapBox->setVisible(visible);
        updatePositions();
    }

    void HUD::setSpellVisible(bool visible)
    {
        mSpellBox->setVisible(visible);
        updatePositions();
    }

    void HUD::setSneakVisible(bool visible)
    {
        mSneakBox->setVisible(visible);
        updatePositions();
    }

    void HUD::setEffectVisible(bool visible)
    {
        mEffectBox->setVisible (visible);
        updatePositions();
    }

    void HUD::setMinimapVisible(bool visible)
    {
        mMinimapBox->setVisible (visible);
        updatePositions();
    }

    void HUD::updatePositions()
    {
        int weapDx = 0, spellDx = 0, sneakDx = 0;
        if (!mHealthFrame->getVisible() && !mMagickaFrame->getVisible() && !mFatigueFrame->getVisible())
            sneakDx = spellDx = weapDx = mWeapBoxBaseLeft - mHealthManaStaminaBaseLeft;

        if (!mWeapBox->getVisible())
        {
            spellDx += mSpellBoxBaseLeft - mWeapBoxBaseLeft;
            sneakDx = spellDx;
        }

        if (!mSpellBox->getVisible())
            sneakDx += mSneakBoxBaseLeft - mSpellBoxBaseLeft;

        mWeaponVisible = mWeapBox->getVisible();
        mSpellVisible = mSpellBox->getVisible();
        if (!mWeaponVisible && !mSpellVisible)
            mWeaponSpellBox->setVisible(false);

        mWeapBox->setPosition(mWeapBoxBaseLeft - weapDx, mWeapBox->getTop());
        mSpellBox->setPosition(mSpellBoxBaseLeft - spellDx, mSpellBox->getTop());
        mSneakBox->setPosition(mSneakBoxBaseLeft - sneakDx, mSneakBox->getTop());

        const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

        // effect box can have variable width -> variable left coordinate
        int effectsDx = 0;
        if (!mMinimapBox->getVisible ())
            effectsDx = mEffectBoxBaseRight - mMinimapBoxBaseRight;

        mMapVisible = mMinimapBox->getVisible ();
        if (!mMapVisible)
            mCellNameBox->setVisible(false);

        mEffectBox->setPosition((viewSize.width - mEffectBoxBaseRight) - mEffectBox->getWidth() + effectsDx, mEffectBox->getTop());
    }

    void HUD::updateEnemyHealthBar()
    {
        MWWorld::Ptr enemy = MWBase::Environment::get().getWorld()->searchPtrViaActorId(mEnemyActorId);
        if (enemy.isEmpty())
            return;
        MWMechanics::CreatureStats& stats = enemy.getClass().getCreatureStats(enemy);
        mEnemyHealth->setProgressRange(100);
        // Health is usually cast to int before displaying. Actors die whenever they are < 1 health.
        // Therefore any value < 1 should show as an empty health bar. We do the same in statswindow :)
        const float healthRatio = stats.getHealth().getModified() > 0.f
            ? std::max(0.f, std::min(1.f, stats.getHealth().getCurrent() / stats.getHealth().getModified()))
            : 0.f;
        mEnemyHealth->setProgressPosition(static_cast<size_t>(healthRatio * 100.f));
        updateCenteredBar(mEnemyHealthBorder, mEnemyHealth, healthRatio, 2, 2);

        static const float fNPCHealthBarFade = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fNPCHealthBarFade")->mValue.getFloat();
        if (fNPCHealthBarFade > 0.f)
            mEnemyHealthFrame->setAlpha(std::max(0.f, std::min(1.f, mEnemyHealthTimer/fNPCHealthBarFade)));
        else
            mEnemyHealthFrame->setAlpha(1.f);

    }

    void HUD::setEnemy(const MWWorld::Ptr &enemy)
    {
        mEnemyActorId = enemy.getClass().getCreatureStats(enemy).getActorId();
        mEnemyHealthTimer = 30.f;
        if (!mEnemyHealthFrame->getVisible())
            mWeaponSpellBox->setPosition(mWeaponSpellBox->getPosition() - MyGUI::IntPoint(0,20));
        mEnemyHealthFrame->setVisible(true);
        mEnemyHealth->setVisible(true);
        updateEnemyHealthBar();
    }

    void HUD::resetEnemy()
    {
        mEnemyActorId = -1;
        mEnemyHealthTimer = -1;
        if (mEnemyHealthFrame)
            mEnemyHealthFrame->setVisible(false);
        if (mEnemyHealth)
            mEnemyHealth->setVisible(false);
    }

    void HUD::clear()
    {
        unsetSelectedSpell();
        unsetSelectedWeapon();
        resetEnemy();
    }

    void HUD::customMarkerCreated(MyGUI::Widget *marker)
    {
        marker->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onMapClicked);
    }

    void HUD::doorMarkerCreated(MyGUI::Widget *marker)
    {
        marker->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onMapClicked);
    }

}
