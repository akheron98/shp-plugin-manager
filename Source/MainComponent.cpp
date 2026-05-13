#include "MainComponent.h"
#include "Theme.h"
#include <BinaryData.h>

using namespace shp::theme;

namespace
{
constexpr int kHeaderHeight = 88;
constexpr int kCardHeight   = 100;
constexpr int kCardGap      = 10;
constexpr int kSidePadding  = 24;
}

class MainComponent::CardList : public juce::Component
{
public:
    void setPlugins (std::vector<PluginInfo> infos,
                     std::function<void (const PluginInfo&)> onAction)
    {
        cards.clear();
        for (auto& info : infos)
        {
            auto card = std::make_unique<PluginCardComponent> (std::move (info));
            card->onAction = onAction;
            addAndMakeVisible (*card);
            cards.push_back (std::move (card));
        }
        resized();
    }

    void resized() override
    {
        int y = 0;
        for (auto& c : cards)
        {
            c->setBounds (0, y, getWidth(), kCardHeight);
            y += kCardHeight + kCardGap;
        }
    }

    int preferredHeight() const
    {
        if (cards.empty()) return 0;
        return (int) cards.size() * (kCardHeight + kCardGap) - kCardGap;
    }

    int count() const { return (int) cards.size(); }

private:
    std::vector<std::unique_ptr<PluginCardComponent>> cards;
};

