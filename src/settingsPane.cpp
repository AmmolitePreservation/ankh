#include "settingsPane.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QVBoxLayout>

SettingsPane::SettingsPane(QWidget* parent)
  : QWidget(parent), group(new QButtonGroup(this))
{
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 20, 24, 8);
  layout->setSpacing(4);
  group->setExclusive(true);
  const QStringList sections = {QStringLiteral("Appearance"), QStringLiteral("About Ankh"), QStringLiteral("Attributions")};
  for (int index = 0; index < sections.size(); ++index) {
    auto* button = new QPushButton(sections.at(index), this);
    button->setCheckable(true);
    button->setProperty("settingsNavigation", true);
    button->setChecked(index == 0);
    group->addButton(button, index);
    layout->addWidget(button);
  }
  layout->addStretch(1);
  connect(group, &QButtonGroup::idClicked, this, &SettingsPane::sectionSelected);
}

void SettingsPane::selectSection(int index)
{
  group->button(index)->setChecked(true);
}
