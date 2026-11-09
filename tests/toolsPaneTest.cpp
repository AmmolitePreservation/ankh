#include "palette.h"
#include "toolsPane.h"

#include <QBuffer>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest>

namespace {

Package tool(const QString& name, const QString& label, const QString& role, const QString& description, bool hasScreenshot = false,
  bool isPublished = true)
{
  Package package{name, label, role, {}, std::nullopt, description, std::nullopt};
  if (isPublished) {
    package.latest = PackageVersion{QStringLiteral("2026.11.03"), name + QStringLiteral(".zip"), 3 * 1024 * 1024, QStringLiteral("abc"),
      QStringLiteral("2026-11-03T00:00:00Z"), 2, 419, QUrl(QStringLiteral("https://ammolite.test/files/1/%1.zip").arg(name))};
  }
  if (hasScreenshot) {
    package.screenshot = PackageScreenshot{QUrl(QStringLiteral("https://ammolite.test/shot")), QStringLiteral("def"), QStringLiteral("image/png")};
  }
  return package;
}

QList<Package> editionTools()
{
  return {
    tool(QStringLiteral("alpha-tools"), QStringLiteral("Alpha tools"), QStringLiteral("requiredTool"), QStringLiteral("Lets the client reach modern servers.")),
    tool(QStringLiteral("alpha-maps"), QStringLiteral("Alpha maps"), QStringLiteral("optionalTool"), QStringLiteral("Adds zone maps."), true),
    tool(QStringLiteral("alpha-bars"), QStringLiteral("Alpha bars"), QStringLiteral("optionalTool"), QString()),
    tool(QStringLiteral("alpha-soon"), QStringLiteral("Alpha soon"), QStringLiteral("optionalTool"), QString(), false, false),
  };
}

QListWidget* listOf(ToolsPane& pane)
{
  return pane.findChild<QListWidget*>(QStringLiteral("toolList"));
}

QPushButton* buttonLabelled(ToolsPane& pane, const QString& text)
{
  for (QPushButton* button : pane.findChildren<QPushButton*>()) {
    if (button->text() == text) {
      return button;
    }
  }
  return nullptr;
}

QLabel* labelWith(ToolsPane& pane, const QString& text)
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

class ToolsPaneTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase()
  {
    qRegisterMetaType<Package>();
    qApp->setStyleSheet(buildStyleSheet(ankhPalette()));
  }

  void beforeAnInstallRequiredToolsAreTickedAndLockedAndOptionalOnesAreYoursToTick()
  {
    ToolsPane pane;
    pane.showTools(editionTools(), false, {}, QString());
    QListWidget* list = listOf(pane);
    QCOMPARE(list->count(), 4);
    QVERIFY(list->item(0)->checkState() == Qt::Checked);
    QVERIFY(!(list->item(0)->flags() & Qt::ItemIsUserCheckable));
    QVERIFY(list->item(0)->text().contains(QStringLiteral("installs with the game, required")));
    QVERIFY(list->item(1)->flags() & Qt::ItemIsUserCheckable);
    QVERIFY(list->item(1)->checkState() == Qt::Unchecked);
    QVERIFY(list->item(1)->text().contains(QStringLiteral("MB download")));
    QVERIFY(!(list->item(3)->flags() & Qt::ItemIsUserCheckable));
    QVERIFY(list->item(3)->text().contains(QStringLiteral("No version published yet")));
    QVERIFY(!buttonLabelled(pane, QStringLiteral("Apply changes"))->isVisibleTo(&pane));
    QVERIFY(pane.chosenOptionalTools().isEmpty());

    QSignalSpy changed(&pane, &ToolsPane::choiceChanged);
    list->item(1)->setCheckState(Qt::Checked);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(pane.chosenOptionalTools(), QStringList{QStringLiteral("alpha-maps")});
    QVERIFY(list->item(1)->text().contains(QStringLiteral("installs with the game")));
    QVERIFY(buttonLabelled(pane, QStringLiteral("Reset"))->isEnabled());
    buttonLabelled(pane, QStringLiteral("Reset"))->click();
    QVERIFY(pane.chosenOptionalTools().isEmpty());
  }

  void afterAnInstallTickingOnlyStagesAChangeUntilApplyIsPressed()
  {
    ToolsPane pane;
    pane.showTools(editionTools(), true, {QStringLiteral("alpha-tools"), QStringLiteral("alpha-maps")}, QString());
    QListWidget* list = listOf(pane);
    QVERIFY(list->item(0)->text().contains(QStringLiteral("installed, required")));
    QVERIFY(list->item(1)->checkState() == Qt::Checked);
    QVERIFY(list->item(1)->text().contains(QStringLiteral("installed")));
    QPushButton* apply = buttonLabelled(pane, QStringLiteral("Apply changes"));
    QVERIFY(apply->isVisibleTo(&pane));
    QVERIFY(!apply->isEnabled());

    QSignalSpy applied(&pane, &ToolsPane::applyRequested);
    list->item(1)->setCheckState(Qt::Unchecked);
    list->item(2)->setCheckState(Qt::Checked);
    QVERIFY(list->item(1)->text().contains(QStringLiteral("will be removed")));
    QVERIFY(list->item(2)->text().contains(QStringLiteral("will be installed")));
    QVERIFY(apply->isEnabled());
    QCOMPARE(applied.count(), 0);
    apply->click();
    QCOMPARE(applied.count(), 1);
    QCOMPARE(applied.first().first().toStringList(), QStringList{QStringLiteral("alpha-bars")});

    buttonLabelled(pane, QStringLiteral("Reset"))->click();
    QVERIFY(list->item(1)->checkState() == Qt::Checked);
    QVERIFY(list->item(2)->checkState() == Qt::Unchecked);
    QVERIFY(!apply->isEnabled());
  }

