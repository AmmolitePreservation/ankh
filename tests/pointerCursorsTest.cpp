#include "pointerCursors.h"

#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabBar>
#include <QtTest>

class PointerCursorsTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase()
  {
    filter = new PointerCursors(this);
    qApp->installEventFilter(filter);
  }

  void enabledButtonGetsTheHand()
  {
    QPushButton button;
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&button, &enter);
    QCOMPARE(button.cursor().shape(), Qt::PointingHandCursor);
  }

  void disabledButtonKeepsTheArrow()
  {
    QPushButton button;
    button.setEnabled(false);
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&button, &enter);
    QCOMPARE(button.cursor().shape(), Qt::ArrowCursor);
  }

  void buttonLosesTheHandWhenDisabled()
  {
    QPushButton button;
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&button, &enter);
    button.setEnabled(false);
    QCOMPARE(button.cursor().shape(), Qt::ArrowCursor);
  }

  void comboBoxAndTabBarGetTheHand()
  {
    QComboBox combo;
    QTabBar tabs;
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&combo, &enter);
    QCoreApplication::sendEvent(&tabs, &enter);
    QCOMPARE(combo.cursor().shape(), Qt::PointingHandCursor);
    QCOMPARE(tabs.cursor().shape(), Qt::PointingHandCursor);
  }

  void textFieldAndLabelAreLeftAlone()
  {
    QLineEdit field;
    QLabel label;
    const Qt::CursorShape fieldBefore = field.cursor().shape();
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&field, &enter);
    QCoreApplication::sendEvent(&label, &enter);
    QCOMPARE(field.cursor().shape(), fieldBefore);
    QCOMPARE(label.cursor().shape(), Qt::ArrowCursor);
  }

  void systemCursorButtonKeepsTheArrow()
  {
    QPushButton button;
    button.setProperty("systemCursor", true);
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&button, &enter);
    QCOMPARE(button.cursor().shape(), Qt::ArrowCursor);
  }

private:
  PointerCursors* filter = nullptr;
};

QTEST_MAIN(PointerCursorsTest)
#include "pointerCursorsTest.moc"
