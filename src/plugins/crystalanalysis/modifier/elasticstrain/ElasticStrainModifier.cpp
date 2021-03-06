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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include "ElasticStrainModifier.h"
#include "ElasticStrainEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(ElasticStrainModifier);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, inputCrystalStructure);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, calculateDeformationGradients);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, calculateStrainTensors);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, latticeConstant);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, axialRatio);
DEFINE_PROPERTY_FIELD(ElasticStrainModifier, pushStrainTensorsForward);
DEFINE_REFERENCE_FIELD(ElasticStrainModifier, patternCatalog);
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, inputCrystalStructure, "Input crystal structure");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, calculateDeformationGradients, "Output deformation gradient tensors");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, calculateStrainTensors, "Output strain tensors");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, latticeConstant, "Lattice constant");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, axialRatio, "c/a ratio");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, pushStrainTensorsForward, "Strain tensor in spatial frame (push-forward)");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ElasticStrainModifier, latticeConstant, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ElasticStrainModifier, axialRatio, FloatParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ElasticStrainModifier::ElasticStrainModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
	_inputCrystalStructure(StructureAnalysis::LATTICE_FCC), 
	_calculateDeformationGradients(false), 
	_calculateStrainTensors(true),
	_latticeConstant(1), 
	_axialRatio(sqrt(8.0/3.0)), 
	_pushStrainTensorsForward(true)
{
	// Create pattern catalog.
	setPatternCatalog(new PatternCatalog(dataset));

	// Create the structure types.
	ParticleType::PredefinedStructureType predefTypes[] = {
			ParticleType::PredefinedStructureType::OTHER,
			ParticleType::PredefinedStructureType::FCC,
			ParticleType::PredefinedStructureType::HCP,
			ParticleType::PredefinedStructureType::BCC,
			ParticleType::PredefinedStructureType::CUBIC_DIAMOND,
			ParticleType::PredefinedStructureType::HEX_DIAMOND
	};
	OVITO_STATIC_ASSERT(sizeof(predefTypes)/sizeof(predefTypes[0]) == StructureAnalysis::NUM_LATTICE_TYPES);
	for(int id = 0; id < StructureAnalysis::NUM_LATTICE_TYPES; id++) {
		OORef<StructurePattern> stype = _patternCatalog->structureById(id);
		if(!stype) {
			stype = new StructurePattern(dataset);
			stype->setId(id);
			stype->setStructureType(StructurePattern::Lattice);
			_patternCatalog->addPattern(stype);
		}
		stype->setName(ParticleType::getPredefinedStructureTypeName(predefTypes[id]));
		stype->setColor(ParticleType::getDefaultParticleColor(ParticleProperty::StructureTypeProperty, stype->name(), id));
		addStructureType(stype);
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ElasticStrainModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier inputs.
	ParticleInputHelper pih(dataset(), input);
	ParticleProperty* posProperty = pih.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = pih.expectSimulationCell();
	if(simCell->is2D())
		throwException(tr("The elastic strain calculation modifier does not support 2d simulation cells."));

	// Build list of preferred crystal orientations.
	std::vector<Matrix3> preferredCrystalOrientations;
	if(inputCrystalStructure() == StructureAnalysis::LATTICE_FCC || inputCrystalStructure() == StructureAnalysis::LATTICE_BCC || inputCrystalStructure() == StructureAnalysis::LATTICE_CUBIC_DIAMOND) {
		preferredCrystalOrientations.push_back(Matrix3::Identity());
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ElasticStrainEngine>(posProperty->storage(),
			simCell->data(), inputCrystalStructure(), std::move(preferredCrystalOrientations),
			calculateDeformationGradients(), calculateStrainTensors(),
			latticeConstant(), axialRatio(), pushStrainTensorsForward());
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState ElasticStrainResults::apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	ElasticStrainModifier* modifier = static_object_cast<ElasticStrainModifier>(modApp->modifier());
	
	PipelineFlowState output = StructureIdentificationResults::apply(time, modApp, input);
	ParticleOutputHelper poh(modApp->dataset(), output);

	// Output cluster graph.
	OORef<ClusterGraphObject> clusterGraphObj(new ClusterGraphObject(modApp->dataset()));
	clusterGraphObj->setStorage(clusterGraph());
	output.addObject(clusterGraphObj);

	// Output pattern catalog.
	output.addObject(modifier->patternCatalog());

	// Output particle properties.
	poh.outputProperty<ParticleProperty>(atomClusters());
	if(modifier->calculateStrainTensors() && strainTensors())
		poh.outputProperty<ParticleProperty>(strainTensors());

	if(modifier->calculateDeformationGradients() && deformationGradients())
	poh.outputProperty<ParticleProperty>(deformationGradients());

	if(volumetricStrains())
		poh.outputProperty<ParticleProperty>(volumetricStrains());

	return output;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

