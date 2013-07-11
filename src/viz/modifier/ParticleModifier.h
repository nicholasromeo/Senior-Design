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

#ifndef __OVITO_PARTICLE_MODIFIER_H
#define __OVITO_PARTICLE_MODIFIER_H

#include <core/Core.h>
#include <core/scene/pipeline/Modifier.h>
#include <core/scene/objects/SceneObject.h>
#include <core/gui/properties/PropertiesEditor.h>
#include <core/reference/CloneHelper.h>
#include <core/reference/RefTargetListener.h>

#include <viz/data/ParticlePropertyObject.h>
#include <viz/data/SimulationCell.h>

namespace Viz {

using namespace Ovito;

/*
 * Abstract base class for modifiers that operate on a system of particles.
 */
class ParticleModifier : public Modifier
{
protected:

	/// Constructor.
	ParticleModifier() {}

public:

	/// This modifies the input object.
	virtual ObjectStatus modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// \brief Returns a structure that describes the current status of the modifier.
	virtual ObjectStatus status() const override { return _modifierStatus; }

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Modifies the particle object. This function must be implemented by sub-classes
	/// do the modifier specific work. The time interval passed
	/// to the function should be reduced to the interval where the returned object is valid/constant.
	virtual ObjectStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) = 0;

	/// Returns a standard particle property from the input state.
	/// The returned property may be NULL if it does not exist.
	ParticlePropertyObject* inputStandardProperty(ParticleProperty::Type which) const;

	/// Returns the given standard property from the input object.
	/// The returned property may not be modified. If the input object does
	/// not contain the standard property then an exception is thrown.
	ParticlePropertyObject* expectStandardProperty(ParticleProperty::Type which) const;

	/// Returns the property with the given name from the input particles.
	/// The returned property may not be modified. If the input object does
	/// not contain a property with the given name and data type, then an exception is thrown.
	ParticlePropertyObject* expectCustomProperty(const QString& propertyName, int dataType, size_t componentCount = 1) const;

	/// Returns the input simulation cell.
	/// The returned object may not be modified. If the input does
	/// not contain a simulation cell, an exception is thrown.
	SimulationCell* expectSimulationCell() const;

	/// Creates a standard particle in the modifier's output.
	/// If the particle property already exists in the input, its contents are copied to the
	/// output property by this method.
	ParticlePropertyObject* outputStandardProperty(ParticleProperty::Type which);

	/// Returns the number of particles in the input.
	size_t inputParticleCount() const { return _inputParticleCount; }

	/// Returns the number of particles in the output.
	size_t outputParticleCount() const { return _outputParticleCount; }

	/// Returns a vector with the input particles colors.
	std::vector<Color> inputParticleColors(TimePoint time, TimeInterval& validityInterval);

	/// Deletes the particles given by the bit-mask.
	/// Returns the number of remaining particles.
	size_t deleteParticles(const std::vector<bool>& mask, size_t deleteCount);

	/// Returns a reference to the input state.
	PipelineFlowState& input() { return _input; }

	/// Returns a reference to the output state.
	PipelineFlowState& output() { return _output; }

	/// Returns a clone helper object that should be used to create shallow and deep copies.
	CloneHelper* cloneHelper() {
		if(!_cloneHelper) _cloneHelper.reset(new CloneHelper());
		return _cloneHelper.data();
	}

	/// Sets the status returned by the modifier and generates a ReferenceEvent::StatusChanged event.
	void setStatus(const ObjectStatus& status);

protected:

	/// The clone helper object that is used to create shallow and deep copies
	/// of the atoms object and its channels.
	QScopedPointer<CloneHelper> _cloneHelper;

	/// The current ModifierApplication object.
	ModifierApplication* _modApp;

	/// The input state.
	PipelineFlowState _input;

	/// The output state.
	PipelineFlowState _output;

	/// The number of particles in the input.
	size_t _inputParticleCount;

	/// The number of particles in the output.
	size_t _outputParticleCount;

	/// The status returned by the modifier.
	ObjectStatus _modifierStatus;

private:

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * Base class for properties editors for ParticleModifier derived classes.
 */
class ParticleModifierEditor : public PropertiesEditor
{
public:

	/// Constructor.
	ParticleModifierEditor() :
		_modifierStatusInfoIcon(":/core/mainwin/status/status_info.png"),
		_modifierStatusWarningIcon(":/core/mainwin/status/status_warning.png"),
		_modifierStatusErrorIcon(":/core/mainwin/status/status_error.png")
	{
		connect(this, SIGNAL(contentsReplaced(RefTarget*)), this, SLOT(updateStatusLabel()));
	}

	/// Returns a widget that displays a message sent by the modifier that
	/// states the outcome of the modifier evaluation. Derived classes of this
	/// editor base class can add the widget to their user interface.
	QWidget* statusLabel();

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

private Q_SLOTS:

	/// Updates the text of the result label.
	void updateStatusLabel();

private:

	QPointer<QWidget> _statusLabel;
	QPointer<QLabel> _statusTextLabel;
	QPointer<QLabel> _statusIconLabel;

	QPixmap _modifierStatusInfoIcon;
	QPixmap _modifierStatusWarningIcon;
	QPixmap _modifierStatusErrorIcon;

	Q_OBJECT
	OVITO_OBJECT
};

};	// End of namespace

#endif // __OVITO_PARTICLE_MODIFIER_H