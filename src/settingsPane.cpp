#include "settingsPane.h"

#include <QButtonGroup>
#include <QPushButton>
#include <QVBoxLayout>

SettingsPane::SettingsPane(QWidget* parent)
  : QWidget(parent)
{
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 8);
  layout->setSpacing(4);
  auto* group = new QButtonGroup(this);
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
