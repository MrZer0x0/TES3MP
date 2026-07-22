#include "quickloot.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>

#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Widget.h>

#include <components/settings/settings.hpp>
#include <components/widgets/box.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/actor.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/character.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/disease.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwinput/sdlmappings.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actiontrap.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/player.hpp"

#include "containeritemmodel.hpp"
#include "inventoryitemmodel.hpp"
#include "inventorywindow.hpp"
#include "itemmodel.hpp"
#include "pickpocketitemmodel.hpp"

namespace MWGui
{
    QuickLoot::QuickLoot()
        : Layout("openmw_quickloot.layout")
        , mQuickLoot(nullptr)
        , mModel(nullptr)
        , mLabel(nullptr)
        , mRows{{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}}
        , mSortModel(nullptr)
        , mOpened(false)
        , mShouldOpen(false)
        , mHidden(true)
        , mPlaying(false)
        , mFocusToolTipX(0.f)
        , mFocusToolTipY(0.f)
        , mDelay(0.f)
        , mRemainingDelay(0.f)
        , mLastMouseX(0)
        , mLastMouseY(0)
        , mEnabled(true)
        , mFrameDuration(0.f)
        , mLastIndex(0)
        , mVisibleStart(0)
    {
        getWidget(mQuickLoot, "QuickLoot");
        getWidget(mLabel, "Label");
        for (int i = 0; i < sVisibleRows; ++i)
            getWidget(mRows[static_cast<std::size_t>(i)], "Row" + std::to_string(i));

        setVisibleAll(false);

        mQuickLoot->eventKeyButtonPressed += MyGUI::newDelegate(this, &QuickLoot::onKeyButtonPressed);
        mQuickLoot->setNeedKeyFocus(true);
        mQuickLoot->setNeedMouseFocus(false);
        mMainWidget->setNeedMouseFocus(false);
        mMainWidget->setNeedKeyFocus(false);

        // Use the same appearance delay as ordinary tooltips.
        mDelay = Settings::Manager::getFloat("tooltip delay", "GUI");
        mRemainingDelay = mDelay;
    }

    QuickLoot::~QuickLoot()
    {
        clearModels();
    }

    void QuickLoot::clearModels()
    {
        if (mSortModel)
            delete mSortModel; // SortFilterItemModel owns mModel.
        else
            delete mModel;

        mSortModel = nullptr;
        mModel = nullptr;
    }

    int QuickLoot::getEntryCount() const
    {
        const std::size_t itemCount = mSortModel ? mSortModel->getItemCount() : 0;
        const std::size_t capped = std::min<std::size_t>(
            itemCount, static_cast<std::size_t>(std::numeric_limits<int>::max() - 1));
        return static_cast<int>(capped) + 1;
    }

    void QuickLoot::refreshRows()
    {
        const int total = getEntryCount();
        mLastIndex = std::max(0, std::min(mLastIndex, total - 1));

        const int maxStart = std::max(0, total - sVisibleRows);
        if (mLastIndex < mVisibleStart)
            mVisibleStart = mLastIndex;
        else if (mLastIndex >= mVisibleStart + sVisibleRows)
            mVisibleStart = mLastIndex - sVisibleRows + 1;
        mVisibleStart = std::max(0, std::min(mVisibleStart, maxStart));

        for (int rowIndex = 0; rowIndex < sVisibleRows; ++rowIndex)
        {
            MyGUI::TextBox* row = mRows[static_cast<std::size_t>(rowIndex)];
            const int entryIndex = mVisibleStart + rowIndex;
            if (entryIndex >= total)
            {
                row->setCaption("");
                row->setVisible(false);
                continue;
            }

            std::string caption;
            if (entryIndex == 0)
            {
                caption = "^  Open regular inventory";
            }
            else if (mSortModel)
            {
                const ItemStack item = mSortModel->getItem(entryIndex - 1);
                caption = item.mBase.getClass().getName(item.mBase);
                if (caption.empty())
                    caption = "Item";
                if (item.mCount > 1)
                    caption += "  x" + std::to_string(item.mCount);
            }

            const bool selected = entryIndex == mLastIndex;
            row->changeWidgetSkin(selected ? "SandBrightText" : "SandText");
            row->setAlpha(selected ? 1.f : 0.72f);
            row->setCaption((selected ? "> " : "  ") + caption);
            row->setVisible(true);
        }
    }

