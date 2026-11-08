#include "libraryPane.h"

#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

namespace {

Game gameNamed(int serverID, const QString& game, const QString& version, const QString& world, bool isInstalled = false)
{
  Game result = gameFromWorld(World{serverID, world, game, QStringLiteral("era"), version, QColor("#82b556"), 2004, 2006, QStringLiteral("Online"), std::nullopt, QString()});
  result.isInstalled = isInstalled;
  return result;
}

QList<Game> gamesInServerOrder()
{
  return {
    gameNamed(1, QStringLiteral("World of Warcraft"), QStringLiteral("Wrath (3.3.5)"), QStringLiteral("Northrend"), true),
    gameNamed(2, QStringLiteral("Motor City Online"), QStringLiteral("Beta Era"), QStringLiteral("Fordite")),
    gameNamed(3, QStringLiteral("World of Warcraft"), QStringLiteral("Classic (1.12.1)"), QStringLiteral("Arcanite"), true),
    gameNamed(4, QStringLiteral("Asheron's Call"), QStringLiteral("Throne of Destiny"), QStringLiteral("Pyreal")),
  };
}

QStringList shownOrder(LibraryPane& pane)
{
  auto* list = pane.findChild<QListView*>();
  QString selected;
  const QMetaObject::Connection connection = QObject::connect(&pane, &LibraryPane::gameSelected, &pane, [&selected](const Game& game) { selected = game.id; });
  QStringList order;
  for (int row = 0; row < list->model()->rowCount(); ++row) {
    list->setCurrentIndex(QModelIndex());
    list->setCurrentIndex(list->model()->index(row, 0));
    order << selected;
  }
  QObject::disconnect(connection);
  return order;
}

QPushButton* buttonNamed(LibraryPane& pane, const QString& accessibleName)
{
  for (QPushButton* button : pane.findChildren<QPushButton*>()) {
    if (button->accessibleName() == accessibleName) {
      return button;
    }
  }
  return nullptr;
}

QStringList ids(std::initializer_list<int> serverIDs)
{
  QStringList result;
  for (int serverID : serverIDs) {
    result << QStringLiteral("world%1").arg(serverID);
  }
  return result;
}

}

class LibraryPaneTest : public QObject {
  Q_OBJECT

private slots:
  void init()
  {
    directory = new QTemporaryDir;
    settings = new QSettings(directory->filePath(QStringLiteral("library.ini")), QSettings::IniFormat);
  }

  void cleanup()
  {
    delete settings;
    delete directory;
  }

  void gamesAreAlphabeticalWithNoSortControl()
  {
    LibraryPane pane(*settings);
    pane.showGames(gamesInServerOrder());
    QCOMPARE(shownOrder(pane), ids({4, 2, 3, 1}));
    for (QPushButton* button : pane.findChildren<QPushButton*>()) {
      QVERIFY2(button->text() != QStringLiteral("Name A-Z") && button->text() != QStringLiteral("All games") && button->text() != QStringLiteral("Favorites"), qPrintable(button->text()));
    }
    for (QLabel* label : pane.findChildren<QLabel*>()) {
      QVERIFY2(label->text() != QStringLiteral("Library") && !label->text().endsWith(QStringLiteral(" games")), qPrintable(label->text()));
    }
  }

  void gamesReloadedLaterAreAlphabeticalToo()
  {
    LibraryPane pane(*settings);
    pane.showGames(gamesInServerOrder());
    pane.showMessage(QStringLiteral("Sign in to see your games"), QStringLiteral("Sign in"));
    QList<Game> reloaded = gamesInServerOrder();
    reloaded.append(gameNamed(5, QStringLiteral("EverQuest"), QStringLiteral("Classic"), QStringLiteral("Peridot")));
    pane.showGames(reloaded);
    QCOMPARE(shownOrder(pane), ids({4, 5, 2, 3, 1}));
  }

  void theStarShowsOnlyFavorites()
  {
    settings->setValue(QStringLiteral("library/favorites"), QStringList{QStringLiteral("world2"), QStringLiteral("world3")});
    LibraryPane pane(*settings);
    pane.showGames(gamesInServerOrder());
    QPushButton* star = buttonNamed(pane, QStringLiteral("Show favorites only"));
    QVERIFY(star != nullptr);
    QVERIFY(star->isCheckable());
    QVERIFY(!star->isChecked());
    QCOMPARE(shownOrder(pane), ids({4, 2, 3, 1}));
    star->click();
    QVERIFY(star->isChecked());
    QCOMPARE(shownOrder(pane), ids({2, 3}));
    star->click();
    QCOMPARE(shownOrder(pane), ids({4, 2, 3, 1}));
  }

  void theDriveShowsOnlyInstalledGames()
  {
    LibraryPane pane(*settings);
    pane.showGames(gamesInServerOrder());
    QPushButton* drive = buttonNamed(pane, QStringLiteral("Show installed only"));
    QVERIFY(drive != nullptr);
    QVERIFY(drive->isCheckable());
    drive->click();
    QCOMPARE(shownOrder(pane), ids({3, 1}));
    drive->click();
    QCOMPARE(shownOrder(pane), ids({4, 2, 3, 1}));
  }

  void bothTogglesAndSearchNarrowTogether()
  {
    settings->setValue(QStringLiteral("library/favorites"), QStringList{QStringLiteral("world2"), QStringLiteral("world3")});
    LibraryPane pane(*settings);
    pane.showGames(gamesInServerOrder());
    buttonNamed(pane, QStringLiteral("Show favorites only"))->click();
    buttonNamed(pane, QStringLiteral("Show installed only"))->click();
    QCOMPARE(shownOrder(pane), ids({3}));
    pane.findChild<QLineEdit*>()->setText(QStringLiteral("arcanite"));
    QCOMPARE(shownOrder(pane), ids({3}));
    pane.findChild<QLineEdit*>()->setText(QStringLiteral("northrend"));
    QCOMPARE(shownOrder(pane), QStringList());
  }

  void aToggleWithNothingToShowSaysNoGamesMatch()
  {
    LibraryPane pane(*settings);
    pane.showGames(gamesInServerOrder());
    buttonNamed(pane, QStringLiteral("Show favorites only"))->click();
    QVERIFY(pane.findChild<QListView*>()->isHidden());
    bool isMessageShown = false;
    for (QLabel* label : pane.findChildren<QLabel*>()) {
      isMessageShown = isMessageShown || label->text() == QStringLiteral("No games match.");
    }
    QVERIFY(isMessageShown);
  }

private:
  QTemporaryDir* directory = nullptr;
  QSettings* settings = nullptr;
};

QTEST_MAIN(LibraryPaneTest)
#include "libraryPaneTest.moc"
