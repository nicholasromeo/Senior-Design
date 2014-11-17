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
 * \file CommonNeighborAnalysisModifier.h
 * \brief Contains the definition of the Particles::CommonNeighborAnalysisModifier class.
 */

#ifndef __OVITO_COMMON_NEIGHBOR_ANALYSIS_MODIFIER_H
#define __OVITO_COMMON_NEIGHBOR_ANALYSIS_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <core/gui/properties/RefTargetListParameterUI.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>

namespace Particles {

using namespace Ovito;

class OnTheFlyNeighborListBuilder;
class TreeNeighborListBuilder;

/**
 * \brief A modifier that performs the common neighbor analysis (CNA) to identify
 *        local coordination structures.
 */
class OVITO_PARTICLES_EXPORT CommonNeighborAnalysisModifier : public StructureIdentificationModifier
{
public:

	// The maximum number of neighbor atoms taken into account for the common neighbor analysis.
	static constexpr int MAX_NEIGHBORS = 16;

	/// The structure types recognized by the common neighbor analysis.
	enum StructureType {
		OTHER = 0,				//< Unidentified structure
		FCC,					//< Face-centered cubic
		HCP,					//< Hexagonal close-packed
		BCC,					//< Body-centered cubic
		ICO,					//< Icosahedral structure
		DIA,					//< Cubic diamond structure

		NUM_STRUCTURE_TYPES 	//< This just counts the number of defined structure types.
	};
	Q_ENUMS(StructureType);

	/// Analysis engine that performs the conventional common neighbor analysis.
	class FixedCommonNeighborAnalysisEngine : public StructureIdentificationModifier::StructureIdentificationEngine
	{
	public:

		/// Constructor.
		FixedCommonNeighborAnalysisEngine(ParticleProperty* positions, const SimulationCellData& simCell, FloatType cutoff) :
			StructureIdentificationModifier::StructureIdentificationEngine(positions, simCell), _cutoff(cutoff) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void compute(FutureInterfaceBase& futureInterface) override;

	private:

		/// The CNA cutoff radius.
		FloatType _cutoff;
	};

	/// Analysis engine that performs the adaptive common neighbor analysis.
	class AdaptiveCommonNeighborAnalysisEngine : public StructureIdentificationModifier::StructureIdentificationEngine
	{
	public:

		/// Constructor.
		AdaptiveCommonNeighborAnalysisEngine(ParticleProperty* positions, const SimulationCellData& simCell) :
			StructureIdentificationModifier::StructureIdentificationEngine(positions, simCell) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void compute(FutureInterfaceBase& futureInterface) override;
	};

public:

	/// Constructor.
	Q_INVOKABLE CommonNeighborAnalysisModifier(DataSet* dataset);

	/// \brief Returns the cutoff radius used in the conventional common neighbor analysis.
	/// \return The cutoff radius in world units.
	/// \sa setCutoff()
	FloatType cutoff() const { return _cutoff; }

	/// \brief Sets the cutoff radius used in the conventional common neighbor analysis.
	/// \param newCutoff The new cutoff radius in world units.
	/// \undoable
	/// \sa cutoff()
	void setCutoff(FloatType newCutoff) { _cutoff = newCutoff; }

	/// \brief Returns true if the cutoff radius is determined adaptively for each particle.
	bool adaptiveMode() const { return _adaptiveMode; }

	/// \brief Controls whether the cutoff radius should be determined adaptively for each particle.
	void setAdaptiveMode(bool adaptive) { _adaptiveMode = adaptive; }

public:

	/// Pair of neighbor atoms that form a bond (bit-wise storage).
	typedef unsigned int CNAPairBond;

	/**
	 * A bit-flag array indicating which pairs of neighbors are bonded
	 * and which are not.
	 */
	struct NeighborBondArray
	{
		/// Two-dimensional bit array that stores the bonds between neighbors.
		unsigned int neighborArray[MAX_NEIGHBORS];

		/// Default constructor.
		NeighborBondArray() {
			memset(neighborArray, 0, sizeof(neighborArray));
		}

		/// Returns whether two nearest neighbors have a bond between them.
		inline bool neighborBond(int neighborIndex1, int neighborIndex2) const {
			OVITO_ASSERT(neighborIndex1 < MAX_NEIGHBORS);
			OVITO_ASSERT(neighborIndex2 < MAX_NEIGHBORS);
			return (neighborArray[neighborIndex1] & (1<<neighborIndex2));
		}

		/// Sets whether two nearest neighbors have a bond between them.
		inline void setNeighborBond(int neighborIndex1, int neighborIndex2, bool bonded) {
			OVITO_ASSERT(neighborIndex1 < MAX_NEIGHBORS);
			OVITO_ASSERT(neighborIndex2 < MAX_NEIGHBORS);
			if(bonded) {
				neighborArray[neighborIndex1] |= (1<<neighborIndex2);
				neighborArray[neighborIndex2] |= (1<<neighborIndex1);
			}
			else {
				neighborArray[neighborIndex1] &= ~(1<<neighborIndex2);
				neighborArray[neighborIndex2] &= ~(1<<neighborIndex1);
			}
		}
	};

	/// Find all atoms that are nearest neighbors of the given pair of atoms.
	static int findCommonNeighbors(const NeighborBondArray& neighborArray, int neighborIndex, unsigned int& commonNeighbors, int numNeighbors);

	/// Finds all bonds between common nearest neighbors.
	static int findNeighborBonds(const NeighborBondArray& neighborArray, unsigned int commonNeighbors, int numNeighbors, CNAPairBond* neighborBonds);

	/// Find all chains of bonds between common neighbors and determine the length
	/// of the longest continuous chain.
	static int calcMaxChainLength(CNAPairBond* neighborBonds, int numBonds);

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates and initializes a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<Engine> createEngine(TimePoint time, TimeInterval& validityInterval) override;

	/// Determines the coordination structure of a single particle using the common neighbor analysis method.
	static StructureType determineStructureAdaptive(TreeNeighborListBuilder& neighList, size_t particleIndex);

	/// Determines the coordination structure of a single particle using the common neighbor analysis method.
	static StructureType determineStructureFixed(OnTheFlyNeighborListBuilder& neighList, size_t particleIndex);

	/// The cutoff radius for the CNA.
	PropertyField<FloatType> _cutoff;

	/// Controls whether the cutoff radius is determined adaptively for each particle.
	PropertyField<bool> _adaptiveMode;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Common neighbor analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_cutoff);
	DECLARE_PROPERTY_FIELD(_adaptiveMode);
};

/**
 * \brief A properties editor for the CommonNeighborAnalysisModifier class.
 */
class CommonNeighborAnalysisModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE CommonNeighborAnalysisModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

};	// End of namespace

#endif // __OVITO_COMMON_NEIGHBOR_ANALYSIS_MODIFIER_H
