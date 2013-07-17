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

#ifndef __OVITO_INVERT_SELECTION_MODIFIER_H
#define __OVITO_INVERT_SELECTION_MODIFIER_H

#include <core/Core.h>
#include "../ParticleModifier.h"

namespace Viz {

using namespace Ovito;

/**
 * \brief This modifier inverts the selection status of each particle.
 */
class InvertSelectionModifier : public ParticleModifier
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE InvertSelectionModifier() {}

	/// \brief Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override { return TimeInterval::forever(); }

protected:

	/// Modifies the particle object. The time interval passed
	/// to the function is reduced to the interval where the modified object is valid/constant.
	virtual ObjectStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Invert Selection");
	Q_CLASSINFO("ModifierCategory", "Selection");
};

};	// End of namespace

#endif // __OVITO_INVERT_SELECTION_MODIFIER_H