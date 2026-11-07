#include "libraryPane.h"

#include "gameCatalog.h"

#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

namespace {

enum Role {
  idRole = Qt::UserRole + 1,
  titleRole,
  editionRole,
  initialsRole,
  tintRole,
  favoriteRole,
  installedRole,
  sortKeyRole,
};

constexpr int rowHeight = 64;

QRect starRectFor(const QRect& row)
{
  return QRect(row.right() - 38, row.top() + (rowHeight - 24) / 2, 24, 24);
}

}

class GameFilterProxy : public QSortFilterProxyModel {
public:
  void setSearchText(const QString& text)
  {
    searchText = text.trimmed();
    invalidate();
  }

  void setMode(int newMode)
  {
    mode = newMode;
    invalidate();
  }

protected:
  bool filterAcceptsRow(int row, const QModelIndex& parent) const override
  {
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    if (mode == 1 && !index.data(installedRole).toBool()) {
      return false;
    }
    if (mode == 2 && !index.data(favoriteRole).toBool()) {
      return false;
    }
    if (searchText.isEmpty()) {
      return true;
    }
    return index.data(titleRole).toString().contains(searchText, Qt::CaseInsensitive)
      || index.data(editionRole).toString().contains(searchText, Qt::CaseInsensitive);
  }

private:
  QString searchText;
  int mode = 0;
};

class GameDelegate : public QStyledItemDelegate {
public:
  explicit GameDelegate(const Skin& skin, QObject* parent)
    : QStyledItemDelegate(parent), skin(skin)
  {
  }

  void setSkin(const Skin& newSkin)
  {
    skin = newSkin;
  }

  QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override
  {
    return QSize(0, rowHeight);
  }

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
  {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    const QRect row = option.rect;
    if (option.state & QStyle::State_Selected) {
      painter->fillRect(row, skin.border);
    } else if (option.state & QStyle::State_MouseOver) {
      QColor hover = skin.border;
      hover.setAlpha(110);
      painter->fillRect(row, hover);
    }
    const QRect iconRect(row.left() + 14, row.top() + (rowHeight - 40) / 2, 40, 40);
    paintGameIcon(*painter, iconRect, index.data(initialsRole).toString(), index.data(tintRole).value<QColor>());

    QFont titleFont = painter->font();
    titleFont.setPixelSize(14);
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->setPen(skin.text);
    const int textLeft = iconRect.right() + 14;
    const int textWidth = starRectFor(row).left() - textLeft - 4;
    const QFontMetrics titleMetrics(titleFont);
    painter->drawText(QRect(textLeft, row.top() + 10, textWidth, 22), Qt::AlignVCenter | Qt::AlignLeft,
      titleMetrics.elidedText(index.data(titleRole).toString(), Qt::ElideRight, textWidth));

    QFont editionFont = painter->font();
    editionFont.setPixelSize(13);
    editionFont.setBold(false);
    painter->setFont(editionFont);
    painter->setPen(skin.secondary);
    const QFontMetrics editionMetrics(editionFont);
    painter->drawText(QRect(textLeft, row.top() + 32, textWidth, 20), Qt::AlignVCenter | Qt::AlignLeft,
      editionMetrics.elidedText(index.data(editionRole).toString(), Qt::ElideRight, textWidth));

    const bool isFavorite = index.data(favoriteRole).toBool();
    QFont starFont = painter->font();
    starFont.setPixelSize(18);
    painter->setFont(starFont);
    painter->setPen(isFavorite ? skin.accent : skin.secondary);
    painter->drawText(starRectFor(row), Qt::AlignCenter, QString(QChar(isFavorite ? 0x2605 : 0x2606)));

    if (option.state & QStyle::State_HasFocus) {
      painter->setPen(QPen(skin.accent, 1));
      painter->setBrush(Qt::NoBrush);
      painter->drawRect(row.adjusted(0, 0, -1, -1));
    }
    painter->restore();
  }

  bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override
  {
    if (event->type() == QEvent::MouseButtonRelease) {
      const auto* mouse = static_cast<QMouseEvent*>(event);
      if (mouse->button() == Qt::LeftButton && starRectFor(option.rect).contains(mouse->position().toPoint())) {
        model->setData(index, !index.data(favoriteRole).toBool(), favoriteRole);
        return true;
      }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
  }

private:
  Skin skin;
};

LibraryPane::LibraryPane(QSettings& settings, QWidget* parent)
  : QWidget(parent),
    settings(settings),
    model(new QStandardItemModel(this)),
    proxy(new GameFilterProxy),
    delegate(new GameDelegate(skinNamed(QStringLiteral("Ankh")), this)),
    search(new QLineEdit(this)),
    filterChoice(new QComboBox(this)),
    sortChoice(new QComboBox(this)),
    countLabel(new QLabel(this)),
    emptyLabel(new QLabel(QStringLiteral("No games match."), this)),
    list(new QListView(this))
{
  proxy->setParent(this);
  const QStringList favorites = settings.value(QStringLiteral("library/favorites")).toStringList();
  for (const Game& game : gameCatalog()) {
    auto* item = new QStandardItem;
    item->setData(game.id, idRole);
    item->setData(game.title, titleRole);
    item->setData(game.edition, editionRole);
    item->setData(game.initials, initialsRole);
    item->setData(game.tint, tintRole);
    item->setData(favorites.contains(game.id), favoriteRole);
    item->setData(game.isInstalled, installedRole);
    item->setData(game.title.toLower() + QLatin1Char(' ') + game.edition.toLower(), sortKeyRole);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    model->appendRow(item);
  }
  proxy->setSourceModel(model);
  proxy->setSortRole(sortKeyRole);

  search->setPlaceholderText(QStringLiteral("Search games"));
  search->setClearButtonEnabled(true);
  search->setAccessibleName(QStringLiteral("Search games"));
  filterChoice->addItems({QStringLiteral("All"), QStringLiteral("Installed"), QStringLiteral("Favorites")});
  filterChoice->setAccessibleName(QStringLiteral("Filter"));
  sortChoice->addItems({QStringLiteral("Custom"), QStringLiteral("Name")});
  sortChoice->setAccessibleName(QStringLiteral("Sort"));
  countLabel->setProperty("secondary", true);
  emptyLabel->setProperty("secondary", true);
  emptyLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  emptyLabel->setContentsMargins(0, 24, 0, 0);

  list->setModel(proxy);
  list->setItemDelegate(delegate);
  list->setMouseTracking(true);
  list->setSelectionMode(QAbstractItemView::SingleSelection);
  list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  list->setAccessibleName(QStringLiteral("Games"));

  auto* controls = new QHBoxLayout;
  controls->setSpacing(8);
  controls->addWidget(filterChoice, 1);
  controls->addWidget(sortChoice, 1);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 8);
  layout->setSpacing(8);
  layout->addWidget(search);
  layout->addLayout(controls);
  layout->addWidget(countLabel);
  layout->addWidget(list, 1);
  layout->addWidget(emptyLabel, 1);

  connect(search, &QLineEdit::textChanged, this, [this] { applyFilters(); });
  connect(filterChoice, &QComboBox::currentIndexChanged, this, [this] { applyFilters(); });
  connect(sortChoice, &QComboBox::currentIndexChanged, this, [this](int index) {
    proxy->sort(index == 1 ? 0 : -1);
  });
  connect(list->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current) {
    if (current.isValid()) {
      emit gameSelected(current.data(idRole).toString());
    }
  });
  connect(model, &QStandardItemModel::dataChanged, this, [this] {
    saveFavorites();
    applyFilters();
  });
  applyFilters();
}

void LibraryPane::applySkin(const Skin& skin)
{
  delegate->setSkin(skin);
  list->viewport()->update();
}

void LibraryPane::selectGame(const QString& id)
{
  for (int row = 0; row < proxy->rowCount(); ++row) {
    const QModelIndex index = proxy->index(row, 0);
    if (index.data(idRole).toString() == id) {
      list->setCurrentIndex(index);
      return;
    }
  }
}

void LibraryPane::applyFilters()
{
  proxy->setSearchText(search->text());
  proxy->setMode(filterChoice->currentIndex());
  const int shown = proxy->rowCount();
  countLabel->setText(QStringLiteral("%1 of %2").arg(shown).arg(model->rowCount()));
  list->setVisible(shown > 0);
  emptyLabel->setVisible(shown == 0);
}

void LibraryPane::saveFavorites()
{
  QStringList favorites;
  for (int row = 0; row < model->rowCount(); ++row) {
    const QModelIndex index = model->index(row, 0);
    if (index.data(favoriteRole).toBool()) {
      favorites.append(index.data(idRole).toString());
    }
  }
  settings.setValue(QStringLiteral("library/favorites"), favorites);
}
