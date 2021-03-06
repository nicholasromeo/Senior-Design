///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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

#include <plugins/stdmod/gui/StdModGui.h>
#include <gui/properties/ModifierDelegateListParameterUI.h>
#include <plugins/stdmod/modifiers/DeleteSelectedModifier.h>
#include "DeleteSelectedModifierEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(DeleteSelectedModifierEditor);
SET_OVITO_OBJECT_EDITOR(DeleteSelectedModifier, DeleteSelectedModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DeleteSelectedModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Delete selected"), rolloutParams, "particles.modifiers.delete_selected_particles.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(8);

	ModifierDelegateListParameterUI* delegatesPUI = new ModifierDelegateListParameterUI(this, rolloutParams.after(rollout));
	layout->addWidget(delegatesPUI->listWidget());
	
	// Status label.
	layout->addWidget(statusLabel());
}

}	// End of namespace
}	// End of namespace
