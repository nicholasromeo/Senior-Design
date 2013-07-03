///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __OVITO_MODIFICATION_LIST_MODEL_H
#define __OVITO_MODIFICATION_LIST_MODEL_H

#include <core/Core.h>
#include <core/reference/RefTarget.h>
#include <core/reference/RefTargetListener.h>
#include <core/scene/pipeline/ModifierApplication.h>
#include <core/scene/SceneNode.h>

#include "ModificationListItem.h"

namespace Ovito {

class SceneObject;			// defined in SceneObject.h
class Modifier;				// defined in Modifier.h

/*
 * This Qt model class is used to populate the QListView widget.
 */
class ModificationListModel : public QAbstractListModel
{
	Q_OBJECT

public:

	/// Constructor.
	ModificationListModel(QObject* parent);

	/// Returns the number of list items.
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override { return _items.size(); }

	/// Returns the data associated with a list entry.
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	/// Changes the data associated with a list entry.
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	/// Returns the flags for an item.
	virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

	/// Discards all list items.
	void clear() {
		if(_items.empty()) return;
		beginRemoveRows(QModelIndex(), 0, _items.size() - 1);
		_items.clear();
		_selectedNodes.clear();
		endRemoveRows();
		_needListUpdate = false;
	}

	/// Returns the associated selection model.
	QItemSelectionModel* selectionModel() const { return _selectionModel; }

	/// Returns the currently selected item in the modification list.
	ModificationListItem* selectedItem() const;

	/// Returns an item from the list model.
	ModificationListItem* item(int index) const {
		OVITO_ASSERT(index >= 0 && index < _items.size());
		return _items[index].get();
	}

	/// Populates the model with the given list items.
	void setItems(const QVector<OORef<ModificationListItem>>& newItems);

	/// Returns true if the list model is currently in a valid state.
	bool isUpToDate() const { return !_needListUpdate; }

Q_SIGNALS:

	/// This signal is emitted if a new list item has been selected, or if the currently
	/// selected item has changed.
	void selectedItemChanged();

public Q_SLOTS:

    /// Inserts the given modifier into the modification pipeline of the selected scene nodes.
	void applyModifier(Modifier* modifier);

	/// Rebuilds the complete list immediately.
	void refreshList();

	/// Updates the appearance of a single list item.
	void refreshItem(ModificationListItem* item);

	/// Rebuilds the list of modification items as soon as possible.
	void requestUpdate() {
		if(_needListUpdate) return;	// Update is already pending.
		_needListUpdate = true;
		// Invoke actual refresh function at some later time.
		QMetaObject::invokeMethod(this, "refreshList", Qt::QueuedConnection);
	}

private Q_SLOTS:

	/// Is called by the system when the animated status icon changed.
	void iconAnimationFrameChanged();

	/// Handles notification events generated by the selected object nodes.
	void onNodeEvent(RefTarget* source, ReferenceEvent* event);

private:

	/// List of items in the model.
	QVector<OORef<ModificationListItem>> _items;

	/// Holds references to the currently selected ObjectNode instances.
	VectorRefTargetListener _selectedNodes;

	/// The item in the modification list that should be selected on the next list update.
	RefTarget* _nextToSelectObject;

	/// The selection model of the list view widget.
	QItemSelectionModel* _selectionModel;

	/// Indicates that the list of items needs to be updated.
	bool _needListUpdate;

	/// Status icons
	QIcon _modifierEnabledIcon;
	QIcon _modifierDisabledIcon;
	QIcon _statusInfoIcon;
	QIcon _statusWarningIcon;
	QIcon _statusErrorIcon;
	QMovie _statusPendingIcon;

	/// The font used for section headers.
	QFont _sectionHeaderFont;
};

};

#endif // __OVITO_MODIFICATION_LIST_MODEL_H
