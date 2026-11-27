#pragma once

#include <QMainWindow>

class QActionGroup;
class QStackedWidget;
class QToolBar;

// Write's Settings window, laid out as a macOS settings window: a toolbar of
// panes at the top, each pane a grouped form below.
class SettingsWindow : public QMainWindow {
  Q_OBJECT

public:
  // Brings the one Settings window forward, opening it first if needed.
  static void present();

private:
  explicit SettingsWindow(QWidget *parent = nullptr);
  void addPane(const QIcon &icon, const QString &title, QWidget *pane);

  QStackedWidget *m_panes;
  QToolBar *m_toolbar;
  QActionGroup *m_paneChoice;
};