    bool QuickLoot::handleMouseWheel(int rel)
    {
        if (!isVisible() || rel == 0)
            return false;

        const int total = getEntryCount();
        if (total <= 0)
            return false;

        // One SDL wheel event always advances exactly one row, even on high-resolution wheels.
        if (rel < 0)
            mLastIndex = (mLastIndex + 1) % total;
        else
            mLastIndex = (mLastIndex + total - 1) % total;

        refreshRows();
        return true;
    }

    bool QuickLoot::checkOwned()
    {
        if (mFocusObject.isEmpty())
            return false;

        MWWorld::Ptr ptr = MWMechanics::getPlayer();
        MWWorld::Ptr victim;

        MWBase::MechanicsManager* mm = MWBase::Environment::get().getMechanicsManager();
        return !mm->isAllowedToUse(ptr, mFocusObject, victim);
    }

    void QuickLoot::openStandardContainer()
    {
        if (mFocusObject.isEmpty())
            return;

        mOpened = true;
        setVisibleAll(false);
        MWBase::Environment::get().getWorld()->getPlayer().activate();
    }

    bool QuickLoot::activateSelected()
    {
        if (!isVisible() || !mModel || !mSortModel)
            return false;

        if (mLastIndex == 0)
        {
            openStandardContainer();
            return true;
        }

        const int itemIndex = mLastIndex - 1;
        if (itemIndex < 0 || itemIndex >= static_cast<int>(mSortModel->getItemCount()))
            return false;

        onItemSelected(itemIndex);
        return true;
    }

    void QuickLoot::onItemSelected(int index)
    {
        if (!mModel || !mSortModel || index < 0
            || index >= static_cast<int>(mSortModel->getItemCount()))
            return;

        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return;

        const ItemModel::ModelIndex sourceIndex = mSortModel->mapToSource(index);
        if (sourceIndex < 0 || sourceIndex >= static_cast<int>(mModel->getItemCount()))
            return;

        mOpened = true;
        ensureTrapTriggered();
        MWMechanics::diseaseContact(MWMechanics::getPlayer(), mFocusObject);

        // Keep a copy: moving the item may invalidate references owned by the model.
        const ItemStack item = mModel->getItem(sourceIndex);

        // Activate takes the complete stack (gold, potions, ingredients, arrows, etc.).
        const int count = static_cast<int>(std::min<std::size_t>(
            item.mCount, static_cast<std::size_t>(std::numeric_limits<int>::max())));

        if (!mModel->onTakeItem(item.mBase, count))
            return;

        ItemModel* playerModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getModel();
        mModel->update();
        MWWorld::Ptr movedItem = mModel->moveItem(item, count, playerModel);
        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->updateItemView();

        if (MyGUI::InputManager::getInstance().isControlPressed())
            MWBase::Environment::get().getWindowManager()->getInventoryWindow()->useItem(movedItem);
        else
        {
            const std::string sound = item.mBase.getClass().getUpSoundId(item.mBase);
            MWBase::Environment::get().getWindowManager()->playSound(sound);
        }

        mModel->update();
        mSortModel->update();
        mLastIndex = std::min(mLastIndex, getEntryCount() - 1);
        refreshRows();
    }

