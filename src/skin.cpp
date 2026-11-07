#include "skin.h"

#include "fatal.h"

#include <QList>

namespace {

const QList<Skin>& skins()
{
  static const QList<Skin> all = {
    {QStringLiteral("Ankh"), QColor("#181B1C"), QColor("#202526"), QColor("#E9E5DA"), QColor("#A4AAA7"), QColor("#C4A565"), QColor("#373D3C"), true},
    {QStringLiteral("MMOnomicon"), QColor("#211F1B"), QColor("#2A2822"), QColor("#E7E0CF"), QColor("#ADA79B"), QColor("#91AAA0"), QColor("#454239"), false},
  };
  return all;
}

}

const Skin& skinNamed(const QString& name)
{
  for (const Skin& skin : skins()) {
    if (skin.name == name) {
      return skin;
    }
  }
  fatal(QStringLiteral("Unknown skin: %1").arg(name));
}

QStringList skinNames()
{
  QStringList names;
  for (const Skin& skin : skins()) {
    names.append(skin.name);
  }
  return names;
}

QString buildStyleSheet(const Skin& skin)
{
  QString sheet = QStringLiteral(R"(
* { font-family: "Segoe UI"; font-size: 14px; color: @text@; }
QWidget { background: @base@; }
QWidget#sidebar, QWidget#sidebarHeader, QWidget#bottomBar { background: @raised@; }
QWidget#bottomBar { border-top: 1px solid @border@; }
QLabel, QSvgWidget { background: transparent; }
QLabel[secondary="true"] { color: @secondary@; font-size: 13px; }
QLabel[heading="true"] { font-size: 22px; }
QLabel[sectionHeading="true"] { font-size: 16px; font-weight: 600; }
QLineEdit, QComboBox { background: @raised@; border: 1px solid @border@; border-radius: 4px; padding: 6px 10px; min-height: 22px; }
QLineEdit:focus, QComboBox:focus { border-color: @accent@; }
QComboBox::drop-down { border: none; width: 24px; }
QComboBox QAbstractItemView { background: @raised@; border: 1px solid @border@; selection-background-color: @border@; outline: none; }
QPushButton { background: transparent; border: 1px solid @border@; border-radius: 4px; padding: 7px 16px; min-height: 22px; }
QPushButton:hover, QPushButton:focus { border-color: @accent@; }
QPushButton:disabled { color: @secondary@; }
QPushButton#primary { background: @accent@; color: @base@; border-color: @accent@; padding: 9px 32px; font-weight: 600; }
QPushButton#primary:disabled { background: @border@; color: @secondary@; border-color: @border@; }
QPushButton[link="true"] { border: none; color: @accent@; padding: 2px 4px; text-align: left; min-height: 20px; }
QPushButton[navigation="true"] { border: none; border-bottom: 2px solid transparent; border-radius: 0; padding: 8px 14px; }
QPushButton[navigation="true"]:checked { border-bottom: 2px solid @accent@; }
QPushButton[settingsNavigation="true"] { border: none; border-radius: 4px; padding: 10px 14px; text-align: left; min-height: 24px; }
QPushButton[settingsNavigation="true"]:checked { background: @border@; }
QListView { background: transparent; border: none; outline: none; }
QTabBar { background: transparent; }
QTabBar::tab { background: transparent; border: none; border-bottom: 2px solid transparent; padding: 8px 16px; color: @secondary@; }
QTabBar::tab:selected { color: @text@; border-bottom: 2px solid @accent@; }
QTabBar::tab:hover { color: @text@; }
QPlainTextEdit, QTextBrowser { background: @raised@; border: 1px solid @border@; border-radius: 4px; }
QScrollArea { border: none; }
QScrollBar:vertical { background: transparent; width: 12px; margin: 0; }
QScrollBar::handle:vertical { background: @border@; border-radius: 4px; min-height: 32px; margin: 2px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QToolTip { background: @raised@; color: @text@; border: 1px solid @border@; padding: 4px 8px; }
QDialog { background: @base@; }
)");
  sheet.replace(QStringLiteral("@base@"), skin.base.name());
  sheet.replace(QStringLiteral("@raised@"), skin.raised.name());
  sheet.replace(QStringLiteral("@text@"), skin.text.name());
  sheet.replace(QStringLiteral("@secondary@"), skin.secondary.name());
  sheet.replace(QStringLiteral("@accent@"), skin.accent.name());
  sheet.replace(QStringLiteral("@border@"), skin.border.name());
  return sheet;
}
