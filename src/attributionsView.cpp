#include "attributionsView.h"

#include "fatal.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

QByteArray readResource(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    fatal(QStringLiteral("Missing resource: %1").arg(path));
  }
  return file.readAll();
}

QLabel* secondaryLabel(const QString& text, QWidget* parent)
{
  auto* label = new QLabel(text, parent);
  label->setProperty("secondary", true);
  label->setWordWrap(true);
  return label;
}

}

AttributionsView::AttributionsView(QWidget* parent)
  : QWidget(parent)
{
  const QJsonDocument document = QJsonDocument::fromJson(readResource(QStringLiteral(":/thirdParty.json")));
  if (!document.isArray()) {
    fatal(QStringLiteral("thirdParty.json is not an array"));
  }

  auto* heading = new QLabel(QStringLiteral("Attributions"), this);
  heading->setProperty("heading", true);
  auto* intro = secondaryLabel(QStringLiteral("Ankh includes the components below. Each entry lists its license; the full text is under Show license text."), this);
  auto* filter = new QLineEdit(this);
  filter->setPlaceholderText(QStringLiteral("Filter components"));
  filter->setClearButtonEnabled(true);
  filter->setAccessibleName(QStringLiteral("Filter components"));

  auto* content = new QWidget(this);
  auto* contentLayout = new QVBoxLayout(content);
  contentLayout->setContentsMargins(0, 8, 12, 8);
  contentLayout->setSpacing(24);

  QList<QPair<QString, QWidget*>> entries;
  for (const QJsonValue& value : document.array()) {
    const QJsonObject entry = value.toObject();
    const QString name = entry.value(QStringLiteral("name")).toString();
    if (name.isEmpty()) {
      fatal(QStringLiteral("thirdParty.json entry has no name"));
    }
    auto* card = new QWidget(content);
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(4);

    QString title = name;
    if (entry.contains(QStringLiteral("version"))) {
      title += QStringLiteral("  ") + entry.value(QStringLiteral("version")).toString();
    }
    auto* titleLabel = new QLabel(title, card);
    titleLabel->setProperty("sectionHeading", true);
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(secondaryLabel(entry.value(QStringLiteral("use")).toString(), card));
    cardLayout->addWidget(new QLabel(entry.value(QStringLiteral("license")).toString(), card));
    cardLayout->addWidget(secondaryLabel(entry.value(QStringLiteral("copyright")).toString(), card));
    const QString note = entry.value(QStringLiteral("note")).toString();
    if (!note.isEmpty()) {
      cardLayout->addWidget(secondaryLabel(note, card));
    }
    auto* source = new QLabel(QStringLiteral("<a href=\"%1\">%1</a>").arg(entry.value(QStringLiteral("source")).toString()), card);
    source->setOpenExternalLinks(true);
    source->setTextInteractionFlags(Qt::TextBrowserInteraction);
    cardLayout->addWidget(source);

    const QJsonArray files = entry.value(QStringLiteral("licenseFiles")).toArray();
    if (!files.isEmpty()) {
      QString text;
      for (const QJsonValue& file : files) {
        if (!text.isEmpty()) {
          text += QStringLiteral("\n\n----------------------------------------\n\n");
        }
        text += QString::fromUtf8(readResource(QStringLiteral(":/") + file.toString()));
      }
      auto* toggle = new QPushButton(QStringLiteral("Show license text"), card);
      toggle->setProperty("link", true);
      toggle->setCheckable(true);
      toggle->setCursor(Qt::PointingHandCursor);
      auto* license = new QPlainTextEdit(card);
      license->setReadOnly(true);
      license->setPlainText(text);
      license->setFixedHeight(260);
      license->setVisible(false);
      license->setAccessibleName(name + QStringLiteral(" license text"));
      cardLayout->addWidget(toggle, 0, Qt::AlignLeft);
      cardLayout->addWidget(license);
      connect(toggle, &QPushButton::toggled, this, [toggle, license](bool isShown) {
        license->setVisible(isShown);
        toggle->setText(isShown ? QStringLiteral("Hide license text") : QStringLiteral("Show license text"));
      });
    }
    contentLayout->addWidget(card);
    entries.append({name, card});
  }
  contentLayout->addStretch(1);

  auto* scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setWidget(content);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(32, 28, 32, 0);
  layout->setSpacing(12);
  layout->addWidget(heading);
  layout->addWidget(intro);
  layout->addWidget(filter);
  layout->addWidget(scroll, 1);

  connect(filter, &QLineEdit::textChanged, this, [entries](const QString& text) {
    for (const auto& entry : entries) {
      entry.second->setVisible(text.trimmed().isEmpty() || entry.first.contains(text.trimmed(), Qt::CaseInsensitive));
    }
  });
}
