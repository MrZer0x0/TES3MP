#include "playpage.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QTextStream>
#include <QVBoxLayout>

// ============================================================================
// Local helpers for config.lua parsing
// ============================================================================
namespace
{
    bool findConfigAssignment(const QString& text, const QString& key, QString* value)
    {
        const QRegularExpression pattern(QStringLiteral("(^|\\n)\\s*config\\.%1\\s*=\\s*([^\\r\\n]+)").arg(QRegularExpression::escape(key)));
        const QRegularExpressionMatch match = pattern.match(text);
        if (!match.hasMatch())
            return false;
        if (value != nullptr)
            *value = match.captured(2).trimmed();
        return true;
    }

    QString replaceConfigAssignment(const QString& text, const QString& key, const QString& value)
    {
        const QRegularExpression pattern(QStringLiteral("(^|\\n)(\\s*config\\.%1\\s*=\\s*)([^\\r\\n]+)").arg(QRegularExpression::escape(key)));
        QString result = text;
        result.replace(pattern, QStringLiteral("\\1\\2") + value);
        return result;
    }

    void loadLineEdit(QLineEdit* widget, const QString& text, const QString& key)
    {
        QString value;
        if (!findConfigAssignment(text, key, &value)) return;
        if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2)
            value = value.mid(1, value.size() - 2);
        widget->setText(value);
    }

    void loadSpinBox(QSpinBox* widget, const QString& text, const QString& key)
    {
        QString value;
        if (!findConfigAssignment(text, key, &value)) return;
        bool ok = false;
        const int parsed = value.toInt(&ok);
        if (ok) widget->setValue(parsed);
    }

    void loadCheckBox(QCheckBox* widget, const QString& text, const QString& key)
    {
        QString value;
        if (!findConfigAssignment(text, key, &value)) return;
        widget->setChecked(value.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0);
    }

    void loadComboBox(QComboBox* widget, const QString& text, const QString& key)
    {
        QString value;
        if (!findConfigAssignment(text, key, &value)) return;
        if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2)
            value = value.mid(1, value.size() - 2);
        const int index = widget->findData(value);
        if (index >= 0) widget->setCurrentIndex(index);
    }
} // namespace

// ============================================================================
// Built-in presets
// ============================================================================
Launcher::GraphicsPreset Launcher::PlayPage::makeLowPreset()
{
    GraphicsPreset p;
    p.name              = QStringLiteral("Low");
    p.viewDistance      = 3000;
    p.textureFilter     = QStringLiteral("nearest");
    p.anisotropy        = 1;
    p.shadows           = false;
    p.shadowMapSize     = 512;
    p.actorShadows      = false;
    p.terrainShadows    = false;
    p.waterShader       = false;
    p.waterReflection   = 0;
    p.fog               = true;
    p.radialFog         = false;
    p.lightingMethod    = QStringLiteral("legacy");
    p.maxLights         = 8;
    p.bloom             = false;
    p.sunlightScattering= false;
    return p;
}

Launcher::GraphicsPreset Launcher::PlayPage::makeMediumPreset()
{
    GraphicsPreset p;
    p.name              = QStringLiteral("Medium");
    p.viewDistance      = 6000;
    p.textureFilter     = QStringLiteral("linear");
    p.anisotropy        = 4;
    p.shadows           = true;
    p.shadowMapSize     = 1024;
    p.actorShadows      = false;
    p.terrainShadows    = false;
    p.waterShader       = true;
    p.waterReflection   = 1;
    p.fog               = true;
    p.radialFog         = false;
    p.lightingMethod    = QStringLiteral("shaders compatibility");
    p.maxLights         = 8;
    p.bloom             = false;
    p.sunlightScattering= false;
    return p;
}

Launcher::GraphicsPreset Launcher::PlayPage::makeHighPreset()
{
    GraphicsPreset p;
    p.name              = QStringLiteral("High");
    p.viewDistance      = 12000;
    p.textureFilter     = QStringLiteral("trilinear");
    p.anisotropy        = 8;
    p.shadows           = true;
    p.shadowMapSize     = 2048;
    p.actorShadows      = true;
    p.terrainShadows    = true;
    p.waterShader       = true;
    p.waterReflection   = 2;
    p.fog               = true;
    p.radialFog         = true;
    p.lightingMethod    = QStringLiteral("shaders compatibility");
    p.maxLights         = 16;
    p.bloom             = true;
    p.sunlightScattering= false;
    return p;
}

Launcher::GraphicsPreset Launcher::PlayPage::makeUltraPreset()
{
    GraphicsPreset p;
    p.name              = QStringLiteral("Ultra");
    p.viewDistance      = 30000;
    p.textureFilter     = QStringLiteral("trilinear");
    p.anisotropy        = 16;
    p.shadows           = true;
    p.shadowMapSize     = 4096;
    p.actorShadows      = true;
    p.terrainShadows    = true;
    p.waterShader       = true;
    p.waterReflection   = 3;
    p.fog               = true;
    p.radialFog         = true;
    p.lightingMethod    = QStringLiteral("shaders");
    p.maxLights         = 64;
    p.bloom             = true;
    p.sunlightScattering= true;
    return p;
}

