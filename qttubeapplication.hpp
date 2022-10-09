#ifndef QTTUBEAPPLICATION_HPP
#define QTTUBEAPPLICATION_HPP
#include "mainwindow.h"
#include <QApplication>
#include <QMouseEvent>

class QtTubeApplication final : public QApplication
{
    Q_OBJECT
public:
    QtTubeApplication(int& argc, char** argv) : QApplication(argc, argv) {}

    bool notify(QObject* receiver, QEvent* event) override
    {
        if (receiver->objectName() == "MainWindowWindow" && event->type() == QEvent::MouseMove)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            TopBar* topbar = MainWindow::instance()->topbar;
            if (!topbar->alwaysShow)
            {
                if (mouseEvent->pos().y() < 35)
                {
                    if (topbar->isHidden() && topbar->animation->state() != topbar->animation->Running)
                    {
                        topbar->animation->setStartValue(QRect(0, 0, topbar->width(), 0));
                        topbar->animation->setEndValue(QRect(0, 0, topbar->width(), 35));
                        disconnect(topbar->animation, &QPropertyAnimation::finished, topbar, &TopBar::hide);
                        topbar->animation->start();
                        topbar->show();
                    }
                }
                else if (topbar->animation->state() != topbar->animation->Running && !topbar->isHidden())
                {
                    topbar->animation->setEndValue(QRect(0, 0, topbar->width(), 0));
                    topbar->animation->setStartValue(QRect(0, 0, topbar->width(), 35));
                    connect(topbar->animation, &QPropertyAnimation::finished, topbar, &TopBar::hide);
                    topbar->animation->start();
                }
            }
        }
        return QApplication::notify(receiver, event);
    }
};

#endif // QTTUBEAPPLICATION_HPP
