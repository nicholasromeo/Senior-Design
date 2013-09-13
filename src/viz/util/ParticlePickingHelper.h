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

#ifndef __OVITO_PARTICLE_PICKING_HELPER_H
#define __OVITO_PARTICLE_PICKING_HELPER_H

#include <core/Core.h>
#include <core/scene/pipeline/PipelineFlowState.h>
#include <core/scene/ObjectNode.h>
#include <core/viewport/Viewport.h>
#include <core/rendering/LineGeometryBuffer.h>
#include <core/rendering/ParticleGeometryBuffer.h>
#include <core/rendering/viewport/ViewportSceneRenderer.h>

namespace Viz {

using namespace Ovito;

/******************************************************************************
* Provides support for picking of atoms.
******************************************************************************/
class ParticlePickingHelper
{
public:

	struct PickResult {

		/// The position of the picked particle in local coordinates.
		Point3 localPos;

		/// The position of the picked particle in world coordinates.
		Point3 worldPos;

		/// The radius of the picked particle.
		FloatType radius;

		/// The index of the picked particle.
		size_t particleIndex;

		/// The identifier of the picked particle.
		int particleId;

		/// The scene node that contains the picked particle.
		OORef<ObjectNode> objNode;
	};

public:

	/// \brief Constructor.
	ParticlePickingHelper() {}

protected:

	/// \brief Finds the particle under the mouse cursor.
	/// \param vp The viewport to perform hit testing in.
	/// \param clickPoint The position of the mouse cursor in the viewport.
	/// \param time The animation at which hit testing is performed.
	/// \param result The output structure that receives information about the picked particle.
	/// \return \c true if a particle has been picked; \c false otherwise.
	bool pickParticle(Viewport* vp, const QPoint& clickPoint, PickResult& result);

	/// \brief Renders the particle selection overlay in a viewport.
	/// \param vp The viewport into which the selection marker should be rendered.
	/// \param renderer The renderer for the viewport.
	/// \param pickRecord Specifies the particle for which the selection marker should be rendered.
	void renderSelectionMarker(Viewport* vp, ViewportSceneRenderer* renderer, const PickResult& pickRecord);

	/// Used to render the marker for a selected particle.
	OORef<LineGeometryBuffer> _markerBuffer;

	/// Used to render the marker for a selected particle.
	OORef<ParticleGeometryBuffer> _markerBuffer2;
};

};	// End of namespace

#endif // __OVITO_PARTICLE_PICKING_HELPER_H
