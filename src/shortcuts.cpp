#include "shortcuts.h"

#include <QSettings>

namespace {

const QString settingsGroup = QStringLiteral("shortcuts/");

// Qt spells Cmd as Ctrl and the Control key as Meta.
QKeySequence portable(const char *keys) {
  return QKeySequence(QString::fromLatin1(keys), QKeySequence::PortableText);
}

// Cmd+, opens Settings, so it can never be taken away from there. Cmd+Q,
// Cmd+H, and Cmd+Option+H belong to the application menu macOS builds.
const QList<QKeySequence> &reservedKeys() {
  static const QList<QKeySequence> reserved{portable("Ctrl+,"), portable("Ctrl+Q"),
                                            portable("Ctrl+H"), portable("Ctrl+Alt+H")};
  return reserved;
}

}

Shortcuts &Shortcuts::shared() {
  static Shortcuts instance;
  return instance;
}

// The menus' order, which the Settings window lists them in.
const QList<ShortcutCommand> &Shortcuts::commands() {
  static const QList<ShortcutCommand> list{
    {QStringLiteral("File"), QStringLiteral("New Window"), portable("Ctrl+N")},
    {QStringLiteral("File"), QStringLiteral("Open…"), portable("Ctrl+O")},
    {QStringLiteral("File"), QStringLiteral("Save"), portable("Ctrl+S")},
    {QStringLiteral("File"), QStringLiteral("Save As…"), portable("Ctrl+Shift+S")},
    {QStringLiteral("File"), QStringLiteral("Print…"), portable("Ctrl+P")},
    {QStringLiteral("File"), QStringLiteral("Close Window"), portable("Ctrl+W")},
    {QStringLiteral("Edit"), QStringLiteral("Undo"), portable("Ctrl+Z")},
    {QStringLiteral("Edit"), QStringLiteral("Redo"), portable("Ctrl+Shift+Z")},
    {QStringLiteral("Edit"), QStringLiteral("Cut"), portable("Ctrl+X")},
    {QStringLiteral("Edit"), QStringLiteral("Copy"), portable("Ctrl+C")},
    {QStringLiteral("Edit"), QStringLiteral("Paste"), portable("Ctrl+V")},
    {QStringLiteral("Edit"), QStringLiteral("Select All"), portable("Ctrl+A")},
    {QStringLiteral("Edit"), QStringLiteral("Copy as HTML"), portable("Ctrl+Shift+C")},
    {QStringLiteral("Edit"), QStringLiteral("Find"), portable("Ctrl+F")},
    // Cmd+H belongs to macOS, which hides the app with it.
    {QStringLiteral("Edit"), QStringLiteral("Find and Replace"), portable("Ctrl+Alt+F")},
    {QStringLiteral("Edit"), QStringLiteral("Bold"), portable("Ctrl+B")},
    {QStringLiteral("Edit"), QStringLiteral("Italic"), portable("Ctrl+I")},
    {QStringLiteral("Edit"), QStringLiteral("Strikethrough"), portable("Ctrl+Shift+X")},
    {QStringLiteral("Edit"), QStringLiteral("Link"), portable("Ctrl+K")},
    {QStringLiteral("View"), QStringLiteral("Bigger"), portable("Ctrl+=")},
    {QStringLiteral("View"), QStringLiteral("Smaller"), portable("Ctrl+-")},
    {QStringLiteral("View"), QStringLiteral("Actual Size"), portable("Ctrl+0")},
    {QStringLiteral("View"), QStringLiteral("Enter Full Screen"), portable("Ctrl+Meta+F")},
    {QStringLiteral("Window"), QStringLiteral("Minimize"), portable("Ctrl+M")},
    {QStringLiteral("Window"), QStringLiteral("Zoom"), QKeySequence()},
  };
  return list;
}

QKeySequence Shortcuts::defaultKeys(const QString &command) {
  for (const ShortcutCommand &entry : commands()) {
    if (entry.name == command)
      return entry.defaultKeys;
  }
  return {};
}

// An empty saved value is a key the writer cleared, unlike a missing one.
Shortcuts::Shortcuts() {
  QSettings settings;
  for (const ShortcutCommand &entry : commands()) {
    const QVariant saved = settings.value(settingsGroup + entry.name);
    m_keys.insert(entry.name, saved.isValid()
                                ? QKeySequence(saved.toString(), QKeySequence::PortableText)
                                : entry.defaultKeys);
  }
}

QKeySequence Shortcuts::keys(const QString &command) const {
  return m_keys.value(command);
}

QString Shortcuts::commandFor(const QKeySequence &keys) const {
  if (keys.isEmpty())
    return {};
  for (auto it = m_keys.cbegin(); it != m_keys.cend(); ++it) {
    if (it.value() == keys)
      return it.key();
  }
  return {};
}

bool Shortcuts::isAssignable(const QKeySequence &keys) {
  if (keys.isEmpty())
    return true;
  if (keys.count() != 1 || reservedKeys().contains(keys))
    return false;
  const QKeyCombination combination = keys[0];
  const Qt::Key key = combination.key();
  if (key >= Qt::Key_F1 && key <= Qt::Key_F35)
    return true;
  return combination.keyboardModifiers()
         & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
}

bool Shortcuts::setKeys(const QString &command, const QKeySequence &keys) {
  if (!m_keys.contains(command) || !isAssignable(keys))
    return false;
  if (m_keys.value(command) == keys)
    return true;
  const QString previousOwner = commandFor(keys);
  if (!previousOwner.isEmpty()) {
    m_keys.insert(previousOwner, QKeySequence());
    save(previousOwner);
  }
  m_keys.insert(command, keys);
  save(command);
  emit changed();
  return true;
}

void Shortcuts::restoreDefaults() {
  QSettings().remove(QStringLiteral("shortcuts"));
  for (const ShortcutCommand &entry : commands())
    m_keys.insert(entry.name, entry.defaultKeys);
  emit changed();
}

void Shortcuts::save(const QString &command) {
  QSettings settings;
  const QKeySequence keys = m_keys.value(command);
  if (keys == defaultKeys(command))
    settings.remove(settingsGroup + command);
  else
    settings.setValue(settingsGroup + command, keys.toString(QKeySequence::PortableText));
}