    void QuickLoot::ensureTrapTriggered()
    {
        if (mFocusObject.isEmpty() || mFocusObject.getTypeName() != typeid(ESM::Container).name())
            return;

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::InventoryStore& invStore = player.getClass().getInventoryStore(player);

        const bool isTrapped = !mFocusObject.getCellRef().getTrap().empty();
        bool hasKey = false;
        std::string keyName;

        static const std::string trapActivationSound = "Disarm Trap Fail";

        // Necessary since having the key will always deactivate the trap.
        const std::string keyId = mFocusObject.getCellRef().getKey();
        if (!keyId.empty())
        {
            MWWorld::Ptr keyPtr = invStore.search(keyId);
            if (!keyPtr.isEmpty())
            {
                hasKey = true;
                keyName = keyPtr.getClass().getName(keyPtr);
            }
        }

        if (isTrapped && hasKey)
        {
            MWBase::Environment::get().getWindowManager()->messageBox(keyName + " #{sKeyUsed}");
            mFocusObject.getCellRef().setTrap("");
            MWBase::Environment::get().getSoundManager()->playSound3D(
                mFocusObject, "Disarm Trap", 1.f, 1.f);
        }
        else if (isTrapped)
        {
            std::shared_ptr<MWWorld::Action> action(
                new MWWorld::ActionTrap(mFocusObject.getCellRef().getTrap(), mFocusObject));
            action->setSound(trapActivationSound);
            action->execute(player);
        }
    }

    void QuickLoot::onKeyButtonPressed(MyGUI::Widget*, MyGUI::KeyCode key, MyGUI::Char)
    {
        const SDL_Keycode takeAllKey = SDL_GetKeyFromName(
            Settings::Manager::getString("key quickloot takeall", "MorroUI").c_str());
        const MyGUI::KeyCode takeAll = MWInput::sdlKeyToMyGUI(takeAllKey);

        if (key == MyGUI::KeyCode::W || key == MyGUI::KeyCode::ArrowUp)
        {
            handleMouseWheel(1);
            return;
        }
        if (key == MyGUI::KeyCode::S || key == MyGUI::KeyCode::ArrowDown)
        {
            handleMouseWheel(-1);
            return;
        }
        if (key == MyGUI::KeyCode::Return)
        {
            activateSelected();
            return;
        }

        // D intentionally has no QuickLoot action.
        if (static_cast<int>(key.getValue()) != static_cast<int>(takeAll.getValue()))
            return;

        if (!mModel || !mSortModel)
            return;

        mOpened = true;
        ensureTrapTriggered();
        MWMechanics::diseaseContact(MWMechanics::getPlayer(), mFocusObject);

        ItemModel* playerModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getModel();
        mModel->update();

        // Unequip all source items first to avoid unequipping/reequipping while transferring.
        if (mFocusObject.getClass().hasInventoryStore(mFocusObject))
        {
            MWWorld::InventoryStore& invStore = mFocusObject.getClass().getInventoryStore(mFocusObject);
            for (std::size_t i = 0; i < mModel->getItemCount(); ++i)
            {
                const ItemStack item = mModel->getItem(static_cast<int>(i));
                if (invStore.isEquipped(item.mBase))
                    invStore.unequipItem(item.mBase, mFocusObject);
            }
        }

        mModel->update();
        const std::size_t itemCount = mModel->getItemCount();
        for (std::size_t i = 0; i < itemCount; ++i)
        {
            const ItemStack item = mModel->getItem(static_cast<int>(i));
            if (i == 0)
            {
                const std::string sound = item.mBase.getClass().getUpSoundId(item.mBase);
                MWBase::Environment::get().getWindowManager()->playSound(sound);
            }

            const int count = static_cast<int>(std::min<std::size_t>(
                item.mCount, static_cast<std::size_t>(std::numeric_limits<int>::max())));
            if (!mModel->onTakeItem(item.mBase, count))
                break;
            mModel->moveItem(item, count, playerModel);
        }

        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->updateItemView();
        mModel->update();
        mSortModel->update();
        mLastIndex = 0;
        mVisibleStart = 0;
        refreshRows();
    }

    void QuickLoot::setEnabled(bool enabled)
    {
        mEnabled = enabled;
        if (!mEnabled)
        {
            clearModels();
            setVisibleAll(false);
        }
    }

