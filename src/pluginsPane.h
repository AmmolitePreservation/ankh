#pragma once

#include "ammoliteClient.h"

#include <QHash>
#include <QPixmap>
#include <QStringList>
#include <QWidget>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;

// The Plugins tab: every plugin of the game's edition as a checkbox row under its category, beside a help box with the
// selected plugin's description and screenshot. Checking a box only stages a choice; the Apply button asks for it to
// happen.
class PluginsPane : public QWidget {
  Q_OBJECT

public:
  explicit PluginsPane(QWidget* parent = nullptr);

  // plugins are the edition's packages other than the client. installedNames is empty and isInstalled false before the
  // game is installed, when the choices go with the Install button instead of Apply. busyReason, if not empty, says why
  // nothing can be changed now. Optional research plugins are listed only with showResearch; one that is installed
  // stays installed while it is not listed.
  void showPlugins(const QList<Package>& plugins, bool isInstalled, const QStringList& installedNames, const QString& busyReason,
    bool showResearch);
  QStringList chosenOptionalPlugins() const;
  void showScreenshot(const QString& packageName, const QByteArray& image);
  void showScreenshotProblem(const QString& packageName, const QString& message);

signals:
  void applyRequested(const QStringList& optionalPluginNames);
  void choiceChanged();
  void screenshotWanted(const Package& package);

private:
  void rebuild(const QString& keepSelected);
  void refreshRows();
  void refreshButtons();
  void showHelp(const Package* package);
  const Package* packageOf(QListWidgetItem* item) const;
  QString selectedName() const;
  QStringList stagedOptionalPlugins() const;
  QStringList installedOptionalPlugins() const;
  bool isListed(const Package& package) const;

  QList<Package> plugins;
  bool showResearch = false;
  bool isInstalled = false;
  QStringList installedNames;
  QString busyReason;
  QString shownSignature;
  QHash<QString, QPixmap> screenshots;
  QHash<QString, QString> screenshotProblems;
  QHash<QString, bool> screenshotRequested;
  bool isRebuilding = false;
  QLabel* intro;
  QListWidget* list;
  QPushButton* applyButton;
  QPushButton* resetButton;
  QLabel* helpTitle;
  QLabel* helpRole;
  QLabel* helpDescription;
  QLabel* helpScreenshot;
};