// ============================================================================
// Constructor
// ============================================================================
Launcher::PlayPage::PlayPage(QWidget *parent)
    : QWidget(parent)
    , mEmbeddedServerConsole(nullptr)
{
    setObjectName("PlayPage");
    setupUi(this);

    // --- Server settings connections ---
    connect(playButton,                   SIGNAL(clicked()), this, SLOT(slotPlayClicked()));
    connect(serverButton,                 SIGNAL(clicked()), this, SLOT(slotServerClicked()));
    connect(reloadServerSettingsButton,   SIGNAL(clicked()), this, SLOT(slotReloadServerSettings()));
    connect(saveServerSettingsButton,     SIGNAL(clicked()), this, SLOT(slotSaveServerSettings()));
    connect(applyServerSettingsFormButton,SIGNAL(clicked()), this, SLOT(slotApplyFormToRawConfig()));
    connect(syncServerSettingsFormButton, SIGNAL(clicked()), this, SLOT(slotSyncFormFromRawConfig()));

    // --- Graphics preset connections ---
    connect(presetLowButton,    SIGNAL(clicked()), this, SLOT(slotPresetLow()));
    connect(presetMediumButton, SIGNAL(clicked()), this, SLOT(slotPresetMedium()));
    connect(presetHighButton,   SIGNAL(clicked()), this, SLOT(slotPresetHigh()));
    connect(presetUltraButton,  SIGNAL(clicked()), this, SLOT(slotPresetUltra()));
    connect(applyGraphicsPresetButton, SIGNAL(clicked()), this, SLOT(slotApplyGraphicsPreset()));
    connect(saveGraphicsPresetButton,  SIGNAL(clicked()), this, SLOT(slotSaveGraphicsPreset()));
    connect(viewDistanceSlider, SIGNAL(valueChanged(int)), this, SLOT(slotViewDistanceChanged(int)));

    pageTabs->setCurrentIndex(0);
    serverSettingsModeTabs->setCurrentIndex(0);
    loadServerSettings();
    loadGraphicsPresets();
}

// ============================================================================
// Server address / port
// ============================================================================
void Launcher::PlayPage::setServerAddress(const QString& addr)
{
    serverAddressEdit->setText(addr);
}

void Launcher::PlayPage::setServerPort(const QString& port)
{
    serverPortEdit->setText(port);
}

void Launcher::PlayPage::setServerConsoleWidget(QWidget* widget)
{
    if (!widget || mEmbeddedServerConsole == widget) return;
    if (mEmbeddedServerConsole)
    {
        serverConsoleHostLayout->removeWidget(mEmbeddedServerConsole);
        mEmbeddedServerConsole->setParent(nullptr);
    }
    mEmbeddedServerConsole = widget;
    widget->setParent(serverConsoleHost);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    serverConsoleHostLayout->addWidget(widget);
}

QString Launcher::PlayPage::serverAddress() const
{
    const QString addr = serverAddressEdit->text().trimmed();
    return addr.isEmpty() ? QStringLiteral("localhost") : addr;
}

QString Launcher::PlayPage::serverPort() const
{
    const QString p = serverPortEdit->text().trimmed();
    return p.isEmpty() ? QStringLiteral("25565") : p;
}

void Launcher::PlayPage::switchToServerConsoleTab()
{
    pageTabs->setCurrentWidget(serverConsoleTab);
}

// ============================================================================
// Server settings – config.lua editor
// ============================================================================
QString Launcher::PlayPage::serverConfigPath() const
{
    const QDir baseDir(QApplication::applicationDirPath());
    return QDir::cleanPath(baseDir.filePath(QStringLiteral("server/scripts/config.lua")));
}

QString Launcher::PlayPage::replaceRawValue(const QString& text, const QString& key, const QString& value) const
{
    return replaceConfigAssignment(text, key, value);
}

void Launcher::PlayPage::setServerSettingsStatus(const QString& text, bool isError)
{
    serverSettingsStatusLabel->setText(text);
    serverSettingsStatusLabel->setStyleSheet(isError
        ? QStringLiteral("color: #9f1f1f; font-weight: 600;")
        : QStringLiteral("color: #245027; font-weight: 600;"));
}

