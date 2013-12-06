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

/** 
 * \file SceneNodesListModel.h
 * \brief Contains the definition of the Ovito::SceneNodesListModel class.
 */

#ifndef __OVITO_SCENE_NODES_LIST_MODEL_H
#define __OVITO_SCENE_NODES_LIST_MODEL_H

#include <core/Core.h>
#include <core/reference/RefTargetListener.h>

namespace Ovito {

class DataSet;		// Defined in DataSet.h

/**
 * A Qt model/view system list model that contains all scene nodes in the current scene.
 */
class OVITO_CORE_EXPORT SceneNodesListModel : public QAbstractListModel
{
	Q_OBJECT
	
public:
	
	/// Constructs the model.
	SceneNodesListModel(DataSetContainer& datasetContainer, QWidget* parent = 0);

	/// Returns the number of rows of the model.
	virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;

	/// Returns the model's data stored under the given role for the item referred to by the index.
	virtual QVariant data(const QModelIndex & index, int role) const override;

private Q_SLOTS:

	/// This is called when a new dataset has been loaded.
	void onDataSetChanged(DataSet* newDataSet);

	/// This handles reference events generated by the root node.
	void onRootNodeNotificationEvent(ReferenceEvent* event);

	/// This handles reference events generated by the scene nodes.
	void onNodeNotificationEvent(RefTarget* source, ReferenceEvent* event);

private:

	/// The container of the dataset.
	DataSetContainer& _datasetContainer;

	/// This helper object is used to listen for reference events generated by scene nodes.
	VectorRefTargetListener _nodeListener;

	/// This helper object is used to listen for reference events generated by the root node.
	RefTargetListener _rootNodeListener;
};

};

#endif // __OVITO_SCENE_NODES_LIST_MODEL_H
