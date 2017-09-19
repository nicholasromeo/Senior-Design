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


#include <gui/GUI.h>
#include "ParameterUI.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

/******************************************************************************
* This UI allows the user to edit a string property of the object being edited.
******************************************************************************/
class OVITO_GUI_EXPORT StringParameterUI : public PropertyParameterUI
{
	Q_OBJECT
	OVITO_CLASS(StringParameterUI)
	
public:
	/// Constructor for a Qt property.
	StringParameterUI(QObject* parentEditor, const char* propertyName);
	
	/// Constructor for a PropertyField property.
	StringParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField);
	
	/// Destructor.
	virtual ~StringParameterUI();

	/// This returns the text widget managed by this ParameterUI.
	QWidget* textBox() const { return _textBox; }
	
	/// Replaces the text widget managed by this ParameterUI. The ParameterUI becomes the owner of the new widget.
	void setTextBox(QWidget* textBox);

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.  
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;
	
	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;
	
	/// Sets the tooltip text for the text box.
	void setToolTip(const QString& text) const { 
		if(textBox()) textBox()->setToolTip(text); 
	}
	
	/// Sets the What's This helper text for the textbox.
	void setWhatsThis(const QString& text) const { 
		if(textBox()) textBox()->setWhatsThis(text); 
	}
	
public:
	
	Q_PROPERTY(QWidget textBox READ textBox);
	
public Q_SLOTS:
	
	/// Takes the value entered by the user and stores it in the property field 
	/// this property UI is bound to. 
	void updatePropertyValue();
	
protected:

	/// The text box of the UI component.
	QPointer<QWidget> _textBox;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