void Launcher::PlayPage::populateFormFromConfig(const QString& text)
{
    loadLineEdit(gameModeEdit,   text, QStringLiteral("gameMode"));
    loadLineEdit(dataPathEdit,   text, QStringLiteral("dataPath"));

    loadSpinBox(loginTimeSpinBox,           text, QStringLiteral("loginTime"));
    loadSpinBox(maxClientsPerIPSpinBox,     text, QStringLiteral("maxClientsPerIP"));
    loadSpinBox(difficultySpinBox,          text, QStringLiteral("difficulty"));
    loadSpinBox(nightStartHourSpinBox,      text, QStringLiteral("nightStartHour"));
    loadSpinBox(nightEndHourSpinBox,        text, QStringLiteral("nightEndHour"));
    loadSpinBox(deathTimeSpinBox,           text, QStringLiteral("deathTime"));
    loadSpinBox(deathPenaltyJailDaysSpinBox,text, QStringLiteral("deathPenaltyJailDays"));
    loadSpinBox(fixmeIntervalSpinBox,       text, QStringLiteral("fixmeInterval"));
    loadSpinBox(pingDifferenceSpinBox,      text, QStringLiteral("pingDifferenceRequiredForAuthority"));
    loadSpinBox(enforcedLogLevelSpinBox,    text, QStringLiteral("enforcedLogLevel"));
    loadSpinBox(physicsFramerateSpinBox,    text, QStringLiteral("physicsFramerate"));

    loadCheckBox(passTimeWhenEmptyCheckBox,            text, QStringLiteral("passTimeWhenEmpty"));
    loadCheckBox(allowConsoleCheckBox,                 text, QStringLiteral("allowConsole"));
    loadCheckBox(allowBedRestCheckBox,                 text, QStringLiteral("allowBedRest"));
    loadCheckBox(allowWildernessRestCheckBox,          text, QStringLiteral("allowWildernessRest"));
    loadCheckBox(allowWaitCheckBox,                    text, QStringLiteral("allowWait"));
    loadCheckBox(useInstancedSpawnCheckBox,            text, QStringLiteral("useInstancedSpawn"));
    loadCheckBox(respawnAtImperialShrineCheckBox,      text, QStringLiteral("respawnAtImperialShrine"));
    loadCheckBox(respawnAtTribunalTempleCheckBox,      text, QStringLiteral("respawnAtTribunalTemple"));
    loadCheckBox(playersRespawnCheckBox,               text, QStringLiteral("playersRespawn"));
    loadCheckBox(bountyResetOnDeathCheckBox,           text, QStringLiteral("bountyResetOnDeath"));
    loadCheckBox(bountyDeathPenaltyCheckBox,           text, QStringLiteral("bountyDeathPenalty"));
    loadCheckBox(allowSuicideCommandCheckBox,          text, QStringLiteral("allowSuicideCommand"));
    loadCheckBox(allowFixmeCommandCheckBox,            text, QStringLiteral("allowFixmeCommand"));
    loadCheckBox(allowOnContainerForUnloadedCellsCheckBox, text, QStringLiteral("allowOnContainerForUnloadedCells"));
    loadCheckBox(enablePlayerCollisionCheckBox,        text, QStringLiteral("enablePlayerCollision"));
    loadCheckBox(enableActorCollisionCheckBox,         text, QStringLiteral("enableActorCollision"));
    loadCheckBox(enablePlacedObjectCollisionCheckBox,  text, QStringLiteral("enablePlacedObjectCollision"));
    loadCheckBox(useActorCollisionForPlacedObjectsCheckBox, text, QStringLiteral("useActorCollisionForPlacedObjects"));
    loadCheckBox(enforceDataFilesCheckBox,             text, QStringLiteral("enforceDataFiles"));
    loadCheckBox(ignoreScriptErrorsCheckBox,           text, QStringLiteral("ignoreScriptErrors"));

    loadCheckBox(shareJournalCheckBox,          text, QStringLiteral("shareJournal"));
    loadCheckBox(shareFactionRanksCheckBox,     text, QStringLiteral("shareFactionRanks"));
    loadCheckBox(shareFactionExpulsionCheckBox, text, QStringLiteral("shareFactionExpulsion"));
    loadCheckBox(shareFactionReputationCheckBox,text, QStringLiteral("shareFactionReputation"));
    loadCheckBox(shareTopicsCheckBox,           text, QStringLiteral("shareTopics"));
    loadCheckBox(shareBountyCheckBox,           text, QStringLiteral("shareBounty"));
    loadCheckBox(shareReputationCheckBox,       text, QStringLiteral("shareReputation"));
    loadCheckBox(shareMapExplorationCheckBox,   text, QStringLiteral("shareMapExploration"));
    loadCheckBox(shareVideosCheckBox,           text, QStringLiteral("shareVideos"));
    loadCheckBox(shareKillsCheckBox,            text, QStringLiteral("shareKills"));

    loadComboBox(databaseTypeComboBox, text, QStringLiteral("databaseType"));
}

