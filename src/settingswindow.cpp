#include "settingswindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCoreApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QStackedWidget>
#include <QToolBar>
#include <QVBoxLayout>

#include "shortcuts.h"

namespace {

// The command names are MainWindow's menu texts, translated where the menus
// translate them.
QString translatedName(const QString &name) {
  return QCoreApplication::translate("MainWindow", name.toUtf8().constData());
}

// One rounded group of rows, as a grouped SwiftUI Form draws a Section.
QFrame *addSection(QVBoxLayout *column, const QString &title) {
  auto *heading = new QLabel(title);
  QFont bold = heading->font();
  bold.setBold(true);
  heading->setFont(bold);
  heading->setContentsMargins(4, 0, 0, 0);
  column->addWidget(heading);

  auto *group = new QFrame;
  group->setObjectName(QStringLiteral("settingsSection"));
  auto *rows = new QVBoxLayout(group);
  rows->setContentsMargins(12, 4, 12, 4);
  rows->setSpacing(0);
  column->addWidget(group);
  column->addSpacing(12);
  return group;
}

// A recorder for one command's key: a typed key that cannot be a shortcut
// is refused with a beep, and the field shows the key the command keeps.
QKeySequenceEdit *shortcutRecorder(const QString &command, QWidget *parent) {
  auto *recorder = new QKeySequenceEdit(Shortcuts::shared().keys(command), parent);
  recorder->setMaximumSequenceLength(1);
  recorder->setClearButtonEnabled(true);
  recorder->setFixedWidth(160);
  const auto showSaved = [recorder, command]() {
    const QKeySequence saved = Shortcuts::shared().keys(command);
    if (recorder->keySequence() != saved)
      recorder->setKeySequence(saved);
  };
  QObject::connect(recorder, &QKeySequenceEdit::editingFinished, recorder, [recorder, command, showSaved]() {
    if (!Shortcuts::shared().setKeys(command, recorder->keySequence()))
      QApplication::beep();
    showSaved();
  });
  // The clear button empties the field without finishing the edit.
  QObject::connect(recorder, &QKeySequenceEdit::keySequenceChanged, recorder, [command](const QKeySequence &keys) {
    if (keys.isEmpty())
      Shortcuts::shared().setKeys(command, keys);
  });
  // A key moved here from another command empties that command's field.
  QObject::connect(&Shortcuts::shared(), &Shortcuts::changed, recorder, showSaved);
  return recorder;
}

QWidget *shortcutsPane() {
  auto *pane = new QWidget;
  auto *paneColumn = new QVBoxLayout(pane);
  paneColumn->setContentsMargins(0, 0, 0, 0);
  paneColumn->setSpacing(0);

  auto *scroll = new QScrollArea(pane);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidgetResizable(true);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  auto *form = new QWidget;
  auto *column = new QVBoxLayout(form);
  column->setContentsMargins(20, 16, 20, 8);
  column->setSpacing(6);

  QString currentMenu;
  QVBoxLayout *rows = nullptr;
  for (const ShortcutCommand &entry : Shortcuts::commands()) {
    if (entry.menu != currentMenu) {
      currentMenu = entry.menu;
      QFrame *group = addSection(column, translatedName(entry.menu));
      rows = static_cast<QVBoxLayout *>(group->layout());
    } else {
      auto *divider = new QFrame;
      divider->setObjectName(QStringLiteral("settingsDivider"));
      divider->setFixedHeight(1);
      rows->addWidget(divider);
    }
    auto *row = new QWidget;
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 6, 0, 6);
    rowLayout->addWidget(new QLabel(translatedName(entry.name)), 1);
    rowLayout->addWidget(shortcutRecorder(entry.name, row));
    rows->addWidget(row);
  }
  column->addStretch(1);
  scroll->setWidget(form);
  paneColumn->addWidget(scroll, 1);

  auto *footer = new QHBoxLayout;
  footer->setContentsMargins(20, 8, 20, 16);
  footer->addStretch(1);
  auto *restore = new QPushButton(QObject::tr("Restore Defaults"), pane);
  QObject::connect(restore, &QPushButton::clicked, pane, []() { Shortcuts::shared().restoreDefaults(); });
  footer->addWidget(restore);
  paneColumn->addLayout(footer);
  return pane;
}

}

void SettingsWindow::present() {
  // ponytail: one window for the app, rebuilt on each open after a close.
  static QPointer<SettingsWindow> window;
  if (!window)
    window = new SettingsWindow;
  window->show();
  window->raise();
  window->activateWindow();
}

SettingsWindow::SettingsWindow(QWidget *parent) : QMainWindow(parent) {
  setAttribute(Qt::WA_DeleteOnClose);
  setUnifiedTitleAndToolBarOnMac(true);
  // A fixed size, as macOS settings windows keep.
  setFixedSize(520, 440);
  setStyleSheet(QStringLiteral(
    "QFrame#settingsSection { background: palette(base); border: 1px solid palette(midlight);"
    " border-radius: 8px; }"
    "QFrame#settingsDivider { background: palette(midlight); border: none; }"
    "QFrame#settingsSection QLabel { background: transparent; border: none; }"));

  m_panes = new QStackedWidget(this);
  setCentralWidget(m_panes);

  m_toolbar = addToolBar(QString());
  m_toolbar->setMovable(false);
  m_toolbar->setFloatable(false);
  m_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  m_toolbar->toggleViewAction()->setVisible(false);
  m_paneChoice = new QActionGroup(m_toolbar);

  addPane(QIcon::fromTheme(QIcon::ThemeIcon::InputKeyboard), tr("Shortcuts"), shortcutsPane());

  // The native menu bar shown over this window has no Close Window of its own.
  connect(new QShortcut(QKeySequence::Close, this), &QShortcut::activated, this, &QWidget::close);
}

void SettingsWindow::addPane(const QIcon &icon, const QString &title, QWidget *pane) {
  QAction *action = m_toolbar->addAction(icon, title);
  action->setCheckable(true);
  m_paneChoice->addAction(action);
  const int index = m_panes->addWidget(pane);
  connect(action, &QAction::triggered, this, [this, index, title]() {
    m_panes->setCurrentIndex(index);
    setWindowTitle(title);
  });
  if (index == 0) {
    action->setChecked(true);
    setWindowTitle(title);
  }
}
