/*
 * Cantata
 *
 * Copyright (c) 2018 Craig Drummond <craig.p.drummond@gmail.com>
 *
 * ----
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "categorizedview.h"
#include "kcategorizedview/kcategorydrawer.h"
#include "kcategorizedview/kcategorizedsortfilterproxymodel.h"
#include "config.h"
#include "icons.h"
#include "support/utils.h"
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QModelIndex>
#include <QApplication>

CategorizedView::CategorizedView(QWidget *parent)
    : KCategorizedView(parent)
    , eventFilter(nullptr)
    , menu(nullptr)
    , zoomLevel(1.0)
{
    proxy=new KCategorizedSortFilterProxyModel(this);
    proxy->setCategorizedModel(true);
    setCategoryDrawer(new KCategoryDrawer(this));
    setDragEnabled(true);
    setContextMenuPolicy(Qt::NoContextMenu);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAlternatingRowColors(false);
    setUniformItemSizes(true);
    setAttribute(Qt::WA_MouseTracking);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(showCustomContextMenu(const QPoint &)));
    connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(checkDoubleClick(const QModelIndex &)));

    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setWordWrap(true);
    setDragDropMode(QAbstractItemView::DragOnly);
}

CategorizedView::~CategorizedView()
{
}

void CategorizedView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    KCategorizedView::selectionChanged(selected, deselected);
    bool haveSelection=haveSelectedItems();

    setContextMenuPolicy(haveSelection ? Qt::ActionsContextMenu : (menu ? Qt::CustomContextMenu : Qt::NoContextMenu));
    emit itemsSelected(haveSelection);
}

bool CategorizedView::haveSelectedItems() const
{
    // Dont need the sorted type of 'selectedIndexes' here...
    return selectionModel() && selectionModel()->selectedIndexes().count()>0;
}

bool CategorizedView::haveUnSelectedItems() const
{
    // Dont need the sorted type of 'selectedIndexes' here...
    return selectionModel() && model() && selectionModel()->selectedIndexes().count()!=model()->rowCount();
}

void CategorizedView::mouseReleaseEvent(QMouseEvent *event)
{
    if (Qt::NoModifier==event->modifiers() && Qt::LeftButton==event->button()) {
        KCategorizedView::mouseReleaseEvent(event);
    }
}

QModelIndexList CategorizedView::selectedIndexes(bool sorted) const
{
    QModelIndexList indexes=selectionModel() ? selectionModel()->selectedIndexes() : QModelIndexList();
    if (sorted) {
        qSort(indexes);
    }
    return indexes;
}

void CategorizedView::setModel(QAbstractItemModel *m)
{
    QAbstractItemModel *old=proxy->sourceModel();
    proxy->setSourceModel(m);

    if (old) {
        disconnect(old, SIGNAL(layoutChanged()), this, SLOT(correctSelection()));
    }

    if (m && old!=m) {
        connect(m, SIGNAL(layoutChanged()), this, SLOT(correctSelection()));
    }
    if (m) {
        KCategorizedView::setModel(proxy);
    } else {
        KCategorizedView::setModel(nullptr);
    }
}

void CategorizedView::addDefaultAction(QAction *act)
{
    if (!menu) {
        menu=new QMenu(this);
    }
    menu->addAction(act);
}

void CategorizedView::setBackgroundImage(const QIcon &icon)
{
    QPalette pal=parentWidget()->palette();
//    if (!icon.isNull()) {
//        pal.setColor(QPalette::Base, Qt::transparent);
//    }
    #ifndef Q_OS_MAC
    setPalette(pal);
    #endif
    viewport()->setPalette(pal);
    bgnd=TreeView::createBgndPixmap(icon);
}

void CategorizedView::paintEvent(QPaintEvent *e)
{
    if (!bgnd.isNull()) {
        QPainter p(viewport());
        QSize sz=size();
        p.fillRect(0, 0, sz.width(), sz.height(), QApplication::palette().color(QPalette::Base));
        p.drawPixmap((sz.width()-bgnd.width())/2, (sz.height()-bgnd.height())/2, bgnd);
    }
    if (!info.isEmpty() && model() && 0==model()->rowCount()) {
        QPainter p(viewport());
        QColor col(palette().text().color());
        col.setAlphaF(0.5);
        QFont f(font());
        f.setItalic(true);
        p.setPen(col);
        p.setFont(f);
        p.drawText(rect().adjusted(8, 8, -16, -16), Qt::AlignCenter|Qt::TextWordWrap, info);
    }
    KCategorizedView::paintEvent(e);
}

// Workaround for https://bugreports.qt-project.org/browse/QTBUG-18009
void CategorizedView::correctSelection()
{
    if (!selectionModel()) {
        return;
    }

    QItemSelection s = selectionModel()->selection();
    setCurrentIndex(currentIndex());
    selectionModel()->select(s, QItemSelectionModel::SelectCurrent);
}

void CategorizedView::showCustomContextMenu(const QPoint &pos)
{
    if (menu) {
        menu->popup(mapToGlobal(pos));
    }
}

void CategorizedView::checkDoubleClick(const QModelIndex &idx)
{
    if (!TreeView::getForceSingleClick() && idx.model() && idx.model()->rowCount(idx)) {
        return;
    }
    emit itemDoubleClicked(idx);
}
