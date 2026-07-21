#include "existinginstallationpage.hpp"

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>

#include "mainwizard.hpp"

Wizard::ExistingInstallationPage::ExistingInstallationPage(QWidget *parent) :
    QWizardPage(parent)
{
    mWizard = qobject_cast<MainWizard*>(parent);

    setupUi(this);

    // Add a placeholder item to the list of installations
    QListWidgetItem *emptyItem = new QListWidgetItem(tr("No existing installations detected"));
    emptyItem->setFlags(Qt::NoItemFlags);

    installationsList->insertItem(0, emptyItem);

}

void Wizard::ExistingInstallationPage::initializePage()
{
    // Add the available installation paths
    QStringList paths(mWizard->mInstallations.keys());

    // Hide the default item if there are installations to choose from
    installationsList->item(0)->setHidden(!paths.isEmpty());

    for (const QString &path : paths)
    {
        if (installationsList->findItems(path, Qt::MatchExactly).isEmpty())
        {
            QListWidgetItem *item = new QListWidgetItem(path);
            installationsList->addItem(item);
        }
    }

    connect(installationsList, SIGNAL(currentTextChanged(QString)),
            this, SLOT(textChanged(QString)));

    connect(installationsList,SIGNAL(itemSelectionChanged()),
            this, SIGNAL(completeChanged()));
}

bool Wizard::ExistingInstallationPage::validatePage()
{
    // See if Morrowind.ini is detected, if not, ask the user
    // It can be missing entirely
    // Or failed to be detected due to the target being a symlink

    QString path(field(QLatin1String("installation.path")).toString());
    QFile file(mWizard->mInstallations[path].iniPath);

    if (!file.exists()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error detecting Morrowind configuration"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setText(QObject::tr("<br><b>Could not find Morrowind.ini</b><br><br> \
                                   The Wizard needs to update settings in this file.<br><br> \
                                   Press \"Browse...\" to specify the location manually.<br>"));

        QAbstractButton *browseButton2 =
                msgBox.addButton(QObject::tr("B&rowse..."), QMessageBox::ActionRole);

        msgBox.exec();

        QString iniFile;
        if (msgBox.clickedButton() == browseButton2) {
            iniFile = QFileDialog::getOpenFileName(
                        this,
                        QObject::tr("Select configuration file"),
                        QDir::currentPath(),
                        QString(tr("Morrowind configuration file (*.ini)")));
        }

        if (iniFile.isEmpty()) {
            return false; // Cancel was clicked;
        }

        // A proper Morrowind.ini was selected, set it
        QFileInfo info(iniFile);
        mWizard->mInstallations[path].iniPath = info.absoluteFilePath();
    }

    return true;
}

void Wizard::ExistingInstallationPage::on_browseButton_clicked()
{
    QString initialPath = field(QLatin1String("installation.path")).toString();
    if (initialPath.isEmpty())
        initialPath = QDir::currentPath();

    const QString selectedDirectory = QFileDialog::getExistingDirectory(
        this,
        tr("Select the Morrowind installation folder or Data Files folder"),
        initialPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (selectedDirectory.isEmpty())
        return;

    QDir selectedDir(selectedDirectory);
    QString dataPath;

    // Accept both the game root and Data Files itself. This avoids asking the
    // user for the same installation a second time after the Wizard finishes.
    if (selectedDir.exists(QLatin1String("Morrowind.esm")))
        dataPath = selectedDir.absolutePath();
    else
    {
        const QString nestedDataPath = selectedDir.filePath(QLatin1String("Data Files"));
        QDir nestedDataDir(nestedDataPath);
        if (nestedDataDir.exists(QLatin1String("Morrowind.esm")))
            dataPath = nestedDataDir.absolutePath();
    }

    if (dataPath.isEmpty() || !mWizard->findFiles(QLatin1String("Morrowind"), dataPath))
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error detecting Morrowind files"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setText(QObject::tr(
            "<b>Morrowind.esm or Morrowind.bsa was not found.</b><br><br>"
            "Select either the Morrowind installation folder or its Data Files folder."
        ));
        msgBox.exec();
        return;
    }

    const QString path = QDir::toNativeSeparators(QDir(dataPath).absolutePath());
    QList<QListWidgetItem*> items = installationsList->findItems(path, Qt::MatchExactly);

    if (items.isEmpty())
    {
        mWizard->addInstallation(path);
        installationsList->item(0)->setHidden(true);

        QListWidgetItem* item = new QListWidgetItem(path);
        installationsList->addItem(item);
        installationsList->setCurrentItem(item);
    }
    else
        installationsList->setCurrentItem(items.first());

    emit completeChanged();
}

void Wizard::ExistingInstallationPage::textChanged(const QString &text)
{
    // Set the installation path manually, as registerField doesn't work
    // Because it doesn't accept two widgets operating on a single field
    if (!text.isEmpty())
        mWizard->setField(QLatin1String("installation.path"), text);
}

bool Wizard::ExistingInstallationPage::isComplete() const
{
    if (installationsList->selectionModel()->hasSelection()) {
        return true;
    } else {
        return false;
    }
}

int Wizard::ExistingInstallationPage::nextId() const
{
    return MainWizard::Page_LanguageSelection;
}
