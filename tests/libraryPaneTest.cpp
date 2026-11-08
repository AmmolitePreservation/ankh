#include "libraryPane.h"

#include <QListView>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

namespace {

Game gameNamed(int serverID, const QString& game, const QString& version, const QString& world)
{
  return gameFromWorld(World{serverID, world, game, version, QColor("#82b556"), 2004, 2006, QStringLiteral("Online"), std::nullopt, QString()});
}

QList<Game> gamesInServerOrder()
{
  return {
    gameNamed(1, QStringLiteral("World of Warcraft"), QStringLiteral("Wrath (3.3.5)"), QStringLiteral("Northrend")),
    gameNamed(2, QStringLiteral("Motor City Online"), QStringLiteral("Beta Era"), QStringLiteral("Fordite")),
    gameNamed(3, QStringLiteral("World of Warcraft"), QStringLiteral("Classic (1.12.1)"), QStringLiteral("Arcanite")),
    gameNamed(4, QStringLiteral("Asheron's Call"), QStringLiteral("Throne of Destiny"), QStringLiteral("Pyreal")),
  };
}

QStringList shownOrder(LibraryPane& pane)
{
  auto* list = pane.findChild<QListView*>();
  QString selected;
  QObject::connect(&pane, &LibraryPane::gameSelected, &pane, [&selected](const Game& game) { selected = game.id; });
  QStringList ids;
  for (int row = 0; row < list->model()->rowCount(); ++row) {
    list->setCurrentIndex(QModelIndex());
    list->setCurrentIndex(list->model()->index(row, 0));
    ids << selected;
  }
  return ids;
}

QPushButton* nameChipOf(LibraryPane& pane)
{
  for (QPushButton* button : pane.findChildren<QPushButton*>()) {
    if (button->text() == QStringLiteral("Name A-Z")) {
      return button;
    }
  }
  return nullptr;
}

}

class LibraryPaneTest : public QObject {
  Q_OBJECT

private slots:
  void gamesStartInAlphabeticalOrder()
  {
    QTemporaryDir directory;
    QSettings settings(directory.filePath(QStringLiteral("library.ini")), QSettings::IniFormat);
    LibraryPane pane(settings);
    pane.showGames(gamesInServerOrder());
    QCOMPARE(shownOrder(pane), (QStringList{QStringLiteral("world4"), QStringLiteral("world2"), QStringLiteral("world3"), QStringLiteral("world1")}));
    QVERIFY(nameChipOf(pane)->isChecked());
  }

  void turningOffNameSortRestoresServerOrder()
  {
    QTemporaryDir directory;
    QSettings settings(directory.filePath(QStringLiteral("library.ini")), QSettings::IniFormat);
    LibraryPane pane(settings);
    pane.showGames(gamesInServerOrder());
    nameChipOf(pane)->click();
    QVERIFY(!nameChipOf(pane)->isChecked());
    QCOMPARE(shownOrder(pane), (QStringList{QStringLiteral("world1"), QStringLiteral("world2"), QStringLiteral("world3"), QStringLiteral("world4")}));
  }

  void gamesReloadedLaterAreAlphabeticalToo()
  {
    QTemporaryDir directory;
    QSettings settings(directory.filePath(QStringLiteral("library.ini")), QSettings::IniFormat);
    LibraryPane pane(settings);
    pane.showGames(gamesInServerOrder());
    pane.showMessage(QStringLiteral("Sign in to see your games"), QStringLiteral("Sign in"));
    QList<Game> reloaded = gamesInServerOrder();
    reloaded.append(gameNamed(5, QStringLiteral("EverQuest"), QStringLiteral("Classic"), QStringLiteral("Peridot")));
    pane.showGames(reloaded);
    QCOMPARE(shownOrder(pane), (QStringList{QStringLiteral("world4"), QStringLiteral("world5"), QStringLiteral("world2"), QStringLiteral("world3"), QStringLiteral("world1")}));
  }
};

QTEST_MAIN(LibraryPaneTest)
#include "libraryPaneTest.moc"
