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
#include <plugins/crystalanalysis/util/DelaunayTessellation.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include "DislocationAnalysisEngine.h"
#include "DislocationAnalysisModifier.h"

#if 0
#include <fstream>
#endif

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationAnalysisEngine::DislocationAnalysisEngine(
		ConstPropertyPtr positions, const SimulationCell& simCell,
		int inputCrystalStructure, int maxTrialCircuitSize, int maxCircuitElongation,
		ConstPropertyPtr particleSelection,
		ConstPropertyPtr crystalClusters,
		std::vector<Matrix3> preferredCrystalOrientations,
		bool onlyPerfectDislocations, int defectMeshSmoothingLevel,
		int lineSmoothingLevel, FloatType linePointInterval,
		bool outputInterfaceMesh) :
	StructureIdentificationModifier::StructureIdentificationEngine(positions, simCell, {}, std::move(particleSelection)),
	_results(std::make_shared<DislocationAnalysisResults>(positions->size(), simCell.volume3D())),
	_structureAnalysis(positions, simCell, (StructureAnalysis::LatticeStructureType)inputCrystalStructure, selection(), _results->structures(), std::move(preferredCrystalOrientations), !onlyPerfectDislocations),
	_elasticMapping(_structureAnalysis, _tessellation),
	_interfaceMesh(_elasticMapping),
	_dislocationTracer(_interfaceMesh, _structureAnalysis.clusterGraph(), maxTrialCircuitSize, maxCircuitElongation),
	_inputCrystalStructure(inputCrystalStructure),
	_crystalClusters(crystalClusters),
	_onlyPerfectDislocations(onlyPerfectDislocations),
	_defectMeshSmoothingLevel(defectMeshSmoothingLevel),
	_lineSmoothingLevel(lineSmoothingLevel),
	_linePointInterval(linePointInterval),
	_outputInterfaceMesh(outputInterfaceMesh)
{
	_results->setAtomClusters(_structureAnalysis.atomClusters());
	_results->setDislocationNetwork(_dislocationTracer.network());
	_results->setClusterGraph(_dislocationTracer.clusterGraph());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void DislocationAnalysisEngine::perform()
{
	setProgressText(DislocationAnalysisModifier::tr("Dislocation analysis (DXA)"));

	beginProgressSubStepsWithWeights({ 35, 6, 1, 220, 60, 1, 53, 190, 146, 20, 4, 4 });
	if(!_structureAnalysis.identifyStructures(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.buildClusters(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.connectClusters(*this))
		return;

#if 0
	Point3 corners[8];
	corners[0] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,0,0));
	corners[1] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,0,0));
	corners[2] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,1,0));
	corners[3] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,1,0));
	corners[4] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,0,1));
	corners[5] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,0,1));
	corners[6] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,1,1));
	corners[7] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,1,1));

	std::ofstream stream("cell.vtk");
	stream << "# vtk DataFile Version 3.0" << std::endl;
	stream << "# Simulation cell" << std::endl;
	stream << "ASCII" << std::endl;
	stream << "DATASET UNSTRUCTURED_GRID" << std::endl;
	stream << "POINTS 8 double" << std::endl;
	for(int i = 0; i < 8; i++)
		stream << corners[i].x() << " " << corners[i].y() << " " << corners[i].z() << std::endl;

	stream << std::endl << "CELLS 1 9" << std::endl;
	stream << "8 0 1 2 3 4 5 6 7" << std::endl;

	stream << std::endl << "CELL_TYPES 1" << std::endl;
	stream << "12" << std::endl;  // Hexahedron
#endif

	nextProgressSubStep();
	FloatType ghostLayerSize = 3.0f * _structureAnalysis.maximumNeighborDistance();
	if(!_tessellation.generateTessellation(_structureAnalysis.cell(), _structureAnalysis.positions()->constDataPoint3(),
			_structureAnalysis.atomCount(), ghostLayerSize, selection() ? selection()->constDataInt() : nullptr, *this))
		return;

	// Build list of edges in the tessellation.
	nextProgressSubStep();
	if(!_elasticMapping.generateTessellationEdges(*this))
		return;

	// Assign each vertex to a cluster.
	nextProgressSubStep();
	if(!_elasticMapping.assignVerticesToClusters(*this))
		return;

	// Determine the ideal vector corresponding to each edge of the tessellation.
	nextProgressSubStep();
	if(!_elasticMapping.assignIdealVectorsToEdges(4, *this))
		return;

	// Free some memory that is no longer needed.
	_structureAnalysis.freeNeighborLists();

	// Create the mesh facets.
	nextProgressSubStep();
	if(!_interfaceMesh.createMesh(_structureAnalysis.maximumNeighborDistance(), crystalClusters().get(), *this))
		return;

	// Trace dislocation lines.
	nextProgressSubStep();
	if(!_dislocationTracer.traceDislocationSegments(*this))
		return;
	_dislocationTracer.finishDislocationSegments(_inputCrystalStructure);

