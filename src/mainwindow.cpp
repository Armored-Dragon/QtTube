#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qttubeapplication.h"
#include "stores/settingsstore.h"
#include "ui/browsehelper.h"
#include "ui/views/channelview.h"
#include "ui/views/viewcontroller.h"
#include "ui/views/watchview.h"
#include "ui/widgets/accountmenu/accountcontrollerwidget.h"
#include "utils/uiutils.h"
#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QScrollBar>
#include <QUrlQuery>

MainWindow::MainWindow(const QCommandLineParser& parser, QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_centralWidget = ui->centralwidget;
    m_size = geometry().size();
    m_topbar = new TopBar(this);

    notificationMenu = new ContinuableListWidget(this);
    notificationMenu->hide();
    notificationMenu->setContinuationThreshold(5);

    findbar = new FindBar(this);
    connect(ui->centralwidget, &QStackedWidget::currentChanged, this, [this] { if (findbar->isVisible()) { findbar->setReveal(false); } });

    connect(m_topbar, &TopBar::signInStatusChanged, this, [this] { if (ui->centralwidget->currentIndex() == 0) browse(); });
    connect(m_topbar->avatarButton, &TubeLabel::clicked, this, &MainWindow::showAccountMenu);
    connect(m_topbar->notificationBell, &TopBarBell::clicked, this, &MainWindow::showNotifications);
    connect(m_topbar->searchBox, &SearchBox::searchRequested, this, &MainWindow::search);

    ui->tabWidget->setTabEnabled(4, false);
    ui->tabWidget->setTabEnabled(5, false);
    ui->tabWidget->setCurrentIndex(5); // just some blank tab so you can pick one
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::browse);

    connect(notificationMenu, &ContinuableListWidget::continuationReady, this, [this] {
        BrowseHelper::instance()->continuation<InnertubeEndpoints::GetNotificationMenu>(notificationMenu, "NOTIFICATIONS_MENU_REQUEST_TYPE_INBOX", 5);
    });
    connect(ui->historyWidget, &ContinuableListWidget::continuationReady, this, [this] {
        BrowseHelper::instance()->continuation<InnertubeEndpoints::BrowseHistory>(ui->historyWidget, lastSearchQuery);
    });
    connect(ui->homeWidget, &ContinuableListWidget::continuationReady, this, [this] {
        BrowseHelper::instance()->continuation<InnertubeEndpoints::BrowseHome>(ui->homeWidget);
    });
    connect(ui->searchWidget, &ContinuableListWidget::continuationReady, this, [this] {
        BrowseHelper::instance()->continuation<InnertubeEndpoints::Search>(ui->searchWidget, lastSearchQuery);
    });
    connect(ui->subscriptionsWidget, &ContinuableListWidget::continuationReady, this, [this] {
        BrowseHelper::instance()->continuation<InnertubeEndpoints::BrowseSubscriptions>(ui->subscriptionsWidget);
    });

    QAction* reloadShortcut = new QAction(this);
    reloadShortcut->setAutoRepeat(false);
    reloadShortcut->setShortcuts(QList<QKeySequence>() << Qt::Key_F5 << QKeySequence(Qt::ControlModifier | Qt::Key_R));
    connect(reloadShortcut, &QAction::triggered, this, &MainWindow::reloadCurrentTab);
    addAction(reloadShortcut);
    
    qtTubeApp->creds().initialize();
    qtTubeApp->settings().initialize();

    UIUtils::defaultStyle = qApp->style()->objectName();
    UIUtils::setAppStyle(qtTubeApp->settings().appStyle, qtTubeApp->settings().darkTheme);

#ifdef Q_OS_LINUX
    if (qtTubeApp->settings().vaapi)
    {
        qputenv("LIBVA_DRI3_DISABLE", "1"); // fixes issue on some older GPUs
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", qgetenv("QTWEBENGINE_CHROMIUM_FLAGS") + " --enable-features=VaapiVideoDecoder --disable-features=UseChromeOSDirectVideoDecoder");
    }
#endif

    InnerTube::instance()->createContext(InnertubeClient(InnertubeClient::ClientType::WEB, "2.20231130.09.00", "DESKTOP"));
    tryRestoreData();
    connect(InnerTube::instance()->authStore(), &InnertubeAuthStore::authenticateSuccess, m_topbar, [this] { m_topbar->postSignInSetup(); });

    if (parser.isSet("channel"))
        ViewController::loadChannel(parser.value("channel"));
    else if (parser.isSet("video"))
        ViewController::loadVideo(parser.value("video"));
}

