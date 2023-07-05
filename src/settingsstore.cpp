#include "settingsstore.h"
#include <QSettings>

SettingsStore* SettingsStore::instance()
{
    std::call_once(m_onceFlag, [] { m_instance = new SettingsStore; });
    return m_instance;
}

void SettingsStore::initializeFromSettingsFile()
{
    QSettings settings(configPath, QSettings::IniFormat);

    appStyle = settings.value("appStyle", "Default").toString();
    condensedViews = settings.value("condensedViews", false).toBool();
    darkTheme = settings.value("darkTheme", false).toBool();
    fullSubs = settings.value("fullSubs", false).toBool();
    homeShelves = settings.value("homeShelves", false).toBool();
    returnDislikes = settings.value("returnDislikes", true).toBool();
    themedChannels = settings.value("themedChannels", false).toBool();

    disable60Fps = settings.value("player/disable60Fps", false).toBool();
    disablePlayerInfoPanels = settings.value("player/disableInfoPanels", false).toBool();
    h264Only = settings.value("player/h264Only", false).toBool();
    preferredQuality = settings.value("player/preferredQuality", static_cast<int>(PlayerQuality::Auto)).value<PlayerQuality>();
    preferredVolume = settings.value("player/preferredVolume", 100).toInt();
    restoreAnnotations = settings.value("player/restoreAnnotations", false).toBool();

    playbackTracking = settings.value("privacy/playbackTracking", true).toBool();
    watchtimeTracking = settings.value("privacy/watchtimeTracking", true).toBool();

    showSBToasts = settings.value("sponsorBlock/toasts", true).toBool();
    sponsorBlockCategories.clear();

    int size = settings.beginReadArray("sponsorBlock/categories");
    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        sponsorBlockCategories.append(settings.value("name").toString());
    }
    settings.endArray();
}

void SettingsStore::saveToSettingsFile()
{
    QSettings settings(configPath, QSettings::IniFormat);

    settings.setValue("appStyle", appStyle);
    settings.setValue("condensedViews", condensedViews);
    settings.setValue("darkTheme", darkTheme);
    settings.setValue("fullSubs", fullSubs);
    settings.setValue("homeShelves", homeShelves);
    settings.setValue("returnDislikes", returnDislikes);
    settings.setValue("themedChannels", themedChannels);

    settings.setValue("player/disable60Fps", disable60Fps);
    settings.setValue("player/disableInfoPanels", disablePlayerInfoPanels);
    settings.setValue("player/h264Only", h264Only);
    settings.setValue("player/preferredQuality", static_cast<int>(preferredQuality));
    settings.setValue("player/preferredVolume", preferredVolume);
    settings.setValue("player/restoreAnnotations", restoreAnnotations);

    settings.setValue("privacy/playbackTracking", playbackTracking);
    settings.setValue("privacy/watchtimeTracking", watchtimeTracking);

    settings.setValue("sponsorBlock/toasts", showSBToasts);

    settings.beginWriteArray("sponsorBlock/categories");
    for (int i = 0; i < sponsorBlockCategories.size(); i++)
    {
        settings.setArrayIndex(i);
        settings.setValue("name", sponsorBlockCategories.at(i));
    }
    settings.endArray();
}
