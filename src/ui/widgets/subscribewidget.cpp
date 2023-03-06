#include "subscribewidget.h"
#include "http.h"
#include "settingsstore.h"

SubscribeWidget::SubscribeWidget(QWidget* parent) : QWidget(parent)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_subscribeLabel = new SubscribeLabel(this);
    m_layout->addWidget(m_subscribeLabel);

    m_subscribersCountLabel = new TubeLabel(this);
    m_subscribersCountLabel->setFixedHeight(24);
    m_subscribersCountLabel->setStyleSheet(subscribersCountStyle);
    m_layout->addWidget(m_subscribersCountLabel);
}

void SubscribeWidget::setPreferredPalette(const QPalette& pal)
{
    m_subscribeLabel->setPreferredPalette(pal);
    m_subscribersCountLabel->setPalette(pal);
}

void SubscribeWidget::setSubscribeButton(const InnertubeObjects::SubscribeButton& subscribeButton)
{
    m_subscribeLabel->setSubscribeButton(subscribeButton);
}

void SubscribeWidget::setSubscriberCount(const QString& subscriberCountText, const QString& channelId)
{
    if (!SettingsStore::instance().fullSubs)
    {
        m_subscribersCountLabel->setText(subscriberCountText.left(subscriberCountText.lastIndexOf(" ")));
        m_subscribersCountLabel->adjustSize();
        return;
    }

    Http http;
    http.setReadTimeout(2000);
    http.setMaxRetries(5);

    HttpReply* reply = http.get(QUrl("https://api.socialcounts.org/youtube-live-subscriber-count/" + channelId));
    connect(reply, &HttpReply::error, this, [this, subscriberCountText] {
        m_subscribersCountLabel->setText(subscriberCountText.left(subscriberCountText.lastIndexOf(" ")));
        m_subscribersCountLabel->adjustSize();
    });
    connect(reply, &HttpReply::finished, this, [this](const HttpReply& reply) {
        int subs = QJsonDocument::fromJson(reply.body())["est_sub"].toInt();
        m_subscribersCountLabel->setText(QLocale::system().toString(subs));
        m_subscribersCountLabel->adjustSize();
    });
}
