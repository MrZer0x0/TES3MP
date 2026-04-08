#include "playpage.hpp"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTextStream>
#include <QVBoxLayout>

Launcher::PlayPage::PlayPage(QWidget *parent)
    : QWidget(parent)
    , mEmbeddedServerConsole(nullptr)
{
    setObjectName("PlayPage");
    setupUi(this);

    connect(playButton, SIGNAL(clicked()), this, SLOT(slotPlayClicked()));
    connect(serverButton, SIGNAL(clicked()), this, SLOT(slotServerClicked()));
    connect(reloadServerSettingsButton, SIGNAL(clicked()), this, SLOT(slotReloadServerSettings()));
    connect(saveServerSettingsButton, SIGNAL(clicked()), this, SLOT(slotSaveServerSettings()));

    pageTabs->setCurrentIndex(0);
    loadServerSettings();
}

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
    if (widget == nullptr)
        return;

    if (mEmbeddedServerConsole == widget)
        return;

    mEmbeddedServerConsole = widget;
    widget->setParent(serverConsoleHost);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    serverConsoleHostLayout->addWidget(widget);
}

QString Launcher::PlayPage::serverAddress() const
{
    QString addr = serverAddressEdit->text().trimmed();
    return addr.isEmpty() ? QString("localhost") : addr;
}

QString Launcher::PlayPage::serverPort() const
{
    QString p = serverPortEdit->text().trimmed();
    return p.isEmpty() ? QString("25565") : p;
}

void Launcher::PlayPage::switchToServerConsoleTab()
{
    pageTabs->setCurrentWidget(serverConsoleTab);
}

QString Launcher::PlayPage::serverConfigPath() const
{
    const QDir baseDir(QApplication::applicationDirPath());
    return QDir::cleanPath(baseDir.filePath(QStringLiteral("server/scripts/config.lua")));
}

void Launcher::PlayPage::loadServerSettings()
{
    const QString path = serverConfigPath();
    serverSettingsPathLabel->setText(QDir::toNativeSeparators(path));

    QFile file(path);
    if (!file.exists())
    {
        serverSettingsEditor->setPlainText(QString());
        serverSettingsStatusLabel->setText(tr("config.lua not found"));
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        serverSettingsStatusLabel->setText(tr("Could not open config.lua"));
        return;
    }

    QTextStream stream(&file);
    serverSettingsEditor->setPlainText(stream.readAll());
    serverSettingsStatusLabel->setText(tr("Loaded config.lua"));
}

bool Launcher::PlayPage::saveServerSettings()
{
    const QString path = serverConfigPath();
    QFileInfo info(path);
    QDir dir = info.absoluteDir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
    {
        QMessageBox::warning(this, tr("Save error"), tr("Could not create directory for config.lua"));
        serverSettingsStatusLabel->setText(tr("Could not create directory"));
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QMessageBox::warning(this, tr("Save error"), tr("Could not write config.lua"));
        serverSettingsStatusLabel->setText(tr("Could not write config.lua"));
        return false;
    }

    QTextStream stream(&file);
    stream << serverSettingsEditor->toPlainText();
    file.close();

    serverSettingsStatusLabel->setText(tr("Saved config.lua"));
    return true;
}

void Launcher::PlayPage::slotPlayClicked()
{
    emit playButtonClicked();
}

void Launcher::PlayPage::slotServerClicked()
{
    switchToServerConsoleTab();
    emit serverButtonClicked();
}

void Launcher::PlayPage::slotReloadServerSettings()
{
    loadServerSettings();
}

void Launcher::PlayPage::slotSaveServerSettings()
{
    saveServerSettings();
}
