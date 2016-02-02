// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/

/*!
 * \ingroup MPFA
 * \ingroup Properties
 * \file
 *
 * \brief Properties for the MPFA-O method.
 */
#ifndef DUMUX_FVMPFAL3DPROPERTIES2P_ADAPTIVE_HH
#define DUMUX_FVMPFAL3DPROPERTIES2P_ADAPTIVE_HH

// dumux environment
#include <dumux/porousmediumflow/2p/sequential/diffusion/properties.hh>
#include <dumux/porousmediumflow/sequential/cellcentered/mpfa/properties.hh>

namespace Dumux
{
namespace Properties
{
NEW_TYPE_TAG(FvMpfaL3dPressureTwoPAdaptive, INHERITS_FROM(PressureTwoP, MPFAProperties));
}
}

#include <dumux/porousmediumflow/2p/sequential/diffusion/mpfa/lmethod/3dpressurevelocityadaptive.hh>
#include <dumux/porousmediumflow/sequential/cellcentered/mpfa/velocityintransport.hh>

namespace Dumux
{
namespace Properties
{
SET_TYPE_PROP(FvMpfaL3dPressureTwoPAdaptive, MPFAInteractionVolume, Dumux::FvMpfaL3dInteractionVolumeAdaptive<TypeTag>);
SET_TYPE_PROP(FvMpfaL3dPressureTwoPAdaptive, MPFAInteractionVolumeContainer, Dumux::FvMpfaL3dInteractionVolumeContainerAdaptive<TypeTag>);
SET_TYPE_PROP(FvMpfaL3dPressureTwoPAdaptive, PressureModel, Dumux::FvMpfaL3dPressureVelocity2pAdaptive<TypeTag>);
SET_TYPE_PROP( FvMpfaL3dPressureTwoPAdaptive, Velocity, Dumux::FvMpfaVelocityInTransport<TypeTag> );
}
}// end of Dune namespace
#endif