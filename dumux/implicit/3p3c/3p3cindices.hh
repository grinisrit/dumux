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
 * \file
 * \brief Defines the indices required for the three-phase three-component
 *        fully implicit model.
 */
#ifndef DUMUX_3P3C_INDICES_HH
#define DUMUX_3P3C_INDICES_HH

#include "3p3cproperties.hh"

namespace Dumux
{

/*!
 * \ingroup ThreePThreeCModel
 * \ingroup ImplicitIndices
 * \brief The indices for the isothermal three-phase three-component model.
 *
 * \tparam formulation The formulation, only pgSwSn is available.
 * \tparam PVOffset The first index in a primary variable vector.
 */
template <class TypeTag, int PVOffset = 0>
class ThreePThreeCIndices
{
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

public:
    // Phase indices
    static const int wPhaseIdx = FluidSystem::wPhaseIdx; //!< index of the wetting liquid phase
    static const int nPhaseIdx = FluidSystem::nPhaseIdx; //!< index of the nonwetting liquid phase
    static const int gPhaseIdx = FluidSystem::gPhaseIdx; //!< index of the gas phase

    // Component indices to indicate the main component 
    // of the corresponding phase at atmospheric pressure 1 bar 
    // and room temperature 20°C:
    static const int wCompIdx = FluidSystem::wCompIdx; 
    static const int nCompIdx = FluidSystem::nCompIdx; 
    static const int gCompIdx = FluidSystem::gCompIdx; 

    // present phases (-> 'pseudo' primary variable)
    static const int threePhases = 1; //!< All three phases are present
    static const int wPhaseOnly = 2; //!< Only the water phase is present
    static const int gnPhaseOnly = 3; //!< Only gas and NAPL phases are present
    static const int wnPhaseOnly = 4; //!< Only water and NAPL phases are present
    static const int gPhaseOnly = 5; //!< Only gas phase is present
    static const int wgPhaseOnly = 6; //!< Only water and gas phases are present

    // Primary variable indices
    static const int pressureIdx = PVOffset + 0; //!< Index for gas phase pressure in a solution vector
    static const int switch1Idx = PVOffset + 1; //!< Index 1 of saturation or mole fraction
    static const int switch2Idx = PVOffset + 2; //!< Index 2 of saturation or mole fraction

    static const int pgIdx = pressureIdx; //!< Index for gas phase pressure in a solution vector
    static const int sOrX1Idx = switch1Idx; //!< Index of the either the saturation of the gas phase or the mass fraction secondary component if a phase is not present
    static const int sOrX2Idx = switch2Idx; //!< Index of the either the saturation of the gas phase or the mass fraction secondary component if a phase is not present

    DUNE_DEPRECATED_MSG("use sOrX1Idx (uncapitalized 's') instead") 
    static const int SOrX1Idx = sOrX1Idx; //!< \deprecated
    DUNE_DEPRECATED_MSG("use sOrX2Idx (uncapitalized 's') instead") 
    static const int SOrX2Idx = sOrX2Idx; //!< \deprecated

    // equation indices
    static const int conti0EqIdx = PVOffset    + wCompIdx; //!< Index of the mass conservation equation for the water component
    static const int conti1EqIdx = conti0EqIdx + nCompIdx; //!< Index of the mass conservation equation for the contaminant component
    static const int conti2EqIdx = conti0EqIdx + gCompIdx; //!< Index of the mass conservation equation for the gas component

    static const int contiWEqIdx = conti0EqIdx + wCompIdx; //!< index of the mass conservation equation for the water component
    static const int contiNEqIdx = conti0EqIdx + nCompIdx; //!< index of the mass conservation equation for the contaminant component
    static const int contiGEqIdx = conti0EqIdx + gCompIdx; //!< index of the mass conservation equation for the air component
};

}

#endif
