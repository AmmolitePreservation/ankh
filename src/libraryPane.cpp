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
  return QRect(row.right() - 46, row.top() + (rowHeight - 28) / 2, 28, 28);
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
  explicit GameDelegate(const Palette& colors, QObject* parent)
    : QStyledItemDelegate(parent), colors(colors)
  {
  }

  void setColors(const Palette& newColors)
  {
    colors = newColors;
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
      QColor fill = colors.base;
      fill.setRgb((colors.base.red() * 9 + colors.accent.red()) / 10, (colors.base.green() * 9 + colors.accent.green()) / 10, (colors.base.blue() * 9 + colors.accent.blue()) / 10);
      painter->setPen(QPen(colors.accent, 1));
      painter->setBrush(fill);
      painter->drawRoundedRect(QRectF(row.adjusted(0, 3, -1, -4)).translated(0.5, 0.5), 5, 5);
    } else {
      if (option.state & QStyle::State_MouseOver) {
        QColor hover = colors.border;
        hover.setAlpha(80);
        painter->fillRect(row.adjusted(0, 0, 0, -1), hover);
      }
      painter->setPen(colors.border);
      painter->drawLine(row.left() + 10, row.bottom(), row.right() - 10, row.bottom());
    }
    const QRect iconRect(row.left() + 11, row.top() + (rowHeight - iconSize) / 2, iconSize, iconSize);
    paintGameIcon(*painter, iconRect, index.data(initialsRole).toString(), index.data(tintRole).value<QColor>());

    const int textLeft = iconRect.right() + 20;
    const int textWidth = starRectFor(row).left() - textLeft - 4;
    QFont titleFont = painter->font();
    titleFont.setPixelSize(18);
    titleFont.setWeight(QFont::DemiBold);
    painter->setFont(titleFont);
    painter->setPen(colors.text);
    painter->drawText(QRect(textLeft, row.top() + 14, textWidth, 26), Qt::AlignVCenter | Qt::AlignLeft,
      QFontMetrics(titleFont).elidedText(index.data(titleRole).toString(), Qt::ElideRight, textWidth));

    QFont editionFont = painter->font();
    editionFont.setPixelSize(16);
    editionFont.setWeight(QFont::Normal);
    painter->setFont(editionFont);
    painter->setPen(colors.secondary);
    painter->drawText(QRect(textLeft, row.top() + 40, textWidth, 24), Qt::AlignVCenter | Qt::AlignLeft,
      QFontMetrics(editionFont).elidedText(index.data(editionRole).toString(), Qt::ElideRight, textWidth));

    const bool isFavorite = index.data(favoriteRole).toBool();
    const QRect star = starRectFor(row);
    painter->drawPixmap(star.adjusted(2, 2, -2, -2), iconPixmap(isFavorite ? IconKind::starFilled : IconKind::star, 24, isFavorite ? colors.accent : colors.text, painter->device()->devicePixelRatioF()));
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
  Palette colors;
};

LibraryPane::LibraryPane(QSettings& settings, QWidget* parent)
  : QWidget(parent),
    settings(settings),
    model(new QStandardItemModel(this)),
    proxy(new GameFilterProxy),
    delegate(new GameDelegate(ankhPalette(), this)),
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
    messageBox(new QWidget(this)),
    emptyLabel(new QLabel(this)),
    actionButton(new QPushButton(this)),
    list(new QListView(this))
{
  proxy->setParent(this);
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
    chip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  }
  allChip->setChecked(true);

  emptyLabel->setProperty("secondary", true);
  emptyLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  emptyLabel->setWordWrap(true);
  emptyLabel->setContentsMargins(0, 24, 0, 0);
  auto* messageLayout = new QVBoxLayout(messageBox);
  messageLayout->setContentsMargins(0, 0, 0, 0);
  messageLayout->setSpacing(14);
  messageLayout->addWidget(emptyLabel);
  messageLayout->addWidget(actionButton, 0, Qt::AlignHCenter);
  messageLayout->addStretch(1);

  list->setModel(proxy);
  list->setItemDelegate(delegate);
  list->setMouseTracking(true);
  list->viewport()->installEventFilter(this);
  list->setSelectionMode(QAbstractItemView::SingleSelection);
  list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  list->setAccessibleName(QStringLiteral("Games"));

  auto* headingRow = new QHBoxLayout;
  headingRow->addWidget(heading);
  headingRow->addStretch(1);
  headingRow->addWidget(countLabel, 0, Qt::AlignBottom);

  auto* chips = new QHBoxLayout;
  chips->setSpacing(8);
  chips->addWidget(allChip, allChip->sizeHint().width());
  chips->addWidget(favoritesChip, favoritesChip->sizeHint().width());
  chips->addWidget(nameChip, nameChip->sizeHint().width());

  auto* top = new QWidget(this);
  auto* topLayout = new QVBoxLayout(top);
  topLayout->setContentsMargins(10, 0, 10, 0);
  topLayout->setSpacing(14);
  topLayout->addLayout(headingRow);
  topLayout->addWidget(search);
  topLayout->addLayout(chips);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(14, 22, 14, 8);
  layout->setSpacing(10);
  layout->addWidget(top);
  layout->addWidget(list, 1);
  layout->addWidget(messageBox, 1);

  connect(actionButton, &QPushButton::clicked, this, &LibraryPane::actionRequested);
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
      emit gameSelected(games.value(current.data(idRole).toString()));
    }
  });
  connect(model, &QStandardItemModel::dataChanged, this, [this] {
    saveFavorites();
    applyFilters();
  });
  showMessage(QString(), QString());
}

