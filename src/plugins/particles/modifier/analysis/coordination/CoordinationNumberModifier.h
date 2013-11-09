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

#ifndef __OVITO_COORDINATION_NUMBER_MODIFIER_H
#define __OVITO_COORDINATION_NUMBER_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/util/OnTheFlyNeighborListBuilder.h>
#include "../../AsynchronousParticleModifier.h"

class QCustomPlot;

namespace Particles {

/*
 * This modifier determines the coordination number of each particle.
 */
class OVITO_PARTICLES_EXPORT CoordinationNumberModifier : public AsynchronousParticleModifier
{
public:

	/// Default constructor.
	Q_INVOKABLE CoordinationNumberModifier();

	/// Returns the cutoff radius used to build the neighbor lists for the analysis.
	FloatType cutoff() const { return _cutoff; }

	/// \brief Sets the cutoff radius used to build the neighbor lists for the analysis.
	void setCutoff(FloatType newCutoff) { _cutoff = newCutoff; }

	/// Returns the computed coordination numbers.
	const ParticleProperty& coordinationNumbers() const { OVITO_CHECK_POINTER(_coordinationNumbers.constData()); return *_coordinationNumbers; }

	/// Returns the X coordinates of the RDF data points.
	const QVector<double>& rdfX() const { return _rdfX; }

	/// Returns the Y coordinates of the RDF data points.
	const QVector<double>& rdfY() const { return _rdfY; }

public:

	Q_PROPERTY(FloatType cutoff READ cutoff WRITE setCutoff)

private:

	/// Computes the modifier's results.
	class CoordinationAnalysisEngine : public AsynchronousParticleModifier::Engine
	{
	public:

		/// Constructor.
		CoordinationAnalysisEngine(ParticleProperty* positions, const SimulationCellData& simCell, FloatType cutoff, int rdfSampleCount) :
			_positions(positions), _simCell(simCell),
			_cutoff(cutoff), _rdfHistogram(rdfSampleCount, 0.0),
			_coordinationNumbers(new ParticleProperty(positions->size(), ParticleProperty::CoordinationProperty)) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void compute(FutureInterfaceBase& futureInterface) override;

		/// Returns the property storage that contains the input particle positions.
		ParticleProperty* positions() const { return _positions.data(); }

		/// Returns the simulation cell data.
		const SimulationCellData& cell() const { return _simCell; }

		/// Returns the property storage that contains the computed coordination numbers.
		ParticleProperty* coordinationNumbers() const { return _coordinationNumbers.data(); }

		/// Returns the cutoff radius.
		FloatType cutoff() const { return _cutoff; }

		/// Returns the histogram for the radial distribution function.
		const QVector<double>& rdfHistogram() const { return _rdfHistogram; }

	private:

		FloatType _cutoff;
		SimulationCellData _simCell;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _coordinationNumbers;
		QVector<double> _rdfHistogram;
	};

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates and initializes a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<Engine> createEngine(TimePoint time) override;

	/// Unpacks the computation results stored in the given engine object.
	virtual void retrieveModifierResults(Engine* engine) override;

	/// Inserts the computed and cached modifier results into the modification pipeline.
	virtual ObjectStatus applyModifierResults(TimePoint time, TimeInterval& validityInterval) override;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _coordinationNumbers;

	/// Controls the cutoff radius for the neighbor lists.
	PropertyField<FloatType> _cutoff;

	/// The X coordinates of the RDF data points.
	QVector<double> _rdfX;

	/// The Y coordinates of the RDF data points.
	QVector<double> _rdfY;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Coordination analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_cutoff)
};

/******************************************************************************
* A properties editor for the CoordinationNumberModifier class.
******************************************************************************/
class CoordinationNumberModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE CoordinationNumberModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

protected Q_SLOTS:

	/// Replots the RDF computed by the modifier.
	void plotRDF();

private:

	/// The graph widget to display the RDF.
	QCustomPlot* _rdfPlot;

	Q_OBJECT
	OVITO_OBJECT
};

};	// End of namespace

#endif // __OVITO_COORDINATION_NUMBER_MODIFIER_H