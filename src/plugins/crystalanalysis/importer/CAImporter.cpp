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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/utilities/concurrent/Future.h>
#include <core/dataset/importexport/FileSource.h>
#include <core/scene/ObjectNode.h>
#include <core/gui/properties/BooleanParameterUI.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/modifier/SmoothSurfaceModifier.h>
#include <plugins/crystalanalysis/modifier/SmoothDislocationsModifier.h>
#include <plugins/particles/import/lammps/LAMMPSTextDumpImporter.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/particles/objects/SurfaceMeshDisplay.h>
#include "CAImporter.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, CAImporter, FileSourceImporter);
IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, CAImporterEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(CAImporter, CAImporterEditor);
DEFINE_PROPERTY_FIELD(CAImporter, _loadParticles, "LoadParticles");
SET_PROPERTY_FIELD_LABEL(CAImporter, _loadParticles, "Load particles");

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void CAImporter::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(CAImporter::_loadParticles)) {
		requestReload();
	}
	FileSourceImporter::propertyChanged(field);
}

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool CAImporter::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation)
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Read first line.
	stream.readLine(20);

	// Files start with the string "CA_FILE_VERSION ".
	if(stream.lineStartsWith("CA_FILE_VERSION "))
		return true;

	return false;

}
/******************************************************************************
* Reads the data from the input file(s).
******************************************************************************/
void CAImporter::CrystalAnalysisFrameLoader::parseFile(CompressedTextReader& stream)
{
	setProgressText(tr("Reading crystal analysis file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Read file header.
	stream.readLine();
	if(!stream.lineStartsWith("CA_FILE_VERSION "))
		throw Exception(tr("Failed to parse file. This is not a proper file written by the Crystal Analysis Tool."));
	int fileFormatVersion = 0;
	if(sscanf(stream.line(), "CA_FILE_VERSION %i", &fileFormatVersion) != 1)
		throw Exception(tr("Failed to parse file. This is not a proper file written by the Crystal Analysis Tool."));
	if(fileFormatVersion != 4)
		throw Exception(tr("Failed to parse file. This file format version is not supported: %1").arg(fileFormatVersion));
	stream.readLine();
	if(!stream.lineStartsWith("CA_LIB_VERSION"))
		throw Exception(tr("Failed to parse file. This is not a proper file written by the Crystal Analysis Tool."));

	// Read file path information.
	stream.readLine();
	QString caFilename = stream.lineString().mid(12).trimmed();
	stream.readLine();
	QString atomsFilename = stream.lineString().mid(11).trimmed();

	// Read pattern catalog.
	int numPatterns;
	if(sscanf(stream.readLine(), "STRUCTURE_PATTERNS %i", &numPatterns) != 1 || numPatterns <= 0)
		throw Exception(tr("Failed to parse file. Invalid number of structure patterns in line %1.").arg(stream.lineNumber()));
	std::vector<int> patternId2Index;
	for(int index = 0; index < numPatterns; index++) {
		PatternInfo pattern;
		if(sscanf(stream.readLine(), "PATTERN ID %i", &pattern.id) != 1)
			throw Exception(tr("Failed to parse file. Invalid pattern ID in line %1.").arg(stream.lineNumber()));
		if((int)patternId2Index.size() <= pattern.id)
			patternId2Index.resize(pattern.id+1);
		patternId2Index[pattern.id] = index;
		stream.readLine();
		pattern.shortName = stream.lineString().mid(5).trimmed();
		stream.readLine();
		pattern.longName = stream.lineString().mid(9).trimmed();
		stream.readLine();
		QString patternTypeString = stream.lineString().mid(5).trimmed();
		if(patternTypeString == QStringLiteral("LATTICE")) pattern.type = StructurePattern::Lattice;
		else if(patternTypeString == QStringLiteral("INTERFACE")) pattern.type = StructurePattern::Interface;
		else if(patternTypeString == QStringLiteral("POINTDEFECT")) pattern.type = StructurePattern::PointDefect;
		else throw Exception(tr("Failed to parse file. Invalid pattern type in line %1: %2").arg(stream.lineNumber()).arg(patternTypeString));
		if(sscanf(stream.readLine(), "COLOR " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &pattern.color.r(), &pattern.color.g(), &pattern.color.b()) != 3)
			throw Exception(tr("Failed to parse file. Invalid pattern color in line %1.").arg(stream.lineNumber()));
		int numFamilies;
		if(sscanf(stream.readLine(), "BURGERS_VECTOR_FAMILIES %i", &numFamilies) != 1 || numFamilies < 0)
			throw Exception(tr("Failed to parse file. Invalid number of Burgers vectors families in line %1.").arg(stream.lineNumber()));
		for(int familyIndex = 0; familyIndex < numFamilies; familyIndex++) {
			BurgersVectorFamilyInfo family;
			if(sscanf(stream.readLine(), "BURGERS_VECTOR_FAMILY ID %i", &family.id) != 1)
				throw Exception(tr("Failed to parse file. Invalid Burgers vector family ID in line %1.").arg(stream.lineNumber()));
			stream.readLine();
			family.name = stream.lineString().trimmed();
			if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &family.burgersVector.x(), &family.burgersVector.y(), &family.burgersVector.z()) != 3)
				throw Exception(tr("Failed to parse file. Invalid Burgers vector in line %1.").arg(stream.lineNumber()));
			if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &family.color.r(), &family.color.g(), &family.color.b()) != 3)
				throw Exception(tr("Failed to parse file. Invalid color in line %1.").arg(stream.lineNumber()));
			pattern.burgersVectorFamilies.push_back(family);
		}
		stream.readLine();
		_patterns.push_back(pattern);
	}

	// Read simulation cell geometry.
	AffineTransformation cell;
	if(sscanf(stream.readLine(), "SIMULATION_CELL_ORIGIN " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cell(0,3), &cell(1,3), &cell(2,3)) != 3)
		throw Exception(tr("Failed to parse file. Invalid cell origin in line %1.").arg(stream.lineNumber()));
	if(sscanf(stream.readLine(), "SIMULATION_CELL " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
			" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
			" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
			&cell(0,0), &cell(0,1), &cell(0,2), &cell(1,0), &cell(1,1), &cell(1,2), &cell(2,0), &cell(2,1), &cell(2,2)) != 9)
		throw Exception(tr("Failed to parse file. Invalid cell vectors in line %1.").arg(stream.lineNumber()));
	int pbcFlags[3];
	if(sscanf(stream.readLine(), "PBC_FLAGS %i %i %i", &pbcFlags[0], &pbcFlags[1], &pbcFlags[2]) != 3)
		throw Exception(tr("Failed to parse file. Invalid PBC flags in line %1.").arg(stream.lineNumber()));
	simulationCell().setMatrix(cell);
	simulationCell().setPbcFlags(pbcFlags[0], pbcFlags[1], pbcFlags[2]);

	// Read cluster list.
	int numClusters;
	if(sscanf(stream.readLine(), "CLUSTERS %i", &numClusters) != 1)
		throw Exception(tr("Failed to parse file. Invalid number of clusters in line %1.").arg(stream.lineNumber()));
	setProgressText(tr("Reading clusters"));
	setProgressRange(numClusters);
	_clusterGraph = new ClusterGraph();
	for(int index = 0; index < numClusters; index++) {
		if(!setProgressValueIntermittent(index))
			return;
		int patternId, clusterId, clusterProc;
		stream.readLine();
		if(sscanf(stream.readLine(), "%i %i", &clusterId, &clusterProc) != 2)
			throw Exception(tr("Failed to parse file. Invalid cluster ID in line %1.").arg(stream.lineNumber()));
		if(sscanf(stream.readLine(), "%i", &patternId) != 1)
			throw Exception(tr("Failed to parse file. Invalid cluster pattern index in line %1.").arg(stream.lineNumber()));
		Cluster* cluster = _clusterGraph->createCluster(patternId);
		OVITO_ASSERT(cluster->structure != 0);
		if(sscanf(stream.readLine(), "%i", &cluster->atomCount) != 1)
			throw Exception(tr("Failed to parse file. Invalid cluster atom count in line %1.").arg(stream.lineNumber()));
		if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cluster->centerOfMass.x(), &cluster->centerOfMass.y(), &cluster->centerOfMass.z()) != 3)
			throw Exception(tr("Failed to parse file. Invalid cluster center of mass in line %1.").arg(stream.lineNumber()));
		if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
				" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
				" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
				&cluster->orientation(0,0), &cluster->orientation(0,1), &cluster->orientation(0,2),
				&cluster->orientation(1,0), &cluster->orientation(1,1), &cluster->orientation(1,2),
				&cluster->orientation(2,0), &cluster->orientation(2,1), &cluster->orientation(2,2)) != 9)
			throw Exception(tr("Failed to parse file. Invalid cluster orientation matrix in line %1.").arg(stream.lineNumber()));
	}

	// Read cluster transition list.
	int numClusterTransitions;
	if(sscanf(stream.readLine(), "CLUSTER_TRANSITIONS %i", &numClusterTransitions) != 1)
		throw Exception(tr("Failed to parse file. Invalid number of cluster transitions in line %1.").arg(stream.lineNumber()));
	setProgressText(tr("Reading cluster transitions"));
	setProgressRange(numClusterTransitions);
	for(int index = 0; index < numClusterTransitions; index++) {
		if(!setProgressValueIntermittent(index))
			return;
		int clusterIndex1, clusterIndex2;
		if(sscanf(stream.readLine(), "TRANSITION %i %i", &clusterIndex1, &clusterIndex2) != 2 || clusterIndex1 >= numClusters || clusterIndex2 >= numClusters)
			throw Exception(tr("Failed to parse file. Invalid cluster transition in line %1.").arg(stream.lineNumber()));
		Matrix3 tm;
		if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
				" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING
				" " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
				&tm(0,0), &tm(0,1), &tm(0,2),
				&tm(1,0), &tm(1,1), &tm(1,2),
				&tm(2,0), &tm(2,1), &tm(2,2)) != 9)
			throw Exception(tr("Failed to parse file. Invalid cluster transition matrix in line %1.").arg(stream.lineNumber()));
		_clusterGraph->createClusterTransition(_clusterGraph->clusters()[clusterIndex1+1], _clusterGraph->clusters()[clusterIndex2+1], tm);
	}

	// Read dislocations list.
	int numDislocationSegments;
	if(sscanf(stream.readLine(), "DISLOCATIONS %i", &numDislocationSegments) != 1)
		throw Exception(tr("Failed to parse file. Invalid number of dislocation segments in line %1.").arg(stream.lineNumber()));
	setProgressText(tr("Reading dislocations"));
	setProgressRange(numDislocationSegments);
	_dislocations = new DislocationNetwork(_clusterGraph.data());
	for(int index = 0; index < numDislocationSegments; index++) {
		if(!setProgressValueIntermittent(index))
			return;
		int segmentId;
		if(sscanf(stream.readLine(), "%i", &segmentId) != 1)
			throw Exception(tr("Failed to parse file. Invalid segment ID in line %1.").arg(stream.lineNumber()));

		Vector3 burgersVector;
		if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &burgersVector.x(), &burgersVector.y(), &burgersVector.z()) != 3)
			throw Exception(tr("Failed to parse file. Invalid Burgers vector in line %1.").arg(stream.lineNumber()));

		int clusterIndex;
		if(sscanf(stream.readLine(), "%i", &clusterIndex) != 1 || clusterIndex < 0 || clusterIndex >= numClusters)
			throw Exception(tr("Failed to parse file. Invalid segment cluster ID in line %1.").arg(stream.lineNumber()));

		DislocationSegment* segment = _dislocations->createSegment(ClusterVector(burgersVector, _clusterGraph->clusters()[clusterIndex+1]));

		// Read polyline.
		int numPoints;
		if(sscanf(stream.readLine(), "%i", &numPoints) != 1 || numPoints <= 1)
			throw Exception(tr("Failed to parse file. Invalid segment number of points in line %1.").arg(stream.lineNumber()));
		segment->line.resize(numPoints);
		for(Point3& p : segment->line) {
			if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &p.x(), &p.y(), &p.z()) != 3)
				throw Exception(tr("Failed to parse file. Invalid point in line %1.").arg(stream.lineNumber()));
		}

		// Read dislocation core size.
		segment->coreSize.resize(numPoints);
		for(int& coreSize : segment->coreSize) {
			if(sscanf(stream.readLine(), "%i", &coreSize) != 1)
				throw Exception(tr("Failed to parse file. Invalid core size in line %1.").arg(stream.lineNumber()));
		}

	}

	// Read dislocation junctions.
	stream.readLine();
	for(int index = 0; index < numDislocationSegments; index++) {
		DislocationSegment* segment = _dislocations->segments()[index];
		for(int nodeIndex = 0; nodeIndex < 2; nodeIndex++) {
			int isForward, otherSegmentId;
			if(sscanf(stream.readLine(), "%i %i", &isForward, &otherSegmentId) != 2 || otherSegmentId < 0 || otherSegmentId >= numDislocationSegments)
				throw Exception(tr("Failed to parse file. Invalid dislocation junction record in line %1.").arg(stream.lineNumber()));
			segment->nodes[nodeIndex]->junctionRing = _dislocations->segments()[otherSegmentId]->nodes[isForward ? 0 : 1];
		}
	}

	// Read defect mesh vertices.
	int numDefectMeshVertices;
	if(sscanf(stream.readLine(), "DEFECT_MESH_VERTICES %i", &numDefectMeshVertices) != 1)
		throw Exception(tr("Failed to parse file. Invalid number of defect mesh vertices in line %1.").arg(stream.lineNumber()));
	setProgressText(tr("Reading defect surface"));
	setProgressRange(numDefectMeshVertices);
	_defectSurface->reserveVertices(numDefectMeshVertices);
	for(int index = 0; index < numDefectMeshVertices; index++) {
		if(!setProgressValueIntermittent(index)) return;
		Point3 p;
		if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &p.x(), &p.y(), &p.z()) != 3)
			throw Exception(tr("Failed to parse file. Invalid point in line %1.").arg(stream.lineNumber()));
		_defectSurface->createVertex(p);
	}

	// Read defect mesh facets.
	int numDefectMeshFacets;
	if(sscanf(stream.readLine(), "DEFECT_MESH_FACETS %i", &numDefectMeshFacets) != 1)
		throw Exception(tr("Failed to parse file. Invalid number of defect mesh facets in line %1.").arg(stream.lineNumber()));
	setProgressRange(numDefectMeshFacets * 2);
	_defectSurface->reserveFaces(numDefectMeshFacets);
	for(int index = 0; index < numDefectMeshFacets; index++) {
		if(!setProgressValueIntermittent(index))
			return;
		int v[3];
		if(sscanf(stream.readLine(), "%i %i %i", &v[0], &v[1], &v[2]) != 3)
			throw Exception(tr("Failed to parse file. Invalid triangle facet in line %1.").arg(stream.lineNumber()));
		_defectSurface->createFace({ _defectSurface->vertex(v[0]), _defectSurface->vertex(v[1]), _defectSurface->vertex(v[2]) });
	}

	// Read facet adjacency information.
	for(int index = 0; index < numDefectMeshFacets; index++) {
		if(!setProgressValueIntermittent(index + numDefectMeshFacets))
			return;
		int v[3];
		if(sscanf(stream.readLine(), "%i %i %i", &v[0], &v[1], &v[2]) != 3)
			throw Exception(tr("Failed to parse file. Invalid triangle adjacency info in line %1.").arg(stream.lineNumber()));
		HalfEdgeMesh<>::Edge* edge = _defectSurface->face(index)->edges();
		for(int i = 0; i < 3; i++, edge = edge->nextFaceEdge()) {
			OVITO_CHECK_POINTER(edge);
			if(edge->oppositeEdge() != nullptr) continue;
			HalfEdgeMesh<>::Face* oppositeFace = _defectSurface->face(v[i]);
			HalfEdgeMesh<>::Edge* oppositeEdge = oppositeFace->edges();
			do {
				OVITO_CHECK_POINTER(oppositeEdge);
				if(oppositeEdge->vertex1() == edge->vertex2() && oppositeEdge->vertex2() == edge->vertex1()) {
					edge->linkToOppositeEdge(oppositeEdge);
					break;
				}
				oppositeEdge = oppositeEdge->nextFaceEdge();
			}
			while(oppositeEdge != oppositeFace->edges());
			OVITO_ASSERT(edge->oppositeEdge());
		}
	}

	// Load particles if requested by the user.
	if(_loadParticles) {
		FileSourceImporter::Frame particleFileInfo;
		particleFileInfo.byteOffset = 0;
		particleFileInfo.lineNumber = 0;

		// Resolve relative path to atoms file.
		QFileInfo caFileInfo(caFilename);
		QFileInfo atomsFileInfo(atomsFilename);
		if(!atomsFileInfo.isAbsolute()) {
			QDir baseDir = caFileInfo.absoluteDir();
			QString relativePath = baseDir.relativeFilePath(atomsFileInfo.absoluteFilePath());
			if(frame().sourceFile.isLocalFile()) {
				particleFileInfo.sourceFile = QUrl::fromLocalFile(QFileInfo(frame().sourceFile.toLocalFile()).dir().filePath(relativePath));
			}
			else {
				particleFileInfo.sourceFile = frame().sourceFile;
				particleFileInfo.sourceFile.setPath(QFileInfo(frame().sourceFile.path()).dir().filePath(relativePath));
			}
		}
		else particleFileInfo.sourceFile = QUrl::fromLocalFile(atomsFilename);

		// Create and execute the import sub-task.
		_particleLoadTask = LAMMPSTextDumpImporter::createFrameLoader(&datasetContainer(), particleFileInfo, true, false, InputColumnMapping());
		if(!waitForSubTask(_particleLoadTask))
			return;

		setStatus(tr("Number of segments: %1\n%2").arg(numDislocationSegments).arg(_particleLoadTask->status().text()));
	}
	else {
		setStatus(tr("Number of segments: %1").arg(numDislocationSegments));
	}
}

