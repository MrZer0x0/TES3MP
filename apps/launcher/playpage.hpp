#ifndef PLAYPAGE_H
#define PLAYPAGE_H

#include "ui_playpage.h"

#include <QString>
#include <QMap>

class QWidget;
class QButtonGroup;

namespace Launcher
{
    // -------------------------------------------------------------------------
    // Graphics preset descriptor
    // -------------------------------------------------------------------------
    struct GraphicsPreset
    {
        QString name;

        // Rendering
        int     viewDistance       = 6000;
        QString textureFilter      = "trilinear";
        int     anisotropy         = 4;

        // Shadows
        bool    shadows            = false;
        int     shadowMapSize      = 1024;
        bool    actorShadows       = false;
        bool    terrainShadows     = false;

        // Water / sky
        bool    waterShader        = false;
        int     waterReflection    = 0;
        bool    fog                = true;
        bool    radialFog          = false;

        // Lighting
        QString lightingMethod     = "legacy";
        int     maxLights          = 8;
        bool    bloom              = false;
        bool    sunlightScattering = false;
    };

    // -------------------------------------------------------------------------
    class PlayPage : public QWidget, private Ui::PlayPage
    {
        Q_OBJECT

    public:
        explicit PlayPage(QWidget *parent = nullptr);

        void setServerAddress(const QString& addr);
        void setServerPort(const QString& port);
        void setServerConsoleWidget(QWidget* widget);

        QString serverAddress() const;
        QString serverPort() const;

        void switchToServerConsoleTab();
        void loadServerSettings();
        bool saveServerSettings();

        // Graphics presets API
        void loadGraphicsPresets();
        void applyPresetToUI(const GraphicsPreset& preset);
        GraphicsPreset collectUIPreset(const QString& name = QString()) const;
        void applyPresetToSettingsCfg(const GraphicsPreset& preset);
        void loadSettingsCfgIntoUI();

    signals:
        void playButtonClicked();
        void serverButtonClicked();

    private slots:
        void slotPlayClicked();
        void slotServerClicked();
        void slotReloadServerSettings();
        void slotSaveServerSettings();
        void slotApplyFormToRawConfig();
        void slotSyncFormFromRawConfig();

        // Graphics preset slots
        void slotPresetLow();
        void slotPresetMedium();
        void slotPresetHigh();
        void slotPresetUltra();
        void slotApplyGraphicsPreset();
        void slotSaveGraphicsPreset();
        void slotViewDistanceChanged(int value);

    private:
        QString serverConfigPath() const;
        QString replaceRawValue(const QString& text, const QString& key, const QString& value) const;
        QString updatedConfigFromForm(const QString& input) const;
        void populateFormFromConfig(const QString& text);
        void setServerSettingsStatus(const QString& text, bool isError = false);

        QString settingsCfgPath() const;
        QString readSettingsCfgKey(const QString& section, const QString& key) const;
        void    writeSettingsCfgKey(const QString& section, const QString& key, const QString& value);
        void    updatePresetButtonStates(int activeIndex);
        int     detectCurrentPreset() const;
        void    setGraphicsPresetStatus(const QString& msg);

        static GraphicsPreset makeLowPreset();
        static GraphicsPreset makeMediumPreset();
        static GraphicsPreset makeHighPreset();
        static GraphicsPreset makeUltraPreset();

        QWidget* mEmbeddedServerConsole;
        int mCurrentPresetIndex = -1;

        // Staged preset waiting for Apply
        GraphicsPreset mStagedPreset;
        bool mHasStagedPreset = false;
    };
}
#endif