#if 0

	auto isWrappedFacet = [this](const InterfaceMesh::Face* f) -> bool {
		InterfaceMesh::Edge* e = f->edges();
		do {
			Vector3 v = e->vertex1()->pos() - e->vertex2()->pos();
			if(_structureAnalysis.cell().isWrappedVector(v))
				return true;
			e = e->nextFaceEdge();
		}
		while(e != f->edges());
		return false;
	};

	// Count facets which are not crossing the periodic boundaries.
	size_t numFacets = 0;
	for(const InterfaceMesh::Face* f : _interfaceMesh.faces()) {
		if(isWrappedFacet(f) == false)
			numFacets++;
	}

	std::ofstream stream("mesh.vtk");
	stream << "# vtk DataFile Version 3.0\n";
	stream << "# Interface mesh\n";
	stream << "ASCII\n";
	stream << "DATASET UNSTRUCTURED_GRID\n";
	stream << "POINTS " << _interfaceMesh.vertices().size() << " float\n";
	for(const InterfaceMesh::Vertex* n : _interfaceMesh.vertices()) {
		const Point3& pos = n->pos();
		stream << pos.x() << " " << pos.y() << " " << pos.z() << "\n";
	}
	stream << "\nCELLS " << numFacets << " " << (numFacets*4) << "\n";
	for(const InterfaceMesh::Face* f : _interfaceMesh.faces()) {
		if(isWrappedFacet(f) == false) {
			stream << f->edgeCount();
			InterfaceMesh::Edge* e = f->edges();
			do {
				stream << " " << e->vertex1()->index();
				e = e->nextFaceEdge();
			}
			while(e != f->edges());
			stream << "\n";
		}
	}

	stream << "\nCELL_TYPES " << numFacets << "\n";
	for(size_t i = 0; i < numFacets; i++)
		stream << "5\n";	// Triangle

	stream << "\nCELL_DATA " << numFacets << "\n";

	stream << "\nSCALARS dislocation_segment int 1\n";
	stream << "\nLOOKUP_TABLE default\n";
	for(const InterfaceMesh::Face* f : _interfaceMesh.faces()) {
		if(isWrappedFacet(f) == false) {
			if(f->circuit != NULL && (f->circuit->isDangling == false || f->testFlag(1))) {
				DislocationSegment* segment = f->circuit->dislocationNode->segment;
				while(segment->replacedWith != NULL) segment = segment->replacedWith;
				stream << segment->id << "\n";
			}
			else
				stream << "-1\n";
		}
	}

	stream << "\nSCALARS is_primary_segment int 1\n";
	stream << "\nLOOKUP_TABLE default\n";
	for(const InterfaceMesh::Face* f : _interfaceMesh.faces()) {
		if(isWrappedFacet(f) == false)
			stream << f->testFlag(1) << "\n";
	}

	stream.close();
#endif

	// Generate the defect mesh.
	nextProgressSubStep();
	if(!_interfaceMesh.generateDefectMesh(_dislocationTracer, *defectMesh(), *this))
		return;

#if 0
	_tessellation.dumpToVTKFile("tessellation.vtk");
#endif

	nextProgressSubStep();

	// Post-process surface mesh.
	if(_defectMeshSmoothingLevel > 0 && !SurfaceMesh::smoothMesh(*defectMesh(), cell(), _defectMeshSmoothingLevel, *this))
		return;

	nextProgressSubStep();
		
	// Post-process dislocation lines.
	if(_lineSmoothingLevel > 0 || _linePointInterval > 0) {
		if(!dislocationNetwork()->smoothDislocationLines(_lineSmoothingLevel, _linePointInterval, *this))
			return;
	}

	endProgressSubSteps();
	
	// Return the results of the compute engine.
	_results->setCompletelyGoodOrBad(interfaceMesh().isCompletelyGood(), interfaceMesh().isCompletelyBad());
	if(_outputInterfaceMesh) {
		auto outputInterfaceMesh = std::make_shared<HalfEdgeMesh<>>();
		outputInterfaceMesh->copyFrom(interfaceMesh());
		_results->setInterfaceMesh(std::move(outputInterfaceMesh));
	}	
	setResult(std::move(_results));
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