    void QuickLoot::onFrame(float frameDuration)
    {
        mFrameDuration = frameDuration;
    }

    void QuickLoot::setVisibleAll(bool visible)
    {
        mHidden = !visible;
        if (visible && mOpened && mShouldOpen)
        {
            playOpenAnimation();
            mShouldOpen = false;
        }

        if (visible)
            resize();

        mMainWidget->setVisible(visible);
        for (int i = 0; i < mMainWidget->getChildCount(); ++i)
            mMainWidget->getChildAt(i)->setVisible(visible);

        if (!visible && MyGUI::InputManager::getInstance().getKeyFocusWidget() == mQuickLoot)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(nullptr);
    }

    void QuickLoot::update(float)
    {
        if (!mEnabled)
            return;

        MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
        const bool guiMode = winMgr->isGuiMode();
        const bool inCombat = MWBase::Environment::get().getWorld()->getPlayer().isInCombat();

        if (guiMode || mFocusObject.isEmpty() || mFocusObject.getCellRef().getLockLevel() > 0)
        {
            clearModels();
            setVisibleAll(false);
            return;
        }

        clearModels();

        const bool loot = mFocusObject.getClass().isActor()
            && mFocusObject.getClass().getCreatureStats(mFocusObject).isDead();
        const bool sneaking = MWBase::Environment::get().getMechanicsManager()->isSneaking(
            MWMechanics::getPlayer());

        mQuickLoot->getParent()->changeWidgetSkin(checkOwned() ? "HUD_Box_Owned" : "HUD_Box");

        if (mFocusObject.getClass().hasInventoryStore(mFocusObject))
        {
            if (mFocusObject.getClass().isNpc() && !loot && sneaking && !inCombat)
            {
                mModel = new PickpocketItemModel(mFocusObject, new InventoryItemModel(mFocusObject),
                    !mFocusObject.getClass().getCreatureStats(mFocusObject).getKnockedDown());
            }
            else if (loot)
                mModel = new InventoryItemModel(mFocusObject);
        }
        else
            mModel = new ContainerItemModel(mFocusObject);

        if (!mModel)
        {
            setVisibleAll(false);
            return;
        }

        mSortModel = new SortFilterItemModel(mModel);
        mSortModel->setCategory(SortFilterItemModel::Category_Simple);
        mSortModel->update();

        mLastIndex = 0;
        mVisibleStart = 0;
        mLabel->setCaption(mFocusObject.getClass().getName(mFocusObject));
        refreshRows();
        setVisibleAll(true);
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mQuickLoot);
    }

    void QuickLoot::position(MyGUI::IntPoint& position, MyGUI::IntSize size, MyGUI::IntSize viewportSize)
    {
        position += MyGUI::IntPoint(0, 32)
            - MyGUI::IntPoint(static_cast<int>(
                MyGUI::InputManager::getInstance().getMousePosition().left
                / static_cast<float>(viewportSize.width) * size.width), 0);

        if (position.left + size.width > viewportSize.width)
            position.left = viewportSize.width - size.width;
        if (position.top + size.height > viewportSize.height)
            position.top = MyGUI::InputManager::getInstance().getMousePosition().top - size.height - 8;
    }

    void QuickLoot::playOpenAnimation()
    {
        if (mFocusObject.isEmpty() || mFocusObject.getTypeName() != typeid(ESM::Container).name())
            return;
        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(mFocusObject);

        if (!anim || !anim->hasAnimation("containeropen") || anim->isPlaying("containeropen")
            || anim->isPlaying("containerclose"))
            return;

        mPlaying = true;
        anim->play("containeropen", MWMechanics::Priority_Persistent, MWRender::Animation::BlendMask_All,
            false, 1.f, "start", "stop", 0.f, 0);
    }

    void QuickLoot::playCloseAnimation() const
    {
        if (mFocusObject.isEmpty() || mFocusObject.getTypeName() != typeid(ESM::Container).name())
            return;

        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(mFocusObject);
        if (!anim || !anim->hasAnimation("containerclose"))
            return;

        float complete = 0.f;
        float startPoint = 0.f;
        if (anim->getInfo("containeropen", &complete))
            startPoint = 1.f - complete;

        anim->play("containerclose", MWMechanics::Priority_Persistent, MWRender::Animation::BlendMask_All,
            false, 1.f, "start", "stop", startPoint, 0);
    }

    void QuickLoot::resize()
    {
        const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        const int rows = std::max(1, std::min(sVisibleRows, getEntryCount()));
        const MyGUI::IntSize tooltipSize(360, 44 + rows * 22 + 8);
        setCoord(viewSize.width * 7 / 10 - tooltipSize.width / 2,
            viewSize.height * 6 / 10 - tooltipSize.height / 2,
            tooltipSize.width, tooltipSize.height);
    }

    void QuickLoot::clear()
    {
        mFocusObject = MWWorld::Ptr();
        mLastFocusObject = MWWorld::Ptr();
        clearModels();
        setVisibleAll(false);
    }

    void QuickLoot::setFocusObject(const MWWorld::Ptr& focus)
    {
        // The in-game GUI switch is now the single source of truth for the overlay.
        const bool quickLootEnabled = Settings::Manager::getBool("quick loot", "GUI");

        if (!mEnabled || !quickLootEnabled)
        {
            mLastFocusObject = mFocusObject;
            mFocusObject = focus;
            clearModels();
            setVisibleAll(false);
            return;
        }

        const MWWorld::Ptr player = MWMechanics::getPlayer();
        const bool werewolf = player.getClass().getNpcStats(player).isWerewolf();
        const bool incapacitated = player.getClass().getCreatureStats(player).isParalyzed()
            || player.getClass().getCreatureStats(player).getKnockedDown();

        if (focus.isEmpty() || MWBase::Environment::get().getWindowManager()->isGuiMode() || werewolf
            || incapacitated || (focus.getTypeName() != typeid(ESM::Container).name()
                && !focus.getClass().hasInventoryStore(focus)))
        {
            mLastFocusObject = mFocusObject;
            mFocusObject = focus;
            clearModels();
            setVisibleAll(false);
            return;
        }

        if (focus != mFocusObject)
        {
            if (mOpened && !mShouldOpen)
                playCloseAnimation();
            mOpened = false;
            mShouldOpen = true;
        }

        mLastFocusObject = mFocusObject;
        mFocusObject = focus;

        const bool combat = MWBase::Environment::get().getWorld()->getPlayer().isInCombat();
        const bool loot = mFocusObject.getClass().isActor()
            && mFocusObject.getClass().getCreatureStats(mFocusObject).isDead()
            && mFocusObject.getClass().getCreatureStats(mFocusObject).isDeathAnimationFinished();
        const bool sneaking = MWBase::Environment::get().getMechanicsManager()->isSneaking(player);

        bool hide = false;
        if (mFocusObject.getClass().hasInventoryStore(mFocusObject) && mFocusObject.getClass().isNpc())
        {
            if ((!loot && !sneaking) || (!loot && sneaking && combat))
                hide = true;
        }

        if (mLastFocusObject == mFocusObject && !hide && mSortModel)
        {
            mSortModel->update();
            mLastIndex = std::min(mLastIndex, getEntryCount() - 1);
            refreshRows();
            setVisibleAll(true);
            if (MyGUI::InputManager::getInstance().getKeyFocusWidget() == nullptr)
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mQuickLoot);
            return;
        }

        setVisibleAll(false);
        clearModels();
        if (!hide)
            update(mFrameDuration);
    }

    void QuickLoot::setFocusObjectScreenCoords(float min_x, float min_y, float max_x, float max_y)
    {
        mFocusToolTipX = (min_x + max_x) / 2;
        mFocusToolTipY = min_y;
    }

    void QuickLoot::setDelay(float delay)
    {
        mDelay = delay;
        mRemainingDelay = mDelay;
    }
}
