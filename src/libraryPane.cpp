#include "libraryPane.h"

#include "gameCatalog.h"
#include "icons.h"

#include <QAction>
#include <QActionGroup>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
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
  nameKeyRole,
  installKeyRole,
};

constexpr int rowHeight = 78;
constexpr int iconSize = 56;

QRect starRectFor(const QRect& row)
{
  return QRect(row.right() - 44, row.top() + (rowHeight - 28) / 2, 28, 28);
}

}

class GameFilterProxy : public QSortFilterProxyModel {
public:
  void setSearchText(const QString& text)
  {
    searchText = text.trimmed();
    invalidate();
  }

  void setFavoritesOnly(bool isOnly)
  {
    favoritesOnly = isOnly;
    invalidate();
  }

  void setInstalledOnly(bool isOnly)
  {
    installedOnly = isOnly;
    invalidate();
  }

protected:
  bool filterAcceptsRow(int row, const QModelIndex& parent) const override
  {
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    if (installedOnly && !index.data(installedRole).toBool()) {
      return false;
    }
    if (favoritesOnly && !index.data(favoriteRole).toBool()) {
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
  bool favoritesOnly = false;
  bool installedOnly = false;
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
    const bool isSelected = option.state & QStyle::State_Selected;
    if (isSelected) {
      painter->setPen(QPen(skin.accent, 1));
      painter->setBrush(skin.base.lighter(125));
      painter->drawRoundedRect(QRectF(row.adjusted(2, 3, -3, -4)).translated(0.5, 0.5), 6, 6);
    } else {
      if (option.state & QStyle::State_MouseOver) {
        QColor hover = skin.border;
        hover.setAlpha(90);
        painter->fillRect(row.adjusted(0, 0, 0, -1), hover);
      }
      painter->setPen(skin.border);
      painter->drawLine(row.left() + 8, row.bottom(), row.right() - 8, row.bottom());
    }
    const QRect iconRect(row.left() + 14, row.top() + (rowHeight - iconSize) / 2, iconSize, iconSize);
    paintGameIcon(*painter, iconRect, index.data(initialsRole).toString(), index.data(tintRole).value<QColor>());

    const int textLeft = iconRect.right() + 18;
    const int textWidth = starRectFor(row).left() - textLeft - 4;
    QFont titleFont = painter->font();
    titleFont.setPixelSize(17);
    titleFont.setWeight(QFont::DemiBold);
    painter->setFont(titleFont);
    painter->setPen(skin.text);
    painter->drawText(QRect(textLeft, row.top() + 15, textWidth, 24), Qt::AlignVCenter | Qt::AlignLeft,
      QFontMetrics(titleFont).elidedText(index.data(titleRole).toString(), Qt::ElideRight, textWidth));

    QFont editionFont = painter->font();
    editionFont.setPixelSize(15);
    editionFont.setWeight(QFont::Normal);
    painter->setFont(editionFont);
    painter->setPen(skin.secondary);
    painter->drawText(QRect(textLeft, row.top() + 40, textWidth, 22), Qt::AlignVCenter | Qt::AlignLeft,
      QFontMetrics(editionFont).elidedText(index.data(editionRole).toString(), Qt::ElideRight, textWidth));

    const bool isFavorite = index.data(favoriteRole).toBool();
    QFont starFont = painter->font();
    starFont.setPixelSize(22);
    painter->setFont(starFont);
    painter->setPen(isFavorite ? skin.accent : skin.secondary);
    painter->drawText(starRectFor(row), Qt::AlignCenter, QString(QChar(isFavorite ? 0x2605 : 0x2606)));
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
    countLabel(new QLabel(this)),
    search(new QLineEdit(this)),
    searchAction(new QAction(this)),
    filterMenuAction(new QAction(QStringLiteral("Filter and sort"), this)),
    filterMenu(new QMenu(this)),
    installedOnlyAction(filterMenu->addAction(QStringLiteral("Installed only"))),
    sortGroup(new QActionGroup(this)),
    customSortAction(nullptr),
    nameSortAction(nullptr),
    installSortAction(nullptr),
    allChip(new QPushButton(QStringLiteral("All games"), this)),
    favoritesChip(new QPushButton(QStringLiteral("Favorites"), this)),
    nameChip(new QPushButton(QStringLiteral("Name A-Z"), this)),
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
    const QString nameKey = game.title.toLower() + QLatin1Char(' ') + game.edition.toLower();
    item->setData(nameKey, nameKeyRole);
    item->setData((game.isInstalled ? QStringLiteral("0 ") : QStringLiteral("1 ")) + nameKey, installKeyRole);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    model->appendRow(item);
  }
  proxy->setSourceModel(model);

  auto* heading = new QLabel(QStringLiteral("Library"), this);
  heading->setProperty("paneHeading", true);
  countLabel->setProperty("secondary", true);

  search->setPlaceholderText(QStringLiteral("Search games..."));
  search->setAccessibleName(QStringLiteral("Search games"));
  searchAction->setText(QStringLiteral("Search"));
  search->addAction(searchAction, QLineEdit::LeadingPosition);
  search->addAction(filterMenuAction, QLineEdit::TrailingPosition);

  installedOnlyAction->setCheckable(true);
  filterMenu->addSeparator();
  customSortAction = filterMenu->addAction(QStringLiteral("Sort: Custom"));
  nameSortAction = filterMenu->addAction(QStringLiteral("Sort: Name A-Z"));
  installSortAction = filterMenu->addAction(QStringLiteral("Sort: Installed first"));
  for (QAction* action : {customSortAction, nameSortAction, installSortAction}) {
    action->setCheckable(true);
    sortGroup->addAction(action);
  }
  customSortAction->setChecked(true);

  for (QPushButton* chip : {allChip, favoritesChip, nameChip}) {
    chip->setProperty("chip", true);
    chip->setCheckable(true);
    chip->setCursor(Qt::PointingHandCursor);
  }
  allChip->setChecked(true);

  emptyLabel->setProperty("secondary", true);
  emptyLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  emptyLabel->setContentsMargins(0, 24, 0, 0);

  list->setModel(proxy);
  list->setItemDelegate(delegate);
  list->setMouseTracking(true);
  list->setSelectionMode(QAbstractItemView::SingleSelection);
  list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  list->setAccessibleName(QStringLiteral("Games"));

  auto* headingRow = new QHBoxLayout;
  headingRow->addWidget(heading);
  headingRow->addStretch(1);
  headingRow->addWidget(countLabel, 0, Qt::AlignBottom);

  auto* chips = new QHBoxLayout;
  chips->setSpacing(8);
  chips->addWidget(allChip);
  chips->addWidget(favoritesChip);
  chips->addWidget(nameChip);
  chips->addStretch(1);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 20, 24, 8);
  layout->setSpacing(12);
  layout->addLayout(headingRow);
  layout->addWidget(search);
  layout->addLayout(chips);
  layout->addWidget(list, 1);
  layout->addWidget(emptyLabel, 1);

