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

#include <core/Core.h>
#include "ObjectStatusWidget.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
ObjectStatusWidget::ObjectStatusWidget(QWidget* parent) : QScrollArea(parent),
	_statusWarningIcon(":/core/mainwin/status/status_warning.png"),
	_statusErrorIcon(":/core/mainwin/status/status_error.png")
{
	QWidget* container = new QWidget();
	QHBoxLayout* layout = new QHBoxLayout(container);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(2);

	_iconLabel = new QLabel(container);
	_iconLabel->setAlignment(Qt::AlignTop);
	layout->addWidget(_iconLabel, 0, Qt::AlignTop);

	_textLabel = new QLabel(container);
	_textLabel->setAlignment(Qt::AlignTop);
	_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
	_textLabel->setWordWrap(true);
	layout->addWidget(_textLabel, 1, Qt::AlignTop);

	setWidget(container);
	setWidgetResizable(true);
}

/******************************************************************************
* Sets the status displayed by the widget.
******************************************************************************/
void ObjectStatusWidget::setStatus(const ObjectStatus& status)
{
	_status = status;

	_textLabel->setText(status.text());

	if(status.type() == ObjectStatus::Warning)
		_iconLabel->setPixmap(_statusWarningIcon);
	else if(status.type() == ObjectStatus::Error)
		_iconLabel->setPixmap(_statusErrorIcon);
	else
		_iconLabel->clear();
}

};