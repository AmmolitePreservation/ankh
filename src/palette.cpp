#include "palette.h"

const Palette& ankhPalette()
{
  static const Palette colors = {QColor("#181B1C"), QColor("#202526"), QColor("#E9E5DA"), QColor("#A4AAA7"), QColor("#C4A565"), QColor("#373D3C")};
  return colors;
}
QString buildStyleSheet(const Palette& colors)
{
  QString sheet = QStringLiteral(R"(
* { font-family: "Segoe UI"; font-size: 15px; color: @text@; }
QWidget { background: @base@; }
QWidget#sidebar { background: @raised@; }
QWidget#sidebarHeader { background: @raised@; border-bottom: 1px solid @border@; }
QWidget#header { background: @base@; border-bottom: 1px solid @border@; }
QWidget#sidebarFoot { background: @raised@; border-top: 1px solid @border@; }
QWidget#bottomBar { background: @base@; border-top: 1px solid @border@; }
QFrame#divider { background: @border@; border: none; }
QLabel, QSvgWidget { background: transparent; }
QLabel[secondary="true"] { color: @secondary@; font-size: 14px; }
QLabel[heading="true"] { font-size: 30px; font-weight: 600; }
QLabel[paneHeading="true"] { font-size: 28px; font-weight: 600; }
QLabel[subheading="true"] { font-size: 17px; color: @secondary@; }
QLabel[sectionHeading="true"] { font-size: 20px; font-weight: 600; }
QLabel[characterName="true"] { font-size: 18px; font-weight: 600; }
QLabel[statusTitle="true"] { font-size: 19px; font-weight: 600; }
QLineEdit, QComboBox { background: @raised@; border: 1px solid @border@; border-radius: 4px; padding: 7px 12px; min-height: 24px; }
QLineEdit:focus, QComboBox:focus { border-color: @accent@; }
QComboBox::drop-down { border: none; width: 28px; }
QComboBox QAbstractItemView { background: @raised@; border: 1px solid @border@; selection-background-color: @border@; outline: none; }
QMenu { background: @raised@; border: 1px solid @border@; padding: 4px; }
QMenu::item { padding: 8px 24px; border-radius: 3px; }
QMenu::item:selected { background: @border@; }
QMenu::item:disabled { color: @secondary@; }
QMenu::separator { height: 1px; background: @border@; margin: 4px 8px; }
QPushButton { background: transparent; border: 1px solid @border@; border-radius: 4px; padding: 8px 18px; min-height: 24px; }
QPushButton:hover, QPushButton:focus { border-color: @accent@; }
QPushButton:disabled { color: @secondary@; }
QPushButton::menu-indicator { image: none; width: 0; }
QPushButton#primary { background: @accent@; color: @base@; border-color: @accent@; padding: 9px 32px; font-weight: 600; }
QPushButton#primary[large="true"] { font-size: 22px; padding: 12px 40px; }
QPushButton#primary:disabled { background: @border@; color: @secondary@; border-color: @border@; }
QPushButton[link="true"] { border: none; color: @accent@; padding: 2px 4px; text-align: left; min-height: 22px; }
QPushButton[link="true"]:disabled { color: @secondary@; }
QPushButton[chip="true"] { padding: 7px 14px; font-size: 15px; }
QPushButton[chip="true"]:checked { border-color: @accent@; background: @raised@; }
QPushButton[disclosure="true"] { border: none; padding: 4px 0; text-align: left; font-size: 16px; }
QPushButton[footer="true"] { border: none; padding: 8px 4px; font-size: 16px; }
QPushButton[footer="true"]:disabled { color: @secondary@; }
QPushButton[navigation="true"] { border: none; border-bottom: 3px solid transparent; border-radius: 0; padding: 0 18px; font-size: 17px; }
QPushButton[navigation="true"]:checked { border-bottom: 3px solid @accent@; color: @accent@; }
QPushButton[settingsNavigation="true"] { border: none; border-radius: 4px; padding: 10px 14px; text-align: left; min-height: 26px; font-size: 16px; }
QPushButton[settingsNavigation="true"]:checked { background: @border@; }
QListView { background: transparent; border: none; outline: none; }
QTabBar { background: transparent; }
QTabBar::tab { background: transparent; border: none; border-bottom: 3px solid transparent; padding: 10px 22px; font-size: 17px; color: @text@; }
QTabBar::tab:selected { color: @accent@; border-bottom: 3px solid @accent@; }
QTabBar::tab:hover { color: @accent@; }
QPlainTextEdit, QTextBrowser { background: @raised@; border: 1px solid @border@; border-radius: 4px; }
QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: transparent; width: 12px; margin: 0; }
QScrollBar::handle:vertical { background: @border@; border-radius: 4px; min-height: 32px; margin: 2px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QToolTip { background: @raised@; color: @text@; border: 1px solid @border@; padding: 4px 8px; }
QDialog { background: @base@; }
)");
  sheet.replace(QStringLiteral("@base@"), colors.base.name());
  sheet.replace(QStringLiteral("@raised@"), colors.raised.name());
  sheet.replace(QStringLiteral("@text@"), colors.text.name());
  sheet.replace(QStringLiteral("@secondary@"), colors.secondary.name());
  sheet.replace(QStringLiteral("@accent@"), colors.accent.name());
  sheet.replace(QStringLiteral("@border@"), colors.border.name());
  return sheet;
}
