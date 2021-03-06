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

#pragma once


#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/netcdf/AMBERNetCDFImporter.h>
#include <gui/dataset/io/FileImporterEditor.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A properties editor for the AMBERNetCDFImporter class.
 */
class AMBERNetCDFImporterEditor : public FileImporterEditor
{
	Q_OBJECT
	OVITO_CLASS(AMBERNetCDFImporterEditor)

public:

	/// Constructor.
	Q_INVOKABLE AMBERNetCDFImporterEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// Displays a dialog box that allows the user to edit the custom file column to particle property mapping.
	bool showEditColumnMappingDialog(AMBERNetCDFImporter* importer, const QUrl& sourceFile, QWidget* parent);

protected Q_SLOTS:

	/// Is called when the user pressed the "Edit column mapping" button.
	void onEditColumnMapping();
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