  connect(search, &QLineEdit::textChanged, this, [this] { applyFilters(); });
  connect(filterMenuAction, &QAction::triggered, this, [this] {
    filterMenu->exec(search->mapToGlobal(QPoint(search->width() - filterMenu->sizeHint().width(), search->height())));
  });
  connect(installedOnlyAction, &QAction::toggled, this, [this] { applyFilters(); });
  connect(allChip, &QPushButton::clicked, this, [this] {
    allChip->setChecked(true);
    favoritesChip->setChecked(false);
    applyFilters();
  });
  connect(favoritesChip, &QPushButton::clicked, this, [this] {
    favoritesChip->setChecked(true);
    allChip->setChecked(false);
    applyFilters();
  });
  connect(nameChip, &QPushButton::clicked, this, [this](bool isChecked) {
    setSortMode(isChecked ? SortMode::name : SortMode::custom);
  });
  connect(customSortAction, &QAction::triggered, this, [this] { setSortMode(SortMode::custom); });
  connect(nameSortAction, &QAction::triggered, this, [this] { setSortMode(SortMode::name); });
  connect(installSortAction, &QAction::triggered, this, [this] { setSortMode(SortMode::installState); });
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
  searchAction->setIcon(makeIcon(IconKind::search, 18, skin.secondary));
  filterMenuAction->setIcon(makeIcon(IconKind::chevronDown, 18, skin.secondary));
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
  proxy->setFavoritesOnly(favoritesChip->isChecked());
  proxy->setInstalledOnly(installedOnlyAction->isChecked());
  const int shown = proxy->rowCount();
  const int total = model->rowCount();
  countLabel->setText(shown == total ? QStringLiteral("%1 games").arg(total) : QStringLiteral("%1 of %2 games").arg(shown).arg(total));
  list->setVisible(shown > 0);
  emptyLabel->setVisible(shown == 0);
}

void LibraryPane::setSortMode(SortMode mode)
{
  nameChip->setChecked(mode == SortMode::name);
  customSortAction->setChecked(mode == SortMode::custom);
  nameSortAction->setChecked(mode == SortMode::name);
  installSortAction->setChecked(mode == SortMode::installState);
  if (mode == SortMode::custom) {
    proxy->sort(-1);
    return;
  }
  proxy->setSortRole(mode == SortMode::name ? nameKeyRole : installKeyRole);
  proxy->sort(0);
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
