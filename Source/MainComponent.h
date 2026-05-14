#pragma once

#include <JuceHeader.h>
#include "PluginCardComponent.h"
#include "RegistryClient.h"
#include "Settings.h"
#include "VersionTracker.h"
#include "InstallManager.h"
#include "SettingsPage.h"
#include "UpdateChecker.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class CardList;

    enum class LoadState { idle, loading, loaded, error };

    void startFetch();
    void checkForManagerUpdate();
    void rebuildFromResult (RegistryFetchResult result);
    PluginInfo toPluginInfo (const RegistryPlugin& r) const;
    void handleCardAction (const PluginInfo& info);
    void showSettings (bool show);

    void rebuildCards();
    void refreshBulkButtons();
    void runBulkAction (bool updatesOnly);
    void installNext (std::shared_ptr<std::vector<juce::String>> queue);

    juce::TextButton refreshButton    { "Refresh" };
    juce::TextButton settingsButton   { "Settings" };
    juce::TextButton installAllButton { "Install All" };
    juce::TextButton updateAllButton  { "Update All" };
    int installableCount { 0 };
    int updatableCount   { 0 };
    bool bulkInProgress  { false };

    std::unique_ptr<CardList> cardList;
    juce::Viewport viewport;
    juce::Image logo;

    Settings        settings;
    VersionTracker  tracker;
    RegistryClient  registry;
    InstallManager  installer;
    UpdateChecker   updateChecker;

    std::map<juce::String, RegistryPlugin> manifestByPluginId;

    std::unique_ptr<SettingsPage> settingsPage;

    ManagerUpdateInfo  pendingManagerUpdate;
    juce::TextButton   updateButton { "Update available" };

    LoadState loadState { LoadState::idle };
    juce::String loadStatusMessage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