/******************************************************************************
* Inserts the data loaded by perform() into the provided container object.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
void CAImporter::CrystalAnalysisFrameLoader::handOver(CompoundObject* container)
{
	// Make a copy of the list of old data objects in the container so we can re-use some objects.
	PipelineFlowState oldObjects(container->status(), container->dataObjects(), TimeInterval::infinite(), container->attributes());
	// Insert simulation cell.
	ParticleFrameLoader::handOver(container);

	// Insert defect surface.
	OORef<SurfaceMesh> defectSurfaceObj = oldObjects.findObject<SurfaceMesh>();
	if(!defectSurfaceObj) {
		defectSurfaceObj = new SurfaceMesh(container->dataset());
		OORef<SurfaceMeshDisplay> displayObj = new SurfaceMeshDisplay(container->dataset());
		displayObj->loadUserDefaults();
		defectSurfaceObj->setDisplayObject(displayObj);
	}
	defectSurfaceObj->setStorage(_defectSurface.data());

	// Insert pattern catalog.
	OORef<PatternCatalog> patternCatalog = oldObjects.findObject<PatternCatalog>();
	if(!patternCatalog) {
		patternCatalog = new PatternCatalog(container->dataset());
	}

	// Update pattern catalog.
	for(int i = 0; i < _patterns.size(); i++) {
		OORef<StructurePattern> pattern;
		if(patternCatalog->patterns().size() > i+1) {
			pattern = patternCatalog->patterns()[i+1];
		}
		else {
			pattern.reset(new StructurePattern(patternCatalog->dataset()));
			patternCatalog->addPattern(pattern);
		}
		if(pattern->shortName() != _patterns[i].shortName)
			pattern->setColor(_patterns[i].color);
		pattern->setShortName(_patterns[i].shortName);
		pattern->setLongName(_patterns[i].longName);
		pattern->setStructureType(_patterns[i].type);
		pattern->setId(_patterns[i].id);

		// Update Burgers vector families.
		for(int j = 0; j < _patterns[i].burgersVectorFamilies.size(); j++) {
			OORef<BurgersVectorFamily> family;
			if(pattern->burgersVectorFamilies().size() > j+1) {
				family = pattern->burgersVectorFamilies()[j+1];
			}
			else {
				family.reset(new BurgersVectorFamily(pattern->dataset()));
				pattern->addBurgersVectorFamily(family);
			}
			if(family->name() != _patterns[i].burgersVectorFamilies[j].name)
				family->setColor(_patterns[i].burgersVectorFamilies[j].color);
			family->setName(_patterns[i].burgersVectorFamilies[j].name);
			family->setBurgersVector(_patterns[i].burgersVectorFamilies[j].burgersVector);
		}
		// Remove excess families.
		for(int j = pattern->burgersVectorFamilies().size() - 1; j > _patterns[i].burgersVectorFamilies.size(); j--)
			pattern->removeBurgersVectorFamily(j);
	}
	// Remove excess patterns from the catalog.
	for(int i = patternCatalog->patterns().size() - 1; i > _patterns.size(); i--)
		patternCatalog->removePattern(i);

	// Insert cluster graph.
	OORef<ClusterGraphObject> clusterGraph = oldObjects.findObject<ClusterGraphObject>();
	if(!clusterGraph) {
		clusterGraph = new ClusterGraphObject(container->dataset());
	}
	clusterGraph->setStorage(_clusterGraph.data());

	// Insert dislocations.
	OORef<DislocationNetworkObject> dislocationNetwork = oldObjects.findObject<DislocationNetworkObject>();
	if(!dislocationNetwork) {
		dislocationNetwork = new DislocationNetworkObject(container->dataset());
		OORef<DislocationDisplay> displayObj = new DislocationDisplay(container->dataset());
		displayObj->loadUserDefaults();
		dislocationNetwork->setDisplayObject(displayObj);
	}
	dislocationNetwork->setStorage(_dislocations.data());

	// Insert particles.
	if(_particleLoadTask) {
		_particleLoadTask->handOver(container);

		// Copy structure patterns into StructureType particle property.
		for(DataObject* dataObj : container->dataObjects()) {
			ParticleTypeProperty* structureTypeProperty = dynamic_object_cast<ParticleTypeProperty>(dataObj);
			if(structureTypeProperty && structureTypeProperty->type() == ParticleProperty::StructureTypeProperty) {
				structureTypeProperty->clearParticleTypes();
				for(StructurePattern* pattern : patternCatalog->patterns()) {
					structureTypeProperty->addParticleType(pattern);
				}
			}
		}
	}

	container->addDataObject(defectSurfaceObj);
	container->addDataObject(patternCatalog);
	container->addDataObject(clusterGraph);
	container->addDataObject(dislocationNetwork);
}

/******************************************************************************
* This method is called when the scene node for the FileSource is created.
******************************************************************************/
void CAImporter::prepareSceneNode(ObjectNode* node, FileSource* importObj)
{
	FileSourceImporter::prepareSceneNode(node, importObj);

	// Add a modifier to smooth the defect surface mesh.
	node->applyModifier(new SmoothSurfaceModifier(node->dataset()));

	// Add a modifier to smooth the dislocation lines.
	node->applyModifier(new SmoothDislocationsModifier(node->dataset()));
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CAImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Crystal analysis file"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	BooleanParameterUI* loadParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(CAImporter::_loadParticles));
	layout->addWidget(loadParticlesUI->checkBox());
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