QString Launcher::PlayPage::updatedConfigFromForm(const QString& input) const
{
    QString text = input;
    const auto replStr = [&text](const QString& key, const QString& value) {
        text = replaceConfigAssignment(text, key, QStringLiteral("\"") + value + QStringLiteral("\""));
    };
    const auto replNum = [&text](const QString& key, int value) {
        text = replaceConfigAssignment(text, key, QString::number(value));
    };
    const auto replBool = [&text](const QString& key, bool value) {
        text = replaceConfigAssignment(text, key, value ? QStringLiteral("true") : QStringLiteral("false"));
    };

    replStr(QStringLiteral("gameMode"), gameModeEdit->text().trimmed());

    const QString dataPathValue = dataPathEdit->text().trimmed();
    if (dataPathValue == QStringLiteral("tes3mp.GetDataPath()"))
        text = replaceConfigAssignment(text, QStringLiteral("dataPath"), dataPathValue);
    else
        replStr(QStringLiteral("dataPath"), dataPathValue);

    replNum(QStringLiteral("loginTime"),             loginTimeSpinBox->value());
    replNum(QStringLiteral("maxClientsPerIP"),        maxClientsPerIPSpinBox->value());
    replNum(QStringLiteral("difficulty"),             difficultySpinBox->value());
    replNum(QStringLiteral("nightStartHour"),         nightStartHourSpinBox->value());
    replNum(QStringLiteral("nightEndHour"),           nightEndHourSpinBox->value());
    replNum(QStringLiteral("deathTime"),              deathTimeSpinBox->value());
    replNum(QStringLiteral("deathPenaltyJailDays"),   deathPenaltyJailDaysSpinBox->value());
    replNum(QStringLiteral("fixmeInterval"),          fixmeIntervalSpinBox->value());
    replNum(QStringLiteral("pingDifferenceRequiredForAuthority"), pingDifferenceSpinBox->value());
    replNum(QStringLiteral("enforcedLogLevel"),       enforcedLogLevelSpinBox->value());
    replNum(QStringLiteral("physicsFramerate"),       physicsFramerateSpinBox->value());

    replBool(QStringLiteral("passTimeWhenEmpty"),     passTimeWhenEmptyCheckBox->isChecked());
    replBool(QStringLiteral("allowConsole"),          allowConsoleCheckBox->isChecked());
    replBool(QStringLiteral("allowBedRest"),          allowBedRestCheckBox->isChecked());
    replBool(QStringLiteral("allowWildernessRest"),   allowWildernessRestCheckBox->isChecked());
    replBool(QStringLiteral("allowWait"),             allowWaitCheckBox->isChecked());
    replBool(QStringLiteral("useInstancedSpawn"),     useInstancedSpawnCheckBox->isChecked());
    replBool(QStringLiteral("respawnAtImperialShrine"),respawnAtImperialShrineCheckBox->isChecked());
    replBool(QStringLiteral("respawnAtTribunalTemple"),respawnAtTribunalTempleCheckBox->isChecked());
    replBool(QStringLiteral("playersRespawn"),        playersRespawnCheckBox->isChecked());
    replBool(QStringLiteral("bountyResetOnDeath"),    bountyResetOnDeathCheckBox->isChecked());
    replBool(QStringLiteral("bountyDeathPenalty"),    bountyDeathPenaltyCheckBox->isChecked());
    replBool(QStringLiteral("allowSuicideCommand"),   allowSuicideCommandCheckBox->isChecked());
    replBool(QStringLiteral("allowFixmeCommand"),     allowFixmeCommandCheckBox->isChecked());
    replBool(QStringLiteral("allowOnContainerForUnloadedCells"), allowOnContainerForUnloadedCellsCheckBox->isChecked());
    replBool(QStringLiteral("enablePlayerCollision"), enablePlayerCollisionCheckBox->isChecked());
    replBool(QStringLiteral("enableActorCollision"),  enableActorCollisionCheckBox->isChecked());
    replBool(QStringLiteral("enablePlacedObjectCollision"), enablePlacedObjectCollisionCheckBox->isChecked());
    replBool(QStringLiteral("useActorCollisionForPlacedObjects"), useActorCollisionForPlacedObjectsCheckBox->isChecked());
    replBool(QStringLiteral("enforceDataFiles"),      enforceDataFilesCheckBox->isChecked());
    replBool(QStringLiteral("ignoreScriptErrors"),    ignoreScriptErrorsCheckBox->isChecked());

    replBool(QStringLiteral("shareJournal"),         shareJournalCheckBox->isChecked());
    replBool(QStringLiteral("shareFactionRanks"),    shareFactionRanksCheckBox->isChecked());
    replBool(QStringLiteral("shareFactionExpulsion"),shareFactionExpulsionCheckBox->isChecked());
    replBool(QStringLiteral("shareFactionReputation"),shareFactionReputationCheckBox->isChecked());
    replBool(QStringLiteral("shareTopics"),          shareTopicsCheckBox->isChecked());
    replBool(QStringLiteral("shareBounty"),          shareBountyCheckBox->isChecked());
    replBool(QStringLiteral("shareReputation"),      shareReputationCheckBox->isChecked());
    replBool(QStringLiteral("shareMapExploration"),  shareMapExplorationCheckBox->isChecked());
    replBool(QStringLiteral("shareVideos"),          shareVideosCheckBox->isChecked());
    replBool(QStringLiteral("shareKills"),           shareKillsCheckBox->isChecked());

    const QString dbType = databaseTypeComboBox->currentData().toString();
    if (!dbType.isEmpty()) replStr(QStringLiteral("databaseType"), dbType);

    return text;
}

void Launcher::PlayPage::loadServerSettings()
{
    const QString path = serverConfigPath();
    serverSettingsPathLabel->setText(QDir::toNativeSeparators(path));

    QFile file(path);
    if (!file.exists())
    {
        serverSettingsEditor->setPlainText(QString());
        setServerSettingsStatus(tr("config.lua not found"), true);
        return;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        setServerSettingsStatus(tr("Could not open config.lua"), true);
        return;
    }
    QTextStream stream(&file);
    const QString text = stream.readAll();
    serverSettingsEditor->setPlainText(text);
    populateFormFromConfig(text);
    setServerSettingsStatus(tr("Loaded config.lua"));
}

