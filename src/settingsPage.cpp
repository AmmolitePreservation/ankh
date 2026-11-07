#include "settingsPage.h"

#include "appVersion.h"
#include "attributionsView.h"
#include "installInfo.h"

#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

QString scopeText()
{
  switch (installScope()) {
  case InstallScope::allUsers: return QStringLiteral("All users");
  case InstallScope::thisUser: return QStringLiteral("This user");
  case InstallScope::buildFolder: return QStringLiteral("Not installed (running from a build folder)");
  }
  return QString();
}
QLabel* headingLabel(const QString& text, QWidget* parent)
{
  auto* label = new QLabel(text, parent);
  label->setProperty("heading", true);
  return label;
}

}

SettingsPage::SettingsPage(QWidget* parent)
  : QWidget(parent), sections(new QStackedWidget(this)), updateStatus(new QLabel(this))
{
  auto* about = new QWidget(this);
  auto* aboutLayout = new QVBoxLayout(about);
  aboutLayout->setContentsMargins(32, 28, 32, 0);
  aboutLayout->setSpacing(16);
  aboutLayout->addWidget(headingLabel(QStringLiteral("About Ankh"), about));
  auto* aboutForm = new QFormLayout;
  aboutForm->setHorizontalSpacing(24);
  aboutForm->setVerticalSpacing(10);
  aboutForm->addRow(QStringLiteral("Version"), new QLabel(QString::fromLatin1(appVersion), about));
  aboutForm->addRow(QStringLiteral("Install scope"), new QLabel(scopeText(), about));
  auto* folderLabel = new QLabel(installFolder(), about);
  folderLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  aboutForm->addRow(QStringLiteral("Install folder"), folderLabel);
  aboutLayout->addLayout(aboutForm);
  auto* check = new QPushButton(QStringLiteral("Check for updates"), about);
  updateStatus->setProperty("secondary", true);
  updateStatus->setWordWrap(true);
  aboutLayout->addWidget(check, 0, Qt::AlignLeft);
  aboutLayout->addWidget(updateStatus);
  aboutLayout->addStretch(1);

  sections->addWidget(about);
  sections->addWidget(new AttributionsView(this));

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(sections);

  connect(check, &QPushButton::clicked, this, &SettingsPage::updateCheckRequested);
}

void SettingsPage::showSection(int index)
{
  sections->setCurrentIndex(index);
}

void SettingsPage::setUpdateStatus(const QString& text)
{
  updateStatus->setText(text);
}
