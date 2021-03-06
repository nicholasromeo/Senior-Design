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


#include <plugins/grid/Grid.h>
#include <plugins/grid/objects/VoxelProperty.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/mesh/surface/SurfaceMeshDisplay.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Grid {

/*
 * Constructs an isosurface from a data grid.
 */
class OVITO_GRID_EXPORT CreateIsosurfaceModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class CreateIsosurfaceModifierClass : public ModifierClass 
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;
	};
		
	Q_OBJECT
	OVITO_CLASS_META(CreateIsosurfaceModifier, CreateIsosurfaceModifierClass)

	Q_CLASSINFO("DisplayName", "Create isosurface");
	Q_CLASSINFO("ModifierCategory", "Fields");
	
public:

	/// Constructor.
	Q_INVOKABLE CreateIsosurfaceModifier(DataSet* dataset);

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(ModifierApplication* modApp) override;

	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override;

	/// Decides whether a preliminary viewport update is performed after the modifier has been 
	/// evaluated but before the entire pipeline evaluation is complete.
	/// We suppress such preliminary updates for this modifier, because it produces a surface mesh,
	/// which requires further asynchronous processing before a viewport update makes sense. 
	virtual bool performPreliminaryUpdateAfterEvaluation() override { return false; }

	/// Returns the level at which to create the isosurface.
	FloatType isolevel() const { return isolevelController() ? isolevelController()->currentFloatValue() : 0; }

	/// Sets the level at which to create the isosurface.
	void setIsolevel(FloatType value) { if(isolevelController()) isolevelController()->setCurrentFloatValue(value); }

protected:

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Holds the modifier's results.
	class ComputeIsosurfaceResults : public ComputeEngineResults
	{
	public:

		/// Constructor.
		using ComputeEngineResults::ComputeEngineResults;

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState apply(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;
		
		/// Indicates whether the entire simulation cell is part of the solid region.
		bool isCompletelySolid() const { return _isCompletelySolid; }

		/// Sets whether the entire simulation cell is part of the solid region.
		void setIsCompletelySolid(bool isSolid) { _isCompletelySolid = isSolid; }

		/// Returns the minimum field value that was encountered.
		FloatType minValue() const { return _minValue; }

		/// Returns the maximum field value that was encountered.
		FloatType maxValue() const { return _maxValue; }

		/// Returns the generated mesh.
		const std::shared_ptr<HalfEdgeMesh<>>& mesh() { return _mesh; }

		/// Adjust the min/max values to include the given value.
		void updateMinMax(FloatType val) {
			if(val < _minValue) _minValue = val;
			if(val > _maxValue) _maxValue = val;
		}
		
	private:

		/// The surface mesh produced by the modifier.
		std::shared_ptr<HalfEdgeMesh<>> _mesh = std::make_shared<HalfEdgeMesh<>>();

		/// Indicates that the entire simulation cell is part of the solid region.
		bool _isCompletelySolid = false;

		/// The minimum field value that was encountered.
		FloatType _minValue =  FLOATTYPE_MAX;

		/// The maximum field value that was encountered.
		FloatType _maxValue = -FLOATTYPE_MAX;
	};

	/// Computation engine that builds the isosurface mesh.
	class ComputeIsosurfaceEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ComputeIsosurfaceEngine(const TimeInterval& validityInterval, const std::vector<size_t>& gridShape, ConstPropertyPtr property, int vectorComponent, const SimulationCell& simCell, FloatType isolevel) :
			_gridShape(gridShape),
			_property(std::move(property)), 
			_vectorComponent(vectorComponent), 
			_simCell(simCell), 
			_isolevel(isolevel), 
			_results(std::make_shared<ComputeIsosurfaceResults>(validityInterval)) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the input voxel property.
		const ConstPropertyPtr& property() const { return _property; }

	private:

		const std::vector<size_t> _gridShape;
		const FloatType _isolevel;
		const int _vectorComponent;
		const ConstPropertyPtr _property;
		const SimulationCell _simCell;
		std::shared_ptr<ComputeIsosurfaceResults> _results;
	};

	/// The voxel property that serves input.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(VoxelPropertyReference, sourceProperty, setSourceProperty);

	/// This controller stores the level at which to create the isosurface.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(Controller, isolevelController, setIsolevelController, PROPERTY_FIELD_MEMORIZE);

	/// The display object for rendering the surface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SurfaceMeshDisplay, surfaceMeshDisplay, setSurfaceMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY | PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace
