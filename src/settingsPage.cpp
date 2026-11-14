#include "settingsPage.h"

#include "accountView.h"
#include "appVersion.h"
#include "attributionsView.h"
#include "installInfo.h"

#include <QCheckBox>
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
  case InstallScope::notInstalled: return QStringLiteral("Not installed by the Ankh installer");
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

SettingsPage::SettingsPage(const QString& serviceAddress, const QString& libraryRoot, bool showResearchPlugins, QWidget* parent)
  : QWidget(parent), sections(new QStackedWidget(this)), account(new AccountView(serviceAddress, this)),
    libraryRootLabel(new QLabel(libraryRoot, this)), libraryProblems(new QLabel(this)), updateStatus(new QLabel(this))
{
  auto* games = new QWidget(this);
  auto* gamesLayout = new QVBoxLayout(games);
  gamesLayout->setContentsMargins(32, 28, 32, 0);
  gamesLayout->setSpacing(16);
  gamesLayout->addWidget(headingLabel(QStringLiteral("Games"), games));
  auto* gamesForm = new QFormLayout;
  gamesForm->setHorizontalSpacing(24);
  gamesForm->setVerticalSpacing(10);
  libraryRootLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  libraryRootLabel->setWordWrap(true);
  gamesForm->addRow(QStringLiteral("Library folder"), libraryRootLabel);
  gamesLayout->addLayout(gamesForm);
  auto* changeLibrary = new QPushButton(QStringLiteral("Change library folder..."), games);
  gamesLayout->addWidget(changeLibrary, 0, Qt::AlignLeft);
  auto* libraryNote = new QLabel(QStringLiteral("Each game edition installs into its own subfolder of the library, unless another folder is chosen "
    "for that one install. Changing the library affects new installs only; games already installed stay where they are."), games);
  libraryNote->setProperty("secondary", true);
  libraryNote->setWordWrap(true);
  gamesLayout->addWidget(libraryNote);
  libraryProblems->setWordWrap(true);
  libraryProblems->setTextInteractionFlags(Qt::TextSelectableByMouse);
  libraryProblems->setVisible(false);
  gamesLayout->addWidget(libraryProblems);
  auto* researchPlugins = new QCheckBox(QStringLiteral("Show research plugins"), games);
  researchPlugins->setChecked(showResearchPlugins);
  gamesLayout->addWidget(researchPlugins, 0, Qt::AlignLeft);
  auto* researchNote = new QLabel(QStringLiteral("Research plugins record what a game does, for bug reports and for programs on this "
    "computer that read the recordings. A game's Plugins tab lists them only while this is ticked; one you have installed stays installed either way."), games);
  researchNote->setProperty("secondary", true);
  researchNote->setWordWrap(true);
  gamesLayout->addWidget(researchNote);
  gamesLayout->addStretch(1);

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

  sections->addWidget(account);
  sections->addWidget(games);
  sections->addWidget(about);
  sections->addWidget(new AttributionsView(this));

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(sections);

  connect(check, &QPushButton::clicked, this, &SettingsPage::updateCheckRequested);
  connect(changeLibrary, &QPushButton::clicked, this, &SettingsPage::libraryRootChangeRequested);
  connect(researchPlugins, &QCheckBox::toggled, this, &SettingsPage::researchPluginsShownChanged);
}

void SettingsPage::setLibraryRoot(const QString& root)
{
  libraryRootLabel->setText(root);
}

void SettingsPage::setLibraryProblems(const QStringList& problems)
{
  libraryProblems->setText(problems.isEmpty() ? QString()
    : QStringLiteral("Install records in this library could not be read, so those games show as not installed:\n%1").arg(problems.join(QLatin1Char('\n'))));
  libraryProblems->setVisible(!problems.isEmpty());
}

AccountView* SettingsPage::accountView() const
{
  return account;
}

void SettingsPage::showSection(int index)
{
  sections->setCurrentIndex(index);
}

void SettingsPage::setUpdateStatus(const QString& text)
{
  updateStatus->setText(text);
}
