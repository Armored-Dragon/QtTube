#include "subscribelabel.h"
#include "innertube.h"
#include <QMessageBox>

SubscribeLabel::SubscribeLabel(QWidget* parent) : QLabel(parent)
{
    setFixedSize(80, 24);
}

void SubscribeLabel::setPreferredPalette(const QPalette& pal)
{
    preferredPalette = pal;
    setPalette(pal);
}

void SubscribeLabel::setSubscribeButton(const InnertubeObjects::SubscribeButton& subscribeButton)
{
    this->subscribeButton = subscribeButton;
    setStyleSheet(subscribeButton.subscribed ? subscribedStyle : subscribeStyle);
    setText(subscribeButton.buttonText.text.isEmpty() ? "Subscribe" : subscribeButton.buttonText.text);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void SubscribeLabel::enterEvent(QEnterEvent*)
#else
void SubscribeButton::enterEvent(QEvent*)
#endif
{
    setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
    if (subscribeButton.subscribed)
    {
        setStyleSheet(unsubscribeStyle);
        setText(subscribeButton.unsubscribeButtonText.text);
    }
    else
    {
        setStyleSheet(subscribeHoveredStyle);
    }
    setPalette(preferredPalette);
}

void SubscribeLabel::leaveEvent(QEvent*)
{
    setCursor(QCursor());
    if (subscribeButton.subscribed)
    {
        setStyleSheet(subscribedStyle);
        setText(subscribeButton.subscribedButtonText.text);
    }
    else
    {
        setStyleSheet(subscribeStyle);
    }
    setPalette(preferredPalette);
}

void SubscribeLabel::mousePressEvent(QMouseEvent*)
{
    if (!InnerTube::instance().hasAuthenticated())
    {
        QMessageBox::information(nullptr, "Need to log in", "You must be logged in to subscribe to channels.\nLocal subscriptions are planned, but not implemented.");
        return;
    }

    if (subscribeButton.subscribed)
    {
        if (QMessageBox::question(nullptr, "Unsubscribe?", "Unsubscribe from this channel?") != QMessageBox::StandardButton::Yes)
            return;
        InnerTube::instance().subscribe(
            qAsConst(subscribeButton.onUnsubscribeEndpoints)[0]["signalServiceEndpoint"]["actions"][0]["openPopupAction"]["popup"]
                ["confirmDialogRenderer"]["confirmButton"]["buttonRenderer"]["serviceEndpoint"]["unsubscribeEndpoint"],
            false
        );
        setStyleSheet(subscribeStyle);
        setText(subscribeButton.unsubscribedButtonText.text);
        subscribeButton.subscribed = false;
    }
    else
    {
        InnerTube::instance().subscribe(qAsConst(subscribeButton.onSubscribeEndpoints)[0]["subscribeEndpoint"], true);
        setStyleSheet(subscribedStyle);
        setText(subscribeButton.subscribedButtonText.text);
        subscribeButton.subscribed = true;
    }

    setPalette(preferredPalette);
}
