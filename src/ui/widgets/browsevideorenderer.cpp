#include "browsevideorenderer.h"
#include "innertube/innertubeexception.h"
#include "ui/views/channelview.h"
#include "ui/views/watchview.h"
#include <QApplication>
#include <QMessageBox>

BrowseVideoRenderer::BrowseVideoRenderer(QWidget* parent) : QWidget(parent)
{
    channelLabel = new TubeLabel;
    hbox = new QHBoxLayout;
    metadataLabel = new TubeLabel;
    textVbox = new QVBoxLayout;
    thumbLabel = new TubeLabel;
    titleLabel = new TubeLabel;

    textVbox->addWidget(titleLabel);
    textVbox->addWidget(channelLabel);
    textVbox->addWidget(metadataLabel);

    hbox->addWidget(thumbLabel);
    hbox->addLayout(textVbox, 1);
    setLayout(hbox);

    channelLabel->setClickable(true, true);

    thumbLabel->setClickable(true, false);
    thumbLabel->setMinimumSize(1, 1);
    thumbLabel->setScaledContents(true);

    titleLabel->setClickable(true, true);
    titleLabel->setFont(QFont(QApplication::font().toString(), QApplication::font().pointSize() + 2));

    connect(channelLabel, &TubeLabel::clicked, this, &BrowseVideoRenderer::navigateChannel);
    connect(thumbLabel, &TubeLabel::clicked, this, &BrowseVideoRenderer::navigateVideo);
    connect(titleLabel, &TubeLabel::clicked, this, &BrowseVideoRenderer::navigateVideo);
}

void BrowseVideoRenderer::navigateChannel()
{
    try
    {
        ChannelView::instance()->loadChannel(channelId);
    }
    catch (const InnertubeException& ie)
    {
        QMessageBox::critical(this, "Failed to load channel", ie.message());
    }
}

void BrowseVideoRenderer::navigateVideo()
{
    try
    {
        WatchView::instance()->loadVideo(videoId, progress);
    }
    catch (const InnertubeException& ie)
    {
        QMessageBox::critical(this, "Failed to load video", ie.message());
    }
}

void BrowseVideoRenderer::setChannelData(const InnertubeObjects::VideoOwner& owner)
{
    channelId = owner.id;
    channelLabel->setText(owner.name);
}

void BrowseVideoRenderer::setThumbnail(const HttpReply& reply)
{
    QPixmap pixmap;
    pixmap.loadFromData(reply.body());
    thumbLabel->setPixmap(pixmap.scaled(240, thumbLabel->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void BrowseVideoRenderer::setVideoData(const QString& length, const QString& publishedTime, int progress, const QString& title,
                                       const QString& videoId, const QString& viewCount)
{
    this->progress = progress;
    this->videoId = videoId;

    QString progStr = QStringLiteral("%1 watched")
            .arg(QDateTime::fromSecsSinceEpoch(progress, Qt::UTC)
            .toString(progress >= 3600 ? "h:mm:ss" : "m:ss"));

    QStringList list({length, publishedTime, viewCount});
    if (progress != 0) list.append(progStr);
    list.removeAll({});

    metadataLabel->setText(list.join(" • "));
    titleLabel->setText(title.length() <= 60 ? title : title.left(60) + "…");
    titleLabel->setToolTip(title);
}