bool Launcher::PlayPage::saveServerSettings()
{
    const QString path = serverConfigPath();
    QFileInfo info(path);
    QDir dir = info.absoluteDir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
    {
        QMessageBox::warning(this, tr("Save error"), tr("Could not create directory for config.lua"));
        setServerSettingsStatus(tr("Could not create directory"), true);
        return false;
    }

    if (serverSettingsModeTabs->currentWidget() == formServerSettingsTab)
        serverSettingsEditor->setPlainText(updatedConfigFromForm(serverSettingsEditor->toPlainText()));

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QMessageBox::warning(this, tr("Save error"), tr("Could not write config.lua"));
        setServerSettingsStatus(tr("Could not write config.lua"), true);
        return false;
    }
    QTextStream stream(&file);
    stream << serverSettingsEditor->toPlainText();
    file.close();
    setServerSettingsStatus(tr("Saved config.lua"));
    return true;
}

// ============================================================================
// Server settings slots
// ============================================================================
void Launcher::PlayPage::slotPlayClicked()   { emit playButtonClicked(); }
void Launcher::PlayPage::slotServerClicked() { switchToServerConsoleTab(); emit serverButtonClicked(); }
void Launcher::PlayPage::slotReloadServerSettings() { loadServerSettings(); }
void Launcher::PlayPage::slotSaveServerSettings()   { saveServerSettings(); }

void Launcher::PlayPage::slotApplyFormToRawConfig()
{
    serverSettingsEditor->setPlainText(updatedConfigFromForm(serverSettingsEditor->toPlainText()));
    setServerSettingsStatus(tr("Form applied to raw config"));
    serverSettingsModeTabs->setCurrentWidget(rawServerSettingsTab);
}

void Launcher::PlayPage::slotSyncFormFromRawConfig()
{
    populateFormFromConfig(serverSettingsEditor->toPlainText());
    setServerSettingsStatus(tr("Form updated from raw config"));
    serverSettingsModeTabs->setCurrentWidget(formServerSettingsTab);
}

// ============================================================================
// Graphics presets – settings.cfg helpers
// ============================================================================
QString Launcher::PlayPage::settingsCfgPath() const
{
    // Standard OpenMW/TES3MP location: next to the executable
    const QDir base(QApplication::applicationDirPath());
    // Try user config dir first (standard OpenMW behaviour)
    const QString userCfg = QDir::cleanPath(
        QDir::homePath() + QStringLiteral("/Documents/my games/OpenMW/settings.cfg"));
    if (QFile::exists(userCfg))
        return userCfg;
    // Fallback: local settings.cfg
    return QDir::cleanPath(base.filePath(QStringLiteral("settings.cfg")));
}

QString Launcher::PlayPage::readSettingsCfgKey(const QString& section, const QString& key) const
{
    QFile f(settingsCfgPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    QTextStream in(&f);
    QString curSection;
    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#'))
            continue;
        if (line.startsWith('[') && line.endsWith(']'))
        {
            curSection = line.mid(1, line.size() - 2).trimmed();
            continue;
        }
        if (curSection != section) continue;
        const int eq = line.indexOf('=');
        if (eq < 0) continue;
        if (line.left(eq).trimmed() == key)
            return line.mid(eq + 1).trimmed();
    }
    return QString();
}

void Launcher::PlayPage::writeSettingsCfgKey(const QString& section, const QString& key, const QString& value)
{
    const QString path = settingsCfgPath();
    QFile f(path);
    QStringList lines;
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&f);
        while (!in.atEnd()) lines << in.readLine();
        f.close();
    }

    // Find section and key
    QString curSection;
    int sectionLine  = -1;
    int keyLine      = -1;
    int sectionEnd   = -1; // last line of the section

    for (int i = 0; i < lines.size(); ++i)
    {
        const QString t = lines[i].trimmed();
        if (t.startsWith('[') && t.endsWith(']'))
        {
            if (curSection == section) sectionEnd = i - 1;
            curSection = t.mid(1, t.size() - 2).trimmed();
            if (curSection == section) sectionLine = i;
            continue;
        }
        if (curSection != section) continue;
        const int eq = t.indexOf('=');
        if (eq < 0) continue;
        if (t.left(eq).trimmed() == key) { keyLine = i; break; }
    }
    if (curSection == section) sectionEnd = lines.size() - 1;

    const QString newLine = key + QStringLiteral(" = ") + value;

    if (keyLine >= 0)
    {
        lines[keyLine] = newLine;
    }
    else if (sectionLine >= 0)
    {
        lines.insert(sectionEnd + 1, newLine);
    }
    else
    {
        // Section not found – append
        lines << QString();
        lines << QStringLiteral("[") + section + QStringLiteral("]");
        lines << newLine;
    }

    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return;
    QTextStream out(&f);
    for (const QString& l : lines)
        out << l << '\n';
}

