#include "palette.h"
#include "pluginsPane.h"

#include <QBuffer>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest>

namespace {

Package plugin(const QString& name, const QString& label, const QString& role, const QString& category, const QString& description,
  bool hasScreenshot = false, bool isPublished = true)
{
  Package package{name, label, role, {}, std::nullopt, description, std::nullopt, category};
  if (isPublished) {
    package.latest = PackageVersion{QStringLiteral("2026.11.03"), name + QStringLiteral(".zip"), 3 * 1024 * 1024, QStringLiteral("abc"),
      QStringLiteral("2026-11-03T00:00:00Z"), 2, 419, QUrl(QStringLiteral("https://ammolite.test/files/1/%1.zip").arg(name))};
  }
  if (hasScreenshot) {
    package.screenshot = PackageScreenshot{QUrl(QStringLiteral("https://ammolite.test/shot")), QStringLiteral("def"), QStringLiteral("image/png")};
  }
  return package;
}

QList<Package> editionPlugins()
{
  return {
    plugin(QStringLiteral("alpha-login"), QStringLiteral("Alpha login"), QStringLiteral("requiredPlugin"), QStringLiteral("Login"),
      QStringLiteral("Signs you in to modern servers.")),
    plugin(QStringLiteral("alpha-maps"), QStringLiteral("Alpha maps"), QStringLiteral("optionalPlugin"), QStringLiteral("Display"),
      QStringLiteral("Adds zone maps."), true),
    plugin(QStringLiteral("alpha-bars"), QStringLiteral("Alpha bars"), QStringLiteral("optionalPlugin"), QStringLiteral("Display"), QString()),
    plugin(QStringLiteral("alpha-soon"), QStringLiteral("Alpha soon"), QStringLiteral("optionalPlugin"), QStringLiteral("Automation"), QString(),
      false, false),
    plugin(QStringLiteral("alpha-capture"), QStringLiteral("Alpha capture"), QStringLiteral("optionalPlugin"), QStringLiteral("Research"),
      QStringLiteral("Records what the game receives.")),
  };
}

QListWidget* listOf(PluginsPane& pane)
{
  return pane.findChild<QListWidget*>(QStringLiteral("pluginList"));
}

// The first line of every row, top to bottom: a category, or a plugin's label.
QStringList rowTitles(PluginsPane& pane)
{
  QStringList titles;
  for (int row = 0; row < listOf(pane)->count(); ++row) {
    titles.append(listOf(pane)->item(row)->text().section(QLatin1Char('\n'), 0, 0));
  }
  return titles;
}

QListWidgetItem* rowOf(PluginsPane& pane, const QString& title)
{
  const int row = rowTitles(pane).indexOf(title);
  return row < 0 ? nullptr : listOf(pane)->item(row);
}

QPushButton* buttonLabelled(PluginsPane& pane, const QString& text)
{
  for (QPushButton* button : pane.findChildren<QPushButton*>()) {
    if (button->text() == text) {
      return button;
    }
  }
  return nullptr;
}

QLabel* labelWith(PluginsPane& pane, const QString& text)
{
  for (QLabel* label : pane.findChildren<QLabel*>()) {
    if (label->text().contains(text)) {
      return label;
    }
  }
  return nullptr;
}

QByteArray pngImage()
{
  QImage image(40, 20, QImage::Format_RGB32);
  image.fill(Qt::red);
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  return bytes;
}

}

Q_DECLARE_METATYPE(Package)

class PluginsPaneTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase()
  {
    qRegisterMetaType<Package>();
    qApp->setStyleSheet(buildStyleSheet(ankhPalette()));
  }

  void pluginsAreListedUnderTheirCategoriesInTheCategoryOrder()
  {
    PluginsPane pane;
    pane.showPlugins(editionPlugins(), false, {}, QString(), false);
    QCOMPARE(rowTitles(pane), QStringList({QStringLiteral("Display"), QStringLiteral("Alpha maps"), QStringLiteral("Alpha bars"),
      QStringLiteral("Login"), QStringLiteral("Alpha login"), QStringLiteral("Automation"), QStringLiteral("Alpha soon")}));
    for (const QString& category : {QStringLiteral("Display"), QStringLiteral("Login"), QStringLiteral("Automation")}) {
      QCOMPARE(rowOf(pane, category)->flags(), Qt::ItemFlags(Qt::NoItemFlags));
    }
    QCOMPARE(listOf(pane)->currentItem(), rowOf(pane, QStringLiteral("Alpha maps")));
    QVERIFY(labelWith(pane, QStringLiteral("Optional plugin")) != nullptr);
    QVERIFY(labelWith(pane, QStringLiteral("Optional plugin"))->text().contains(QStringLiteral("Display")));
  }

  void researchPluginsAreListedOnlyWhenAskedForAndAnInstalledOneStaysInstalledWhileUnlisted()
  {
    PluginsPane pane;
    pane.showPlugins(editionPlugins(), true, {QStringLiteral("alpha-login"), QStringLiteral("alpha-capture")}, QString(), false);
    QVERIFY(rowOf(pane, QStringLiteral("Research")) == nullptr);
    QVERIFY(rowOf(pane, QStringLiteral("Alpha capture")) == nullptr);
    QPushButton* apply = buttonLabelled(pane, QStringLiteral("Apply changes"));
    QVERIFY(!apply->isEnabled());
    QSignalSpy applied(&pane, &PluginsPane::applyRequested);
    rowOf(pane, QStringLiteral("Alpha bars"))->setCheckState(Qt::Checked);
    apply->click();
    QCOMPARE(applied.count(), 1);
    QCOMPARE(applied.first().first().toStringList(), QStringList({QStringLiteral("alpha-bars"), QStringLiteral("alpha-capture")}));

    pane.showPlugins(editionPlugins(), true, {QStringLiteral("alpha-login"), QStringLiteral("alpha-capture")}, QString(), true);
    QCOMPARE(rowTitles(pane).mid(7), QStringList({QStringLiteral("Research"), QStringLiteral("Alpha capture")}));
    QVERIFY(rowOf(pane, QStringLiteral("Alpha capture"))->checkState() == Qt::Checked);
    rowOf(pane, QStringLiteral("Alpha capture"))->setCheckState(Qt::Unchecked);
    QVERIFY(rowOf(pane, QStringLiteral("Alpha capture"))->text().contains(QStringLiteral("will be removed")));
    QVERIFY(pane.chosenOptionalPlugins().isEmpty());
  }

  void beforeAnInstallRequiredPluginsAreTickedAndLockedAndOptionalOnesAreYoursToTick()
  {
    PluginsPane pane;
    pane.showPlugins(editionPlugins(), false, {}, QString(), false);
    QListWidgetItem* login = rowOf(pane, QStringLiteral("Alpha login"));
    QListWidgetItem* maps = rowOf(pane, QStringLiteral("Alpha maps"));
    QListWidgetItem* soon = rowOf(pane, QStringLiteral("Alpha soon"));
    QVERIFY(login->checkState() == Qt::Checked);
    QVERIFY(!(login->flags() & Qt::ItemIsUserCheckable));
    QVERIFY(login->text().contains(QStringLiteral("installs with the game, required")));
    QVERIFY(maps->flags() & Qt::ItemIsUserCheckable);
    QVERIFY(maps->checkState() == Qt::Unchecked);
    QVERIFY(maps->text().contains(QStringLiteral("MB download")));
    QVERIFY(!(soon->flags() & Qt::ItemIsUserCheckable));
    QVERIFY(soon->text().contains(QStringLiteral("No version published yet")));
    QVERIFY(!buttonLabelled(pane, QStringLiteral("Apply changes"))->isVisibleTo(&pane));
    QVERIFY(pane.chosenOptionalPlugins().isEmpty());

    QSignalSpy changed(&pane, &PluginsPane::choiceChanged);
    maps->setCheckState(Qt::Checked);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(pane.chosenOptionalPlugins(), QStringList{QStringLiteral("alpha-maps")});
    QVERIFY(maps->text().contains(QStringLiteral("installs with the game")));
    QVERIFY(buttonLabelled(pane, QStringLiteral("Reset"))->isEnabled());
    buttonLabelled(pane, QStringLiteral("Reset"))->click();
    QVERIFY(pane.chosenOptionalPlugins().isEmpty());
  }

  void afterAnInstallTickingOnlyStagesAChangeUntilApplyIsPressed()
  {
    PluginsPane pane;
    pane.showPlugins(editionPlugins(), true, {QStringLiteral("alpha-login"), QStringLiteral("alpha-maps")}, QString(), false);
    QVERIFY(rowOf(pane, QStringLiteral("Alpha login"))->text().contains(QStringLiteral("installed, required")));
    QVERIFY(rowOf(pane, QStringLiteral("Alpha maps"))->checkState() == Qt::Checked);
    QVERIFY(rowOf(pane, QStringLiteral("Alpha maps"))->text().contains(QStringLiteral("installed")));
    QPushButton* apply = buttonLabelled(pane, QStringLiteral("Apply changes"));
    QVERIFY(apply->isVisibleTo(&pane));
    QVERIFY(!apply->isEnabled());

    QSignalSpy applied(&pane, &PluginsPane::applyRequested);
    rowOf(pane, QStringLiteral("Alpha maps"))->setCheckState(Qt::Unchecked);
    rowOf(pane, QStringLiteral("Alpha bars"))->setCheckState(Qt::Checked);
    QVERIFY(rowOf(pane, QStringLiteral("Alpha maps"))->text().contains(QStringLiteral("will be removed")));
    QVERIFY(rowOf(pane, QStringLiteral("Alpha bars"))->text().contains(QStringLiteral("will be installed")));
    QVERIFY(apply->isEnabled());
    QCOMPARE(applied.count(), 0);
    apply->click();
    QCOMPARE(applied.count(), 1);
    QCOMPARE(applied.first().first().toStringList(), QStringList{QStringLiteral("alpha-bars")});

    buttonLabelled(pane, QStringLiteral("Reset"))->click();
    QVERIFY(rowOf(pane, QStringLiteral("Alpha maps"))->checkState() == Qt::Checked);
    QVERIFY(rowOf(pane, QStringLiteral("Alpha bars"))->checkState() == Qt::Unchecked);
    QVERIFY(!apply->isEnabled());
  }

  void aRefreshWithTheSameFactsKeepsWhatYouTickedButNewFactsStartOver()
  {
    PluginsPane pane;
    pane.showPlugins(editionPlugins(), true, {QStringLiteral("alpha-login")}, QString(), false);
    rowOf(pane, QStringLiteral("Alpha maps"))->setCheckState(Qt::Checked);
    pane.showPlugins(editionPlugins(), true, {QStringLiteral("alpha-login")}, QString(), false);
    QVERIFY(rowOf(pane, QStringLiteral("Alpha maps"))->checkState() == Qt::Checked);
    pane.showPlugins(editionPlugins(), true, {QStringLiteral("alpha-login"), QStringLiteral("alpha-bars")}, QString(), false);
    QVERIFY(rowOf(pane, QStringLiteral("Alpha bars"))->checkState() == Qt::Checked);
    QVERIFY(rowOf(pane, QStringLiteral("Alpha maps"))->checkState() == Qt::Unchecked);
  }

  void whileAnInstallRunsNothingCanBeTickedOrApplied()
  {
    PluginsPane pane;
    pane.showPlugins(editionPlugins(), true, {QStringLiteral("alpha-login")}, QStringLiteral("Plugins cannot change while an install runs."), false);
    QListWidget* list = listOf(pane);
    QCOMPARE(list->count(), 7);
    for (int row = 0; row < list->count(); ++row) {
      QVERIFY(!(list->item(row)->flags() & Qt::ItemIsUserCheckable));
    }
    QVERIFY(!buttonLabelled(pane, QStringLiteral("Apply changes"))->isEnabled());
    QCOMPARE(buttonLabelled(pane, QStringLiteral("Apply changes"))->toolTip(), QStringLiteral("Plugins cannot change while an install runs."));
    QVERIFY(labelWith(pane, QStringLiteral("Plugins cannot change while an install runs.")) != nullptr);
  }

  void theHelpBoxFollowsTheSelectionAndAsksForAScreenshotOnce()
  {
    PluginsPane pane;
    QSignalSpy wanted(&pane, &PluginsPane::screenshotWanted);
    pane.showPlugins(editionPlugins(), false, {}, QString(), false);
    QVERIFY(labelWith(pane, QStringLiteral("Adds zone maps.")) != nullptr);
    QVERIFY(labelWith(pane, QStringLiteral("Loading screenshot...")) != nullptr);
    QCOMPARE(wanted.count(), 1);
    QCOMPARE(wanted.first().first().value<Package>().name, QStringLiteral("alpha-maps"));

    listOf(pane)->setCurrentItem(rowOf(pane, QStringLiteral("Alpha login")));
    QVERIFY(labelWith(pane, QStringLiteral("Signs you in to modern servers.")) != nullptr);
    QVERIFY(labelWith(pane, QStringLiteral("Required plugin  ")) != nullptr);
    QVERIFY(labelWith(pane, QStringLiteral("Required plugin  "))->text().contains(QStringLiteral("Login")));
    listOf(pane)->setCurrentItem(rowOf(pane, QStringLiteral("Alpha bars")));
    QVERIFY(labelWith(pane, QStringLiteral("No description yet.")) != nullptr);
    listOf(pane)->setCurrentItem(rowOf(pane, QStringLiteral("Alpha maps")));
    QCOMPARE(wanted.count(), 1);

    pane.showScreenshot(QStringLiteral("alpha-maps"), pngImage());
    QVERIFY(labelWith(pane, QStringLiteral("Loading screenshot...")) == nullptr);
    bool hasPicture = false;
    for (QLabel* label : pane.findChildren<QLabel*>()) {
      hasPicture = hasPicture || !label->pixmap().isNull();
    }
    QVERIFY(hasPicture);
  }

  void aScreenshotThatCannotBeShownSaysSo()
  {
    PluginsPane pane;
    pane.showPlugins(editionPlugins(), false, {}, QString(), false);
    listOf(pane)->setCurrentItem(rowOf(pane, QStringLiteral("Alpha maps")));
    pane.showScreenshotProblem(QStringLiteral("alpha-maps"), QStringLiteral("timed out"));
    QVERIFY(labelWith(pane, QStringLiteral("The screenshot could not be loaded: timed out")) != nullptr);
    pane.showScreenshot(QStringLiteral("alpha-bars"), QByteArray("not an image"));
    listOf(pane)->setCurrentItem(rowOf(pane, QStringLiteral("Alpha maps")));
    QVERIFY(labelWith(pane, QStringLiteral("The screenshot could not be loaded: timed out")) != nullptr);
  }

  void anEditionWithoutPluginsSaysSo()
  {
    PluginsPane pane;
    pane.showPlugins({}, true, {}, QString(), false);
    QVERIFY(labelWith(pane, QStringLiteral("This edition has no plugins.")) != nullptr);
    QCOMPARE(listOf(pane)->count(), 0);
  }

  void aShorterListReplacesALongerOneWhateverRowWasSelected()
  {
    PluginsPane pane;
    pane.showPlugins(editionPlugins(), false, {}, QString(), false);
    listOf(pane)->setCurrentItem(rowOf(pane, QStringLiteral("Alpha soon")));
    pane.showPlugins({plugin(QStringLiteral("beta-login"), QStringLiteral("Beta login"), QStringLiteral("requiredPlugin"),
      QStringLiteral("Login"), QStringLiteral("Starts the beta client."))}, false, {}, QString(), false);
    QCOMPARE(rowTitles(pane), QStringList({QStringLiteral("Login"), QStringLiteral("Beta login")}));
    QCOMPARE(listOf(pane)->currentItem(), rowOf(pane, QStringLiteral("Beta login")));
    QVERIFY(labelWith(pane, QStringLiteral("Starts the beta client.")) != nullptr);

    pane.showPlugins({}, false, {}, QString(), false);
    QCOMPARE(listOf(pane)->count(), 0);
    QVERIFY(labelWith(pane, QStringLiteral("This edition has no plugins.")) != nullptr);
    QVERIFY(labelWith(pane, QStringLiteral("Select a plugin to see what it does.")) != nullptr);
    QVERIFY(labelWith(pane, QStringLiteral("Starts the beta client.")) == nullptr);
  }

  void theSelectedPluginStaysSelectedWhenItsRowMoves()
  {
    PluginsPane pane;
    pane.showPlugins(editionPlugins(), false, {}, QString(), false);
    listOf(pane)->setCurrentItem(rowOf(pane, QStringLiteral("Alpha bars")));
    const QList<Package> shown = editionPlugins();
    pane.showPlugins({shown.at(2), shown.at(0), shown.at(1)}, false, {}, QString(), false);
    QCOMPARE(rowTitles(pane), QStringList({QStringLiteral("Display"), QStringLiteral("Alpha bars"), QStringLiteral("Alpha maps"),
      QStringLiteral("Login"), QStringLiteral("Alpha login")}));
    QCOMPARE(listOf(pane)->currentItem(), rowOf(pane, QStringLiteral("Alpha bars")));
  }

  void renderedPaneCanBeSavedForReview()
  {
    const QString target = qEnvironmentVariable("ANKH_PLUGINS_PANE_PNG");
    if (target.isEmpty()) {
      QSKIP("Set ANKH_PLUGINS_PANE_PNG to save a picture of the pane.");
    }
    PluginsPane pane;
    pane.resize(1100, 560);
    pane.showPlugins(editionPlugins(), true, {QStringLiteral("alpha-login"), QStringLiteral("alpha-maps")}, QString(), true);
    rowOf(pane, QStringLiteral("Alpha bars"))->setCheckState(Qt::Checked);
    listOf(pane)->setCurrentItem(rowOf(pane, QStringLiteral("Alpha maps")));
    QImage image(400, 250, QImage::Format_RGB32);
    for (int y = 0; y < image.height(); ++y) {
      for (int x = 0; x < image.width(); ++x) {
        image.setPixel(x, y, qRgb(x * 255 / image.width(), y * 255 / image.height(), 140));
      }
    }
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    pane.showScreenshot(QStringLiteral("alpha-maps"), bytes);
    pane.show();
    QVERIFY(pane.grab().save(target));
  }
};

QTEST_MAIN(PluginsPaneTest)
#include "pluginsPaneTest.moc"
