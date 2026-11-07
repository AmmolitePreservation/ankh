#include "settingsPage.h"

#include "appVersion.h"
#include "attributionsView.h"
#include "skin.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

QString installScope()
{
  const QString folder = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
  const QString programFiles = qEnvironmentVariable("ProgramFiles");
  const QString programFilesX86 = qEnvironmentVariable("ProgramFiles(x86)");
  const QString localData = qEnvironmentVariable("LOCALAPPDATA");
  if (!programFiles.isEmpty() && folder.startsWith(QDir::toNativeSeparators(programFiles), Qt::CaseInsensitive)) {
    return QStringLiteral("All users");
  }
  if (!programFilesX86.isEmpty() && folder.startsWith(QDir::toNativeSeparators(programFilesX86), Qt::CaseInsensitive)) {
    return QStringLiteral("All users");
  }
  if (!localData.isEmpty() && folder.startsWith(QDir::toNativeSeparators(localData), Qt::CaseInsensitive)) {
    return QStringLiteral("This user");
  }
  return QStringLiteral("Not installed (running from a build folder)");
}

QLabel* headingLabel(const QString& text, QWidget* parent)
{
  auto* label = new QLabel(text, parent);
  label->setProperty("heading", true);
  return label;
}

}

SettingsPage::SettingsPage(const QString& currentSkin, QWidget* parent)
  : QWidget(parent), sections(new QStackedWidget(this)), updateStatus(new QLabel(this))
{
  auto* appearance = new QWidget(this);
  auto* appearanceLayout = new QVBoxLayout(appearance);
  appearanceLayout->setContentsMargins(32, 28, 32, 0);
  appearanceLayout->setSpacing(16);
  appearanceLayout->addWidget(headingLabel(QStringLiteral("Appearance"), appearance));
  auto* appearanceForm = new QFormLayout;
  appearanceForm->setHorizontalSpacing(16);
  auto* skinChoice = new QComboBox(appearance);
  skinChoice->addItems(skinNames());
  skinChoice->setCurrentText(currentSkin);
  skinChoice->setMinimumWidth(240);
  skinChoice->setAccessibleName(QStringLiteral("Skin"));
  appearanceForm->addRow(QStringLiteral("Skin"), skinChoice);
  appearanceLayout->addLayout(appearanceForm);
  appearanceLayout->addStretch(1);

  auto* about = new QWidget(this);
  auto* aboutLayout = new QVBoxLayout(about);
  aboutLayout->setContentsMargins(32, 28, 32, 0);
  aboutLayout->setSpacing(16);
  aboutLayout->addWidget(headingLabel(QStringLiteral("About Ankh"), about));
  auto* aboutForm = new QFormLayout;
  aboutForm->setHorizontalSpacing(24);
  aboutForm->setVerticalSpacing(10);
  aboutForm->addRow(QStringLiteral("Version"), new QLabel(QString::fromLatin1(appVersion), about));
  aboutForm->addRow(QStringLiteral("Install scope"), new QLabel(installScope(), about));
  auto* folderLabel = new QLabel(QDir::toNativeSeparators(QCoreApplication::applicationDirPath()), about);
  folderLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  aboutForm->addRow(QStringLiteral("Install folder"), folderLabel);
  aboutLayout->addLayout(aboutForm);
  auto* check = new QPushButton(QStringLiteral("Check for updates"), about);
  updateStatus->setProperty("secondary", true);
  updateStatus->setWordWrap(true);
  aboutLayout->addWidget(check, 0, Qt::AlignLeft);
  aboutLayout->addWidget(updateStatus);
  aboutLayout->addStretch(1);

  sections->addWidget(appearance);
  sections->addWidget(about);
  sections->addWidget(new AttributionsView(this));

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(sections);

  connect(skinChoice, &QComboBox::currentTextChanged, this, &SettingsPage::skinChosen);
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