// ============================================================================
// Graphics presets – UI helpers
// ============================================================================
void Launcher::PlayPage::updatePresetButtonStates(int activeIndex)
{
    mCurrentPresetIndex = activeIndex;
    presetLowButton->setChecked(activeIndex == 0);
    presetMediumButton->setChecked(activeIndex == 1);
    presetHighButton->setChecked(activeIndex == 2);
    presetUltraButton->setChecked(activeIndex == 3);
}

void Launcher::PlayPage::setGraphicsPresetStatus(const QString& msg)
{
    graphicsPresetStatusLabel->setText(msg);
}

void Launcher::PlayPage::applyPresetToUI(const GraphicsPreset& p)
{
    // Rendering
    viewDistanceSlider->setValue(p.viewDistance);
    label_viewDistanceValue->setText(QString::number(p.viewDistance));

    const int tfIdx = textureFilterComboBox->findData(p.textureFilter);
    if (tfIdx >= 0) textureFilterComboBox->setCurrentIndex(tfIdx);
    anisotropySpinBox->setValue(p.anisotropy);

    // Shadows
    shadowsCheckBox->setChecked(p.shadows);
    const int srIdx = shadowResComboBox->findData(QString::number(p.shadowMapSize));
    if (srIdx >= 0) shadowResComboBox->setCurrentIndex(srIdx);
    actorShadowsCheckBox->setChecked(p.actorShadows);
    terrainShadowsCheckBox->setChecked(p.terrainShadows);

    // Water / sky
    waterShaderCheckBox->setChecked(p.waterShader);
    const int wrIdx = waterReflectionComboBox->findData(QString::number(p.waterReflection));
    if (wrIdx >= 0) waterReflectionComboBox->setCurrentIndex(wrIdx);
    fogCheckBox->setChecked(p.fog);
    radialFogCheckBox->setChecked(p.radialFog);

    // Lighting
    const int lmIdx = lightingModeComboBox->findData(p.lightingMethod);
    if (lmIdx >= 0) lightingModeComboBox->setCurrentIndex(lmIdx);
    maxLightsSpinBox->setValue(p.maxLights);
    bloomCheckBox->setChecked(p.bloom);
    sunlightScatteringCheckBox->setChecked(p.sunlightScattering);
}

Launcher::GraphicsPreset Launcher::PlayPage::collectUIPreset(const QString& name) const
{
    GraphicsPreset p;
    p.name              = name;
    p.viewDistance      = viewDistanceSlider->value();
    p.textureFilter     = textureFilterComboBox->currentData().toString();
    p.anisotropy        = anisotropySpinBox->value();
    p.shadows           = shadowsCheckBox->isChecked();
    p.shadowMapSize     = shadowResComboBox->currentData().toInt();
    p.actorShadows      = actorShadowsCheckBox->isChecked();
    p.terrainShadows    = terrainShadowsCheckBox->isChecked();
    p.waterShader       = waterShaderCheckBox->isChecked();
    p.waterReflection   = waterReflectionComboBox->currentData().toInt();
    p.fog               = fogCheckBox->isChecked();
    p.radialFog         = radialFogCheckBox->isChecked();
    p.lightingMethod    = lightingModeComboBox->currentData().toString();
    p.maxLights         = maxLightsSpinBox->value();
    p.bloom             = bloomCheckBox->isChecked();
    p.sunlightScattering= sunlightScatteringCheckBox->isChecked();
    return p;
}

void Launcher::PlayPage::applyPresetToSettingsCfg(const GraphicsPreset& p)
{
    // Rendering
    writeSettingsCfgKey(QStringLiteral("Cells"),   QStringLiteral("viewing distance"),   QString::number(p.viewDistance));
    writeSettingsCfgKey(QStringLiteral("General"), QStringLiteral("texture mag filter"),  p.textureFilter);
    writeSettingsCfgKey(QStringLiteral("General"), QStringLiteral("texture min filter"),  p.textureFilter);
    writeSettingsCfgKey(QStringLiteral("General"), QStringLiteral("anisotropy"),           QString::number(p.anisotropy));

    // Shadows
    writeSettingsCfgKey(QStringLiteral("Shadows"), QStringLiteral("enable shadows"),       p.shadows ? QStringLiteral("true") : QStringLiteral("false"));
    writeSettingsCfgKey(QStringLiteral("Shadows"), QStringLiteral("shadow map resolution"),QString::number(p.shadowMapSize));
    writeSettingsCfgKey(QStringLiteral("Shadows"), QStringLiteral("actor shadows"),        p.actorShadows ? QStringLiteral("true") : QStringLiteral("false"));
    writeSettingsCfgKey(QStringLiteral("Shadows"), QStringLiteral("terrain shadows"),      p.terrainShadows ? QStringLiteral("true") : QStringLiteral("false"));

    // Water
    writeSettingsCfgKey(QStringLiteral("Water"),   QStringLiteral("shader"),               p.waterShader ? QStringLiteral("true") : QStringLiteral("false"));
    writeSettingsCfgKey(QStringLiteral("Water"),   QStringLiteral("rtt size"),             QString::number(512 * (1 << p.waterReflection)));
    writeSettingsCfgKey(QStringLiteral("Water"),   QStringLiteral("reflect actors"),       p.waterReflection >= 2 ? QStringLiteral("true") : QStringLiteral("false"));

    // Fog
    writeSettingsCfgKey(QStringLiteral("Fog"),     QStringLiteral("use distance fog"),     p.fog ? QStringLiteral("true") : QStringLiteral("false"));
    writeSettingsCfgKey(QStringLiteral("Fog"),     QStringLiteral("radial fog"),           p.radialFog ? QStringLiteral("true") : QStringLiteral("false"));

    // Lighting
    writeSettingsCfgKey(QStringLiteral("Shaders"), QStringLiteral("lighting method"),      p.lightingMethod);
    writeSettingsCfgKey(QStringLiteral("Shaders"), QStringLiteral("maximum lights"),       QString::number(p.maxLights));

    // Post-process
    writeSettingsCfgKey(QStringLiteral("Post Processing"), QStringLiteral("enabled"),
                        (p.bloom || p.sunlightScattering) ? QStringLiteral("true") : QStringLiteral("false"));
}