  void aRefreshWithTheSameFactsKeepsWhatYouTickedButNewFactsStartOver()
  {
    ToolsPane pane;
    pane.showTools(editionTools(), true, {QStringLiteral("alpha-tools")}, QString());
    listOf(pane)->item(2)->setCheckState(Qt::Checked);
    pane.showTools(editionTools(), true, {QStringLiteral("alpha-tools")}, QString());
    QVERIFY(listOf(pane)->item(2)->checkState() == Qt::Checked);
    pane.showTools(editionTools(), true, {QStringLiteral("alpha-tools"), QStringLiteral("alpha-bars")}, QString());
    QVERIFY(listOf(pane)->item(2)->checkState() == Qt::Checked);
    QVERIFY(listOf(pane)->item(1)->checkState() == Qt::Unchecked);
  }

  void whileAnInstallRunsNothingCanBeTickedOrApplied()
  {
    ToolsPane pane;
    pane.showTools(editionTools(), true, {QStringLiteral("alpha-tools")}, QStringLiteral("Tools cannot change while an install runs."));
    QListWidget* list = listOf(pane);
    for (int row = 0; row < list->count(); ++row) {
      QVERIFY(!(list->item(row)->flags() & Qt::ItemIsUserCheckable));
    }
    QVERIFY(!buttonLabelled(pane, QStringLiteral("Apply changes"))->isEnabled());
    QCOMPARE(buttonLabelled(pane, QStringLiteral("Apply changes"))->toolTip(), QStringLiteral("Tools cannot change while an install runs."));
    QVERIFY(labelWith(pane, QStringLiteral("Tools cannot change while an install runs.")) != nullptr);
  }

  void theHelpBoxFollowsTheSelectionAndAsksForAScreenshotOnce()
  {
    ToolsPane pane;
    pane.showTools(editionTools(), false, {}, QString());
    QSignalSpy wanted(&pane, &ToolsPane::screenshotWanted);
    QVERIFY(labelWith(pane, QStringLiteral("Lets the client reach modern servers.")) != nullptr);
    QCOMPARE(wanted.count(), 0);

    listOf(pane)->setCurrentRow(1);
    QVERIFY(labelWith(pane, QStringLiteral("Adds zone maps.")) != nullptr);
    QVERIFY(labelWith(pane, QStringLiteral("Loading screenshot...")) != nullptr);
    QCOMPARE(wanted.count(), 1);
    QCOMPARE(wanted.first().first().value<Package>().name, QStringLiteral("alpha-maps"));

    listOf(pane)->setCurrentRow(2);
    QVERIFY(labelWith(pane, QStringLiteral("No description yet.")) != nullptr);
    listOf(pane)->setCurrentRow(1);
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
    ToolsPane pane;
    pane.showTools(editionTools(), false, {}, QString());
    listOf(pane)->setCurrentRow(1);
    pane.showScreenshotProblem(QStringLiteral("alpha-maps"), QStringLiteral("timed out"));
    QVERIFY(labelWith(pane, QStringLiteral("The screenshot could not be loaded: timed out")) != nullptr);
    pane.showScreenshot(QStringLiteral("alpha-bars"), QByteArray("not an image"));
    listOf(pane)->setCurrentRow(1);
    QVERIFY(labelWith(pane, QStringLiteral("The screenshot could not be loaded: timed out")) != nullptr);
  }

  void anEditionWithoutToolsSaysSo()
  {
    ToolsPane pane;
    pane.showTools({}, true, {}, QString());
    QVERIFY(labelWith(pane, QStringLiteral("This edition has no tools.")) != nullptr);
    QCOMPARE(listOf(pane)->count(), 0);
  }

  void renderedPaneCanBeSavedForReview()
  {
    const QString target = qEnvironmentVariable("ANKH_TOOLS_PANE_PNG");
    if (target.isEmpty()) {
      QSKIP("Set ANKH_TOOLS_PANE_PNG to save a picture of the pane.");
    }
    ToolsPane pane;
    pane.resize(1100, 560);
    pane.showTools(editionTools(), true, {QStringLiteral("alpha-tools"), QStringLiteral("alpha-maps")}, QString());
    listOf(pane)->item(2)->setCheckState(Qt::Checked);
    listOf(pane)->setCurrentRow(1);
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

QTEST_MAIN(ToolsPaneTest)
#include "toolsPaneTest.moc"