void MainWindow::browse()
{
    if (doNotBrowse)
        return;

    UIUtils::clearLayout(ui->additionalWidgets);
    ui->historySearchWidget->clear();
    ui->historyWidget->clear();
    ui->homeWidget->clear();
    ui->searchWidget->clear();
    ui->subscriptionsWidget->clear();
    ui->trendingWidget->clear();

    switch (ui->tabWidget->currentIndex())
    {
    case 0:
        trySwitchGridStatus(ui->homeWidget);
        BrowseHelper::instance()->browseHome(ui->homeWidget);
        break;
    case 1:
        BrowseHelper::instance()->browseTrending(ui->trendingWidget);
        break;
    case 2:
        trySwitchGridStatus(ui->subscriptionsWidget);
        BrowseHelper::instance()->browseSubscriptions(ui->subscriptionsWidget);
        break;
    case 3:
        QLineEdit* historySearch = new QLineEdit(this);
        historySearch->setPlaceholderText("Search watch history");
        ui->additionalWidgets->addWidget(historySearch);
        connect(historySearch, &QLineEdit::returnPressed, this, &MainWindow::searchWatchHistory);

        BrowseHelper::instance()->browseHistory(ui->historyWidget);
        break;
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (ui->centralwidget->currentIndex() == 0)
    {
        bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
        if ((ctrlPressed && event->key() == Qt::Key_F) || event->key() == Qt::Key_Escape)
            findbar->setReveal(findbar->isHidden());
    }

    QWidget::keyPressEvent(event);
}

void MainWindow::performFilteredSearch()
{
    ui->searchWidget->clear();
    int dateIndex = qobject_cast<QComboBox*>(ui->additionalWidgets->itemAt(1)->widget())->currentIndex();
    int typeIndex = qobject_cast<QComboBox*>(ui->additionalWidgets->itemAt(2)->widget())->currentIndex();
    int durIndex = qobject_cast<QComboBox*>(ui->additionalWidgets->itemAt(3)->widget())->currentIndex();
    int featIndex = qobject_cast<QComboBox*>(ui->additionalWidgets->itemAt(4)->widget())->currentIndex();
    int sortIndex = qobject_cast<QComboBox*>(ui->additionalWidgets->itemAt(5)->widget())->currentIndex();
    BrowseHelper::instance()->search(ui->searchWidget, lastSearchQuery, dateIndex, typeIndex, durIndex, featIndex, sortIndex);
}

void MainWindow::reloadCurrentTab()
{
    if (ui->centralwidget->currentIndex() != 0 || !ui->tabWidget->isTabEnabled(ui->tabWidget->currentIndex()))
        return;

    if (ui->tabWidget->currentIndex() <= 3)
        browse();
    else if (ui->tabWidget->currentIndex() == 4)
        performFilteredSearch();
    else if (ui->tabWidget->currentIndex() == 5)
        searchWatchHistory();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    m_size = event->size();
    notificationMenu->setFixedSize(width() >= 800 ? 600 : 600 - (800 - width()), height() / 2);
    m_topbar->resize(width(), 35);
    m_topbar->scaleAppropriately();
    notificationMenu->move(m_topbar->notificationBell->x() - notificationMenu->width() + 20, 34);

    if (AccountControllerWidget* accountController = findChild<AccountControllerWidget*>("accountController"))
        accountController->move(m_topbar->avatarButton->x() - accountController->width() + 20, 35);
}

void MainWindow::returnFromSearch()
{
    UIUtils::clearLayout(ui->additionalWidgets);
    doNotBrowse = true;
    disconnect(m_topbar->logo, &TubeLabel::clicked, this, &MainWindow::returnFromSearch);
    ui->tabWidget->setTabEnabled(4, false);
    UIUtils::setTabsEnabled(ui->tabWidget, true, {0, 1, 2, 3});
    doNotBrowse = false;
    ui->tabWidget->setCurrentIndex(0);
    ui->searchWidget->clear();
}

void MainWindow::returnFromWatchHistorySearch()
{
    doNotBrowse = true;
    disconnect(m_topbar->logo, &TubeLabel::clicked, this, &MainWindow::returnFromWatchHistorySearch);
    ui->tabWidget->setTabEnabled(5, false);
    UIUtils::setTabsEnabled(ui->tabWidget, true, {0, 1, 2, 3});
    doNotBrowse = false;
    ui->tabWidget->setCurrentIndex(3);
    ui->historySearchWidget->clear();
}

void MainWindow::search(const QString& query, SearchBox::SearchType searchType)
{
    if (query.isEmpty())
        return;

    if (searchType == SearchBox::SearchType::ByLink)
        searchByLink(query);
    else
        searchByQuery(query);
}

void MainWindow::searchByLink(const QString& link)
{
    QString extractedId;

    static QRegularExpression urlRegex("^https?://[^\\s/$.?#].[^\\s]*$");
    QRegularExpressionMatch urlMatch = urlRegex.match(link);

    if (urlMatch.hasMatch())
    {
        QUrl url(link);
        QString host = url.host().replace("www.", "");

        if (host != "youtube.com" && host != "youtu.be")
        {
            QMessageBox::critical(this, "Invalid URL", "URL needs to have https:// in front and the host must be youtube.com or youtu.be.");
            return;
        }

        if (url.path().startsWith("/@"))
        {
            QMessageBox::critical(this, "Not supported", "Channel handles cannot be directly searched yet. You should get a result through normally searching, though.");
            return;
        }

        if (url.path() == "/watch")
        {
            extractedId = QUrlQuery(url).queryItemValue("v");
        }
        else if (url.path().startsWith("/channel/"))
        {
            extractedId = url.path().replace("/channel/", "");
            if (!extractedId.startsWith("UC"))
            {
                QMessageBox::critical(this, "Invalid channel ID", "An invalid channel ID was entered. Channel IDs should start with \"UC\".");
                return;
            }
        }
        else
        {
            extractedId = url.path().mid(1);
        }
    }
    else if (link.startsWith("@"))
    {
        QMessageBox::critical(this, "Not supported", "Channel handles cannot be directly searched yet. You should get a result through normally searching, though.");
        return;
    }
    else
    {
        extractedId = link;
    }

    if (extractedId.startsWith("UC")) // channel IDs always start with "UC"
        ViewController::loadChannel(extractedId);
    else // well, hope it's a video
        ViewController::loadVideo(extractedId);
}

void MainWindow::searchByQuery(const QString& query)
{
    m_topbar->setAlwaysShow(true);
    UIUtils::clearLayout(ui->additionalWidgets);
    ui->historySearchWidget->clear();

    if (ChannelView* channelView = qobject_cast<ChannelView*>(ui->centralwidget->currentWidget()))
        channelView->deleteLater();
    else if (WatchView* watchView = qobject_cast<WatchView*>(ui->centralwidget->currentWidget()))
        watchView->deleteLater();

    TubeLabel* filtersLabel = new TubeLabel("Filters:", this);
    ui->additionalWidgets->addWidget(filtersLabel);

    QComboBox* dateCmb = new QComboBox(this);
    dateCmb->setPlaceholderText("Upload date");
    dateCmb->addItems({"Last hour", "Today", "This week", "This month", "This year"});
    ui->additionalWidgets->addWidget(dateCmb);

    QComboBox* typeCmb = new QComboBox(this);
    typeCmb->setPlaceholderText("Type");
    typeCmb->addItems({"Video", "Channel", "Playlist", "Movie"});
    ui->additionalWidgets->addWidget(typeCmb);

    QComboBox* durCmb = new QComboBox(this);
    durCmb->setPlaceholderText("Duration");
    durCmb->addItems({"Under 4 minutes", "Over 20 minutes", "4-20 minutes"});
    ui->additionalWidgets->addWidget(durCmb);

    QComboBox* featCmb = new QComboBox(this);
    featCmb->setPlaceholderText("Features");
    featCmb->addItems({"Live", "4K", "HD", "Subtitles/CC", "Creative Commons", "360°", "VR180", "3D", "HDR", "Location", "Purchased"});
    ui->additionalWidgets->addWidget(featCmb);

    QComboBox* sortCmb = new QComboBox(this);
    sortCmb->setPlaceholderText("Sort by");
    sortCmb->addItems({"Relevance", "Rating", "Upload date", "View count"});
    ui->additionalWidgets->addWidget(sortCmb);

    if (ui->tabWidget->currentIndex() == 4)
    {
        ui->searchWidget->clear();
    }
    else
    {
        doNotBrowse = true;
        connect(m_topbar->logo, &TubeLabel::clicked, this, &MainWindow::returnFromSearch);
        ui->tabWidget->setTabEnabled(4, true);
        UIUtils::setTabsEnabled(ui->tabWidget, false, {0, 1, 2, 3, 5});
        doNotBrowse = false;
        ui->tabWidget->setCurrentIndex(4);
    }

    lastSearchQuery = query;
    BrowseHelper::instance()->search(ui->searchWidget, lastSearchQuery);

    connect(dateCmb, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::performFilteredSearch);
    connect(typeCmb, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::performFilteredSearch);
    connect(durCmb, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::performFilteredSearch);
    connect(featCmb, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::performFilteredSearch);
    connect(sortCmb, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::performFilteredSearch);
}

void MainWindow::searchWatchHistory()
{
    if (ui->tabWidget->currentIndex() == 5)
    {
        ui->historySearchWidget->clear();
        lastSearchQuery = qobject_cast<QLineEdit*>(ui->additionalWidgets->itemAt(0)->widget())->text();
        BrowseHelper::instance()->browseHistory(ui->historySearchWidget, lastSearchQuery);
        return;
    }

    doNotBrowse = true;
    connect(m_topbar->logo, &TubeLabel::clicked, this, &MainWindow::returnFromWatchHistorySearch);
    ui->tabWidget->setTabEnabled(5, true);
    UIUtils::setTabsEnabled(ui->tabWidget, false, {0, 1, 2, 3});
    doNotBrowse = false;
    ui->tabWidget->setCurrentIndex(5);

    lastSearchQuery = qobject_cast<QLineEdit*>(ui->additionalWidgets->itemAt(0)->widget())->text();
    BrowseHelper::instance()->browseHistory(ui->historySearchWidget, lastSearchQuery);
}

void MainWindow::showAccountMenu()
{
    if (AccountControllerWidget* accountController = findChild<AccountControllerWidget*>("accountController"))
    {
        m_topbar->setAlwaysShow(ui->centralwidget->currentIndex() == 0);
        accountController->deleteLater();
        return;
    }

    m_topbar->setAlwaysShow(true);

    AccountControllerWidget* accountController = new AccountControllerWidget(this);
    accountController->setObjectName("accountController");
    accountController->show();
    accountController->raise();
    accountController->move(m_topbar->avatarButton->x() - accountController->width() + 20, 35);
    connect(accountController, &AccountControllerWidget::resized, this, [this, accountController] {
        accountController->move(m_topbar->avatarButton->x() - accountController->width() + 20, 35);
    });

    auto reply = InnerTube::instance()->get<InnertubeEndpoints::AccountMenu>();
    connect(reply, &InnertubeReply<InnertubeEndpoints::AccountMenu>::finished, accountController->accountMenu, &AccountMenuWidget::initialize);
}

void MainWindow::showNotifications()
{
    if (notificationMenu->isVisible())
    {
        m_topbar->setAlwaysShow(ui->centralwidget->currentIndex() == 0);
        notificationMenu->clear();
        notificationMenu->hide();
        return;
    }

    m_topbar->setAlwaysShow(true);
    notificationMenu->show();
    BrowseHelper::instance()->browseNotificationMenu(notificationMenu);
}

void MainWindow::tryRestoreData()
{
    qtTubeApp->creds().populateAuthStore(qtTubeApp->creds().activeLogin());
    if (InnerTube::instance()->hasAuthenticated())
        m_topbar->postSignInSetup(false);
}

void MainWindow::trySwitchGridStatus(QListWidget* listWidget)
{
    bool preferLists = qtTubeApp->settings().preferLists;
    if (preferLists && listWidget->flow() == QListWidget::LeftToRight)
    {
        listWidget->setFlow(QListWidget::TopToBottom);
        listWidget->setResizeMode(QListWidget::Fixed);
        listWidget->setSpacing(0);
        listWidget->setStyleSheet(QString());
        listWidget->setWrapping(false);
    }
    else if (!preferLists && listWidget->flow() == QListWidget::TopToBottom)
    {
        listWidget->setFlow(QListWidget::LeftToRight);
        listWidget->setResizeMode(QListWidget::Adjust);
        listWidget->setSpacing(3);
        listWidget->setStyleSheet("QListWidget::item { background: transparent; }");
        listWidget->setWrapping(true);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