MainComponent::MainComponent()
    : installer (settings, tracker)
{
    logo = juce::ImageCache::getFromMemory (BinaryData::shp_logo_v3_png,
                                            BinaryData::shp_logo_v3_pngSize);

    addAndMakeVisible (refreshButton);
    refreshButton.onClick = [this] { startFetch(); };

    addAndMakeVisible (settingsButton);
    settingsButton.onClick = [this] { showSettings (true); };

    cardList = std::make_unique<CardList>();
    viewport.setViewedComponent (cardList.get(), false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    addChildComponent (updateButton);
    updateButton.setColour (juce::TextButton::buttonColourId, blood);
    updateButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    updateButton.onClick = [this]
    {
        if (pendingManagerUpdate.installerUrl.isEmpty())
            return;
        updateButton.setEnabled (false);
        updateButton.setButtonText ("Downloading…");
        UpdateChecker::downloadAndRunInstaller (pendingManagerUpdate,
            [this] (juce::String err)
            {
                updateButton.setEnabled (true);
                updateButton.setButtonText ("Update available");
                juce::NativeMessageBox::showAsync (juce::MessageBoxOptions()
                    .withIconType (juce::MessageBoxIconType::WarningIcon)
                    .withTitle ("Manager update failed")
                    .withMessage (err.isNotEmpty() ? err : juce::String ("Unknown error"))
                    .withButton ("OK"), nullptr);
            });
    };

    setSize (820, 600);

    tracker.refresh();
    startFetch();

    const auto myVersion = juce::JUCEApplication::getInstance()->getApplicationVersion();
    updateChecker.check (myVersion, [this] (ManagerUpdateInfo info)
    {
        if (info.updateAvailable && info.installerUrl.isNotEmpty())
        {
            pendingManagerUpdate = std::move (info);
            updateButton.setButtonText ("UPDATE TO v" + pendingManagerUpdate.latestVersion);
            updateButton.setVisible (true);
            resized();
        }
    });
}

MainComponent::~MainComponent() = default;

void MainComponent::showSettings (bool show)
{
    if (show)
    {
        settingsPage = std::make_unique<SettingsPage> (settings);
        settingsPage->onClose = [this] { showSettings (false); };
        settingsPage->onSettingsChanged = [this]
        {
            registry.setManifestUrl (settings.getRegistryUrl());
            registry.setGithubToken (settings.getGithubToken());
            startFetch();
        };
        addAndMakeVisible (*settingsPage);
        viewport.setVisible (false);
        refreshButton.setVisible (false);
        settingsButton.setVisible (false);
        resized();
    }
    else
    {
        settingsPage.reset();
        viewport.setVisible (true);
        refreshButton.setVisible (true);
        settingsButton.setVisible (true);
        resized();
        repaint();
    }
}

PluginInfo MainComponent::toPluginInfo (const RegistryPlugin& r) const
{
    PluginInfo info;
    info.id          = r.id;
    info.name        = r.name;
    info.description = r.description;

    if (auto installed = tracker.get (r.id))
        info.installedVersion = installed->version;

    if (r.latestVersion.isEmpty())
    {
        info.status       = info.installedVersion.isNotEmpty()
                                ? PluginInfo::Status::upToDate
                                : PluginInfo::Status::noRelease;
        info.errorMessage = r.releaseError;
    }
    else
    {
        info.latestVersion = r.latestVersion;

        if (info.installedVersion.isEmpty())
            info.status = PluginInfo::Status::notInstalled;
        else if (info.installedVersion == r.latestVersion)
            info.status = PluginInfo::Status::upToDate;
        else
            info.status = PluginInfo::Status::updateAvailable;
    }

    return info;
}

void MainComponent::startFetch()
{
    loadState = LoadState::loading;
    loadStatusMessage = "Fetching registry…";
    refreshButton.setEnabled (false);
    cardList->setPlugins ({}, {});
    manifestByPluginId.clear();
    repaint();
    resized();

    registry.setManifestUrl (settings.getRegistryUrl());
    registry.setGithubToken (settings.getGithubToken());

    registry.fetch ([this] (RegistryFetchResult result)
    {
        rebuildFromResult (std::move (result));
    });
}

void MainComponent::rebuildFromResult (RegistryFetchResult result)
{
    refreshButton.setEnabled (true);

    if (! result.manifestOk)
    {
        loadState = LoadState::error;
        loadStatusMessage = "Registry unreachable: " + result.manifestError;
        cardList->setPlugins ({}, {});
        manifestByPluginId.clear();
        repaint();
        resized();
        return;
    }

    manifestByPluginId.clear();
    std::vector<PluginInfo> infos;
    infos.reserve (result.plugins.size());
    for (auto& r : result.plugins)
    {
        manifestByPluginId[r.id] = r;
        infos.push_back (toPluginInfo (r));
    }

    cardList->setPlugins (std::move (infos),
                          [this] (const PluginInfo& info) { handleCardAction (info); });

    loadState = LoadState::loaded;
    loadStatusMessage = {};
    repaint();
    resized();
}

void MainComponent::handleCardAction (const PluginInfo& info)
{
    auto it = manifestByPluginId.find (info.id);
    if (it == manifestByPluginId.end())
    {
        juce::NativeMessageBox::showAsync (juce::MessageBoxOptions()
            .withIconType (juce::MessageBoxIconType::WarningIcon)
            .withTitle ("Internal error")
            .withMessage ("Plugin entry no longer in manifest cache. Try Refresh.")
            .withButton ("OK"), nullptr);
        return;
    }

    const auto& plugin = it->second;

    auto onProgress = [this, pluginName = plugin.name] (InstallProgress p)
    {
        loadStatusMessage = pluginName + " — " + p.message;
        repaint();

        if (p.stage == InstallProgress::Stage::done)
        {
            tracker.refresh();
            // Rebuild from cached manifest so version pills update.
            std::vector<PluginInfo> infos;
            for (auto& [id, r] : manifestByPluginId)
                infos.push_back (toPluginInfo (r));
            cardList->setPlugins (std::move (infos),
                                  [this] (const PluginInfo& i) { handleCardAction (i); });
            resized();
            juce::Timer::callAfterDelay (2500, [this] { loadStatusMessage = {}; repaint(); });
        }
        else if (p.stage == InstallProgress::Stage::failed)
        {
            juce::NativeMessageBox::showAsync (juce::MessageBoxOptions()
                .withIconType (juce::MessageBoxIconType::WarningIcon)
                .withTitle ("Install failed")
                .withMessage (p.message + (p.errorDetail.isNotEmpty()
                                ? "\n\n" + p.errorDetail
                                : juce::String()))
                .withButton ("OK"), nullptr);
            loadStatusMessage = {};
            repaint();
        }
    };

    switch (info.status)
    {
        case PluginInfo::Status::notInstalled:
        case PluginInfo::Status::updateAvailable:
            installer.install (plugin, onProgress);
            break;
        case PluginInfo::Status::upToDate:
            installer.uninstall (info.id, plugin.vst3BundleName, onProgress);
            break;
        case PluginInfo::Status::noRelease:
        case PluginInfo::Status::error:
            break;
    }
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (background);

    if (settingsPage != nullptr)
        return;  // SettingsPage paints itself fullscreen

    auto header = getLocalBounds().removeFromTop (kHeaderHeight);

    g.setColour (surfaceDeep);
    g.fillRect (header);
    g.setColour (railDark);
    g.drawHorizontalLine (header.getBottom() - 1, (float) header.getX(), (float) header.getRight());

    auto logoArea = header.removeFromLeft (kHeaderHeight).reduced (16);
    if (logo.isValid())
        g.drawImage (logo, logoArea.toFloat(), juce::RectanglePlacement::centred);

    auto titleArea = header.reduced (8, 14);
    titleArea.removeFromRight (220);

    g.setColour (bone);
    g.setFont (oswaldFont (28.0f, juce::Font::bold, 0.05f));
    g.drawText ("SHP PLUGIN MANAGER",
                titleArea.removeFromTop (32),
                juce::Justification::bottomLeft,
                true);

    g.setColour (dimBone);
    g.setFont (monoFont (10.5f));
    g.drawText ("Install, update, and manage your SHP VST3 suite",
                titleArea.removeFromTop (16),
                juce::Justification::topLeft,
                true);

    if (loadState != LoadState::loaded || cardList->count() == 0 || loadStatusMessage.isNotEmpty())
    {
        auto bodyArea = getLocalBounds().withTrimmedTop (kHeaderHeight).reduced (kSidePadding, 12);
        auto bannerArea = bodyArea.removeFromTop (26);

        auto msg = loadStatusMessage;
        if (msg.isEmpty())
        {
            switch (loadState)
            {
                case LoadState::idle:    msg = "Ready."; break;
                case LoadState::loading: msg = "Fetching registry…"; break;
                case LoadState::loaded:  msg = "No plugins in registry."; break;
                case LoadState::error:   msg = "Registry error."; break;
            }
        }

        const bool isError = loadState == LoadState::error;
        g.setColour ((isError ? blood : dust).withAlpha (0.20f));
        g.fillRoundedRectangle (bannerArea.toFloat(), 3.0f);
        g.setColour (isError ? blood.brighter (0.2f) : bone.withAlpha (0.8f));
        g.setFont (monoFont (10.5f, juce::Font::bold));
        g.drawFittedText (msg, bannerArea.reduced (8, 2), juce::Justification::centredLeft, 1);
    }
}

void MainComponent::resized()
{
    if (settingsPage != nullptr)
    {
        settingsPage->setBounds (getLocalBounds());
        return;
    }

    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop (kHeaderHeight);

    auto headerButtons = header.reduced (12).removeFromRight (
        updateButton.isVisible() ? 410 : 220);
    headerButtons.removeFromTop (headerButtons.getHeight() / 2 - 16);
    settingsButton.setBounds (headerButtons.removeFromRight (100).withHeight (30));
    headerButtons.removeFromRight (10);
    refreshButton.setBounds (headerButtons.removeFromRight (100).withHeight (30));
    if (updateButton.isVisible())
    {
        headerButtons.removeFromRight (10);
        updateButton.setBounds (headerButtons.removeFromRight (180).withHeight (30));
    }

    bounds.reduce (kSidePadding, 16);

    // Reserve a slim banner row at the top of body when a message is shown.
    const bool needBanner = loadState != LoadState::loaded
                         || cardList->count() == 0
                         || loadStatusMessage.isNotEmpty();
    if (needBanner)
        bounds.removeFromTop (32);

    viewport.setBounds (bounds);
    cardList->setSize (bounds.getWidth() - viewport.getScrollBarThickness(),
                       cardList->preferredHeight());
}
