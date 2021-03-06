///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <plugins/vorotop/VoroTopPlugin.h>
#include <plugins/stdobj/properties/PropertyStorage.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include "Filter.h"

namespace voro {
	class voronoicell_neighbor;	// Defined by Voro++
}

namespace Ovito { namespace VoroTop {

/**
 * \brief This analysis modifier performs the Voronoi topology analysis developed by Emanuel A. Lazar.
 */
class OVITO_VOROTOP_EXPORT VoroTopModifier : public StructureIdentificationModifier
{
	Q_OBJECT
	OVITO_CLASS(VoroTopModifier)

	Q_CLASSINFO("DisplayName", "VoroTop analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE VoroTopModifier(DataSet* dataset);

	/// Loads a new filter definition into the modifier.
	void loadFilterDefinition(const QString& filepath);

	/// Returns the VoroTop filter definition cached from the last analysis run.
	const std::shared_ptr<Filter>& filter() const { return _filter; }

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Holds the modifier's results.
	class VoroTopAnalysisResults : public StructureIdentificationResults
	{
	public:

		/// Constructor.
		VoroTopAnalysisResults(size_t particleCount, const std::shared_ptr<Filter>& filter) : 
			StructureIdentificationResults(particleCount),
			_filter(filter) {}

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
	
		/// Returns the VoroTop filter definition.
		const std::shared_ptr<Filter>& filter() const { return _filter; }
	
	private:

		/// The VoroTop filter definition.
		std::shared_ptr<Filter> _filter;
	};

	/// Compute engine that performs the actual analysis in a background thread.
	class VoroTopAnalysisEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		VoroTopAnalysisEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, ConstPropertyPtr selection,
							std::vector<FloatType> radii, const SimulationCell& simCell, const QString& filterFile, std::shared_ptr<Filter> filter, QVector<bool> typesToIdentify) :
			StructureIdentificationEngine(std::move(positions), simCell, std::move(typesToIdentify), std::move(selection)),
			_filterFile(filterFile),
			_filter(std::move(filter)),
			_radii(std::move(radii)) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Processes a single Voronoi cell.
		void processCell(voro::voronoicell_neighbor& vcell, size_t particleIndex, PropertyStorage& structures, QMutex* mutex);

		/// Returns the VoroTop filter definition.
		const std::shared_ptr<Filter>& filter() const { return _filter; }

	private:

		/// The path of the external file containing the filter definition.
		QString _filterFile;

		/// The VoroTop filter definition.
		std::shared_ptr<Filter> _filter;		

		/// The per-particle radii.
		std::vector<FloatType> _radii;
	};

private:

	/// Controls whether the weighted Voronoi tessellation is computed, which takes into account particle radii.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useRadii, setUseRadii);

	/// The external file path of the loaded filter file.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, filterFile, setFilterFile);	

	/// The VoroTop filter definition cached from the last analysis run.
	std::shared_ptr<Filter> _filter;
};

}	// End of namespace
}	// End of namespace