// ============================================================================
// Load current settings.cfg into Graphics Presets UI and detect preset
// ============================================================================
void Launcher::PlayPage::loadSettingsCfgIntoUI()
{
    // Viewing distance
    const QString vd = readSettingsCfgKey(QStringLiteral("Cells"), QStringLiteral("viewing distance"));
    if (!vd.isEmpty())
    {
        bool ok = false; const int v = vd.toInt(&ok);
        if (ok) { viewDistanceSlider->setValue(v); label_viewDistanceValue->setText(vd); }
    }

    // Texture filter
    const QString tf = readSettingsCfgKey(QStringLiteral("General"), QStringLiteral("texture mag filter"));
    if (!tf.isEmpty()) { const int i = textureFilterComboBox->findData(tf); if (i >= 0) textureFilterComboBox->setCurrentIndex(i); }

    // Anisotropy
    const QString an = readSettingsCfgKey(QStringLiteral("General"), QStringLiteral("anisotropy"));
    if (!an.isEmpty()) { bool ok; const int v = an.toInt(&ok); if (ok) anisotropySpinBox->setValue(v); }

    // Shadows
    shadowsCheckBox->setChecked(readSettingsCfgKey(QStringLiteral("Shadows"), QStringLiteral("enable shadows")) == QStringLiteral("true"));
    const QString sr = readSettingsCfgKey(QStringLiteral("Shadows"), QStringLiteral("shadow map resolution"));
    if (!sr.isEmpty()) { const int i = shadowResComboBox->findData(sr); if (i >= 0) shadowResComboBox->setCurrentIndex(i); }
    actorShadowsCheckBox->setChecked(readSettingsCfgKey(QStringLiteral("Shadows"), QStringLiteral("actor shadows")) == QStringLiteral("true"));
    terrainShadowsCheckBox->setChecked(readSettingsCfgKey(QStringLiteral("Shadows"), QStringLiteral("terrain shadows")) == QStringLiteral("true"));

    // Water
    waterShaderCheckBox->setChecked(readSettingsCfgKey(QStringLiteral("Water"), QStringLiteral("shader")) == QStringLiteral("true"));
    fogCheckBox->setChecked(readSettingsCfgKey(QStringLiteral("Fog"), QStringLiteral("use distance fog")) != QStringLiteral("false"));
    radialFogCheckBox->setChecked(readSettingsCfgKey(QStringLiteral("Fog"), QStringLiteral("radial fog")) == QStringLiteral("true"));

    // Lighting
    const QString lm = readSettingsCfgKey(QStringLiteral("Shaders"), QStringLiteral("lighting method"));
    if (!lm.isEmpty()) { const int i = lightingModeComboBox->findData(lm); if (i >= 0) lightingModeComboBox->setCurrentIndex(i); }
    const QString ml = readSettingsCfgKey(QStringLiteral("Shaders"), QStringLiteral("maximum lights"));
    if (!ml.isEmpty()) { bool ok; const int v = ml.toInt(&ok); if (ok) maxLightsSpinBox->setValue(v); }

    updatePresetButtonStates(detectCurrentPreset());
}

int Launcher::PlayPage::detectCurrentPreset() const
{
    const QList<GraphicsPreset> presets = {
        makeLowPreset(), makeMediumPreset(), makeHighPreset(), makeUltraPreset()
    };
    const GraphicsPreset current = collectUIPreset();
    for (int i = 0; i < presets.size(); ++i)
    {
        const GraphicsPreset& p = presets[i];
        if (current.viewDistance    == p.viewDistance    &&
            current.textureFilter   == p.textureFilter   &&
            current.anisotropy      == p.anisotropy      &&
            current.shadows         == p.shadows         &&
            current.shadowMapSize   == p.shadowMapSize   &&
            current.actorShadows    == p.actorShadows    &&
            current.terrainShadows  == p.terrainShadows  &&
            current.waterShader     == p.waterShader     &&
            current.waterReflection == p.waterReflection &&
            current.fog             == p.fog             &&
            current.radialFog       == p.radialFog       &&
            current.lightingMethod  == p.lightingMethod  &&
            current.maxLights       == p.maxLights)
            return i;
    }
    return -1;
}

