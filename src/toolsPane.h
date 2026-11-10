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

// The Client tools tab: every tool of the game's edition as a checkbox row beside a help box with the selected tool's
// description and screenshot. Checking a box only stages a choice; the Apply button asks for it to happen.
class ToolsPane : public QWidget {
  Q_OBJECT

public:
  explicit ToolsPane(QWidget* parent = nullptr);

  // tools are the edition's packages other than the client. installedNames is empty and isInstalled false before the
  // game is installed, when the choices go with the Install button instead of Apply. busyReason, if not empty, says why
  // nothing can be changed now.
  void showTools(const QList<Package>& tools, bool isInstalled, const QStringList& installedNames, const QString& busyReason);
  QStringList chosenOptionalTools() const;
  void showScreenshot(const QString& packageName, const QByteArray& image);
  void showScreenshotProblem(const QString& packageName, const QString& message);

signals:
  void applyRequested(const QStringList& optionalToolNames);
  void choiceChanged();
  void screenshotWanted(const Package& package);

private:
  void rebuild(const QString& keepSelected);
  void refreshRows();
  void refreshButtons();
  void showHelp(const Package* package);
  const Package* packageOf(QListWidgetItem* item) const;
  QString selectedName() const;
  QStringList stagedOptionalTools() const;
  QStringList installedOptionalTools() const;

  QList<Package> tools;
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
