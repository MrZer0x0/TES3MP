#ifndef PLAYPAGE_H
#define PLAYPAGE_H

#include "ui_playpage.h"

#include <QString>

namespace Launcher
{
    // EncoreMP PlayPage: IP/Port input instead of profile selector
    class PlayPage : public QWidget, private Ui::PlayPage
    {
        Q_OBJECT

    public:
        explicit PlayPage(QWidget *parent = nullptr);

        void setServerAddress(const QString& addr);
        void setServerPort(const QString& port);

        QString serverAddress() const;
        QString serverPort() const;

    signals:
        void playButtonClicked();
        void serverButtonClicked();

    private slots:
        void slotPlayClicked();
        void slotServerClicked();
    };
}
#endif
