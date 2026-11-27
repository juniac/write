#pragma once

#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QObject>

// A menu command whose key the writer can change in Settings. The name is
// the English menu text: it keys the saved setting, and the Settings window
// translates it for display.
struct ShortcutCommand {
  QString menu;
  QString name;
  QKeySequence defaultKeys;
};

// The keys of every menu command, shared by all windows and kept in the
// app's settings. Only keys that differ from the defaults are saved, so a
// later default change still reaches writers who never touched that key.
class Shortcuts : public QObject {
  Q_OBJECT

public:
  static Shortcuts &shared();
  static const QList<ShortcutCommand> &commands();
  static QKeySequence defaultKeys(const QString &command);

  QKeySequence keys(const QString &command) const;
  // The command that `keys` runs, or an empty name.
  QString commandFor(const QKeySequence &keys) const;
  // Rejects a key that types text or that macOS and the Settings item keep.
  // A key another command had moves here, as Qt runs neither command of a
  // key claimed twice.
  bool setKeys(const QString &command, const QKeySequence &keys);
  void restoreDefaults();

  // False for a key without Cmd, Option, or Control, which would take a
  // letter away from the text, and for keys reserved by macOS or Write.
  static bool isAssignable(const QKeySequence &keys);

signals:
  void changed();

private:
  Shortcuts();
  void save(const QString &command);

  QHash<QString, QKeySequence> m_keys;
};