bool LibraryPane::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == list->viewport()) {
    if (event->type() == QEvent::MouseMove) {
      const QPoint position = static_cast<QMouseEvent*>(event)->position().toPoint();
      list->viewport()->setCursor(list->indexAt(position).isValid() ? Qt::PointingHandCursor : Qt::ArrowCursor);
    } else if (event->type() == QEvent::Leave) {
      list->viewport()->unsetCursor();
    }
  }
  return QWidget::eventFilter(watched, event);
}

void LibraryPane::applyPalette(const Palette& colors)
{
  delegate->setColors(colors);
  searchAction->setIcon(makeIcon(IconKind::search, 20, colors.text));
  filterMenuAction->setIcon(makeIcon(IconKind::chevronDown, 18, colors.text));
  list->viewport()->update();
}

void LibraryPane::showGames(const QList<Game>& newGames)
{
  const QModelIndex previous = list->currentIndex();
  const QString previousId = previous.isValid() ? previous.data(idRole).toString() : QString();
  const QStringList favorites = settings.value(QStringLiteral("library/favorites")).toStringList();
  model->clear();
  games.clear();
  for (const Game& game : newGames) {
    games.insert(game.id, game);
    auto* item = new QStandardItem;
    item->setData(game.id, idRole);
    item->setData(game.title, titleRole);
    item->setData(libraryDescription(game), editionRole);
    item->setData(game.initials, initialsRole);
    item->setData(game.tint, tintRole);
    item->setData(favorites.contains(game.id), favoriteRole);
    item->setData(game.isInstalled, installedRole);
    const QString nameKey = game.title.toLower() + QLatin1Char(' ') + libraryDescription(game).toLower();
    item->setData(nameKey, nameKeyRole);
    item->setData((game.isInstalled ? QStringLiteral("0 ") : QStringLiteral("1 ")) + nameKey, installKeyRole);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    model->appendRow(item);
  }
  message.clear();
  applyFilters();
  if (!previousId.isEmpty()) {
    selectGame(previousId);
  }
}

void LibraryPane::showMessage(const QString& text, const QString& actionText)
{
  model->clear();
  games.clear();
  message = text;
  actionButton->setText(actionText);
  applyFilters();
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
  const bool isMessage = !message.isEmpty();
  countLabel->setText(isMessage ? QString() : shown == total ? QStringLiteral("%1 games").arg(total) : QStringLiteral("%1 of %2 games").arg(shown).arg(total));
  list->setVisible(!isMessage && shown > 0);
  messageBox->setVisible(isMessage || shown == 0);
  emptyLabel->setText(isMessage ? message : total == 0 ? QStringLiteral("No games are on display yet.") : QStringLiteral("No games match."));
  actionButton->setVisible(isMessage && !actionButton->text().isEmpty());
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
  QStringList favorites = settings.value(QStringLiteral("library/favorites")).toStringList();
  for (int row = 0; row < model->rowCount(); ++row) {
    favorites.removeAll(model->index(row, 0).data(idRole).toString());
  }
  for (int row = 0; row < model->rowCount(); ++row) {
    const QModelIndex index = model->index(row, 0);
    if (index.data(favoriteRole).toBool()) {
      favorites.append(index.data(idRole).toString());
    }
  }
  settings.setValue(QStringLiteral("library/favorites"), favorites);
}
