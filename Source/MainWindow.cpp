#include "MainWindow.h"
#include "Theme.h"

MainWindow::MainWindow (const juce::String& name)
    : DocumentWindow (name,
                      shp::theme::background,
                      DocumentWindow::minimiseButton | DocumentWindow::closeButton)
{
    juce::LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);

    setUsingNativeTitleBar (true);
    setResizable (true, false);
    setResizeLimits (640, 420, 1600, 1200);

    auto content = std::make_unique<MainComponent>();
    setContentOwned (content.release(), true);

    centreWithSize (getWidth(), getHeight());
    setVisible (true);
}

MainWindow::~MainWindow()
{
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