// ============================================================================
// loadGraphicsPresets – initialise UI from settings.cfg on startup
// ============================================================================
void Launcher::PlayPage::loadGraphicsPresets()
{
    // Set slider range label
    label_viewDistanceValue->setText(QString::number(viewDistanceSlider->value()));

    // Load current values from settings.cfg (if available)
    loadSettingsCfgIntoUI();

    setGraphicsPresetStatus(tr("Loaded from settings.cfg"));
}

// ============================================================================
// Graphics preset slots
// ============================================================================
void Launcher::PlayPage::slotPresetLow()
{
    applyPresetToUI(makeLowPreset());
    updatePresetButtonStates(0);
    mStagedPreset    = makeLowPreset();
    mHasStagedPreset = true;
    setGraphicsPresetStatus(tr("Low preset staged — click \"Apply\" to write settings.cfg"));
}

void Launcher::PlayPage::slotPresetMedium()
{
    applyPresetToUI(makeMediumPreset());
    updatePresetButtonStates(1);
    mStagedPreset    = makeMediumPreset();
    mHasStagedPreset = true;
    setGraphicsPresetStatus(tr("Medium preset staged — click \"Apply\" to write settings.cfg"));
}

void Launcher::PlayPage::slotPresetHigh()
{
    applyPresetToUI(makeHighPreset());
    updatePresetButtonStates(2);
    mStagedPreset    = makeHighPreset();
    mHasStagedPreset = true;
    setGraphicsPresetStatus(tr("High preset staged — click \"Apply\" to write settings.cfg"));
}

void Launcher::PlayPage::slotPresetUltra()
{
    applyPresetToUI(makeUltraPreset());
    updatePresetButtonStates(3);
    mStagedPreset    = makeUltraPreset();
    mHasStagedPreset = true;
    setGraphicsPresetStatus(tr("Ultra preset staged — click \"Apply\" to write settings.cfg"));
}

void Launcher::PlayPage::slotApplyGraphicsPreset()
{
    // Collect current UI state (may have been tweaked after preset selection)
    const GraphicsPreset toApply = collectUIPreset();
    applyPresetToSettingsCfg(toApply);

    const int detected = detectCurrentPreset();
    updatePresetButtonStates(detected);
    mHasStagedPreset = false;

    const QString presetName = (detected >= 0)
        ? QStringList{tr("Low"), tr("Medium"), tr("High"), tr("Ultra")}[detected]
        : tr("Custom");
    setGraphicsPresetStatus(tr("Applied %1 settings to settings.cfg").arg(presetName));
}

void Launcher::PlayPage::slotSaveGraphicsPreset()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, tr("Save preset"),
        tr("Preset name:"),
        QLineEdit::Normal,
        tr("My preset"),
        &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    const GraphicsPreset current = collectUIPreset(name.trimmed());

    // Persist into QSettings (launcher.ini / registry)
    QSettings cfg(QStringLiteral("TES3MP"), QStringLiteral("Launcher"));
    cfg.beginGroup(QStringLiteral("GraphicsPresets/") + current.name);
    cfg.setValue(QStringLiteral("viewDistance"),       current.viewDistance);
    cfg.setValue(QStringLiteral("textureFilter"),      current.textureFilter);
    cfg.setValue(QStringLiteral("anisotropy"),         current.anisotropy);
    cfg.setValue(QStringLiteral("shadows"),            current.shadows);
    cfg.setValue(QStringLiteral("shadowMapSize"),      current.shadowMapSize);
    cfg.setValue(QStringLiteral("actorShadows"),       current.actorShadows);
    cfg.setValue(QStringLiteral("terrainShadows"),     current.terrainShadows);
    cfg.setValue(QStringLiteral("waterShader"),        current.waterShader);
    cfg.setValue(QStringLiteral("waterReflection"),    current.waterReflection);
    cfg.setValue(QStringLiteral("fog"),                current.fog);
    cfg.setValue(QStringLiteral("radialFog"),          current.radialFog);
    cfg.setValue(QStringLiteral("lightingMethod"),     current.lightingMethod);
    cfg.setValue(QStringLiteral("maxLights"),          current.maxLights);
    cfg.setValue(QStringLiteral("bloom"),              current.bloom);
    cfg.setValue(QStringLiteral("sunlightScattering"), current.sunlightScattering);
    cfg.endGroup();

    setGraphicsPresetStatus(tr("Preset \"%1\" saved").arg(current.name));
}

void Launcher::PlayPage::slotViewDistanceChanged(int value)
{
    label_viewDistanceValue->setText(QString::number(value));
    // Deselect named preset buttons when user tweaks the slider
    const int detected = detectCurrentPreset();
    if (detected != mCurrentPresetIndex)
        updatePresetButtonStates(-1);
}
