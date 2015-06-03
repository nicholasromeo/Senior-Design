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

#ifndef __OVITO_CRYSTALANALYSIS_IMPORTER_H
#define __OVITO_CRYSTALANALYSIS_IMPORTER_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/dataset/importexport/FileSourceImporter.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>
#include <core/gui/properties/PropertiesEditor.h>
#include <plugins/particles/import/ParticleFrameLoader.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
#include <plugins/crystalanalysis/data/DislocationNetwork.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief Importer for output files generated by the Crystal Analysis Tool.
 */
class OVITO_CRYSTALANALYSIS_EXPORT CAImporter : public FileSourceImporter
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE CAImporter(DataSet* dataset) : FileSourceImporter(dataset), _loadParticles(false) {
		INIT_PROPERTY_FIELD(CAImporter::_loadParticles);
	}

	/// \brief Returns the file filter that specifies the files that can be imported by this service.
	virtual QString fileFilter() override { return QString("*"); }

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	virtual QString fileFilterDescription() override { return tr("Crystal Analysis files"); }

	/// \brief Checks if the given file has format that can be read by this importer.
	virtual bool checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) override;

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("CAT Output"); }

	/// Returns whether loading of the associated particle file is enabled.
	bool loadParticles() const { return _loadParticles; }

	/// Controls the loading of the associated particle file.
	void setLoadParticles(bool enable) { _loadParticles = enable; }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FrameLoader> createFrameLoader(const Frame& frame) override {
		return std::make_shared<CrystalAnalysisFrameLoader>(dataset()->container(), frame, _loadParticles);
	}

protected:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class CrystalAnalysisFrameLoader : public ParticleFrameLoader
	{
	public:

		/// Constructor.
		CrystalAnalysisFrameLoader(DataSetContainer* container, const FileSourceImporter::Frame& frame, bool loadParticles)
			: ParticleFrameLoader(container, frame, true), _loadParticles(loadParticles), _defectSurface(new HalfEdgeMesh<>()) {}

		/// Inserts the data loaded by perform() into the provided container object. This function is
		/// called by the system from the main thread after the asynchronous loading task has finished.
		virtual void handOver(CompoundObject* container) override;

	protected:

		/// Parses the given input file and stores the data in this container object.
		virtual void parseFile(CompressedTextReader& stream) override;

		struct BurgersVectorFamilyInfo {
			int id;
			QString name;
			Vector3 burgersVector;
			Color color;
		};

		struct PatternInfo {
			int id;
			StructurePattern::StructureType type;
			QString shortName;
			QString longName;
			Color color;
			QVector<BurgersVectorFamilyInfo> burgersVectorFamilies;
		};

		/// The triangle mesh of the defect surface.
		QExplicitlySharedDataPointer<HalfEdgeMesh<>> _defectSurface;

		/// The structure pattern catalog.
		QVector<PatternInfo> _patterns;

		/// The cluster list.
		QExplicitlySharedDataPointer<ClusterGraph> _clusterGraph;

		/// The dislocation segments.
		QExplicitlySharedDataPointer<DislocationNetwork> _dislocations;

		/// Controls whether particles should be loaded too.
		bool _loadParticles;

		/// This is the sub-task task that loads the particles.
		std::shared_ptr<FileSourceImporter::FrameLoader> _particleLoadTask;
	};

	/// This method is called when the scene node for the FileSource is created.
	virtual void prepareSceneNode(ObjectNode* node, FileSource* importObj) override;

	/// \brief Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

private:

	/// Controls whether the associated particle file should be loaded too.
	PropertyField<bool> _loadParticles;

	Q_OBJECT
	OVITO_OBJECT

	DECLARE_PROPERTY_FIELD(_loadParticles);
};

/**
 * \brief A properties editor for the CAImporter class.
 */
class CAImporterEditor : public PropertiesEditor
{
public:

	/// Constructor.
	Q_INVOKABLE CAImporterEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_CRYSTALANALYSIS_IMPORTER_H
