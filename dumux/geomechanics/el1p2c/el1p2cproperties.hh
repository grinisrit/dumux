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
 * \ingroup Properties
 * \ingroup ImplicitProperties
 * \ingroup ElOnePTwoCBoxModel
 * \file
 *
 * \brief Defines the properties required for the one-phase two-component
 * linear elasticity model.
 *
 * This class inherits from the properties of the one-phase two-component model and
 * from the properties of the linear elasticity model
 */

#ifndef DUMUX_ELASTIC1P2C_PROPERTIES_HH
#define DUMUX_ELASTIC1P2C_PROPERTIES_HH

#include <dumux/implicit/1p2c/1p2cproperties.hh>
#include <dumux/geomechanics/elastic/elasticproperties.hh>


namespace Dumux
{
// \{
namespace Properties
{
//////////////////////////////////////////////////////////////////
// Type tags
//////////////////////////////////////////////////////////////////

//! The type tag for the single-phase, two-component linear elasticity problems
NEW_TYPE_TAG(BoxElasticOnePTwoC, INHERITS_FROM(BoxModel));

//////////////////////////////////////////////////////////////////
// Property tags
//////////////////////////////////////////////////////////////////
NEW_PROP_TAG(NumPhases);   //!< Number of fluid phases in the system
NEW_PROP_TAG(PhaseIdx); //!< A phase index in to allow that a two-phase fluidsystem is used
NEW_PROP_TAG(NumComponents);   //!< Number of fluid components in the system
NEW_PROP_TAG(Indices); //!< Enumerations for the 1p2c model
NEW_PROP_TAG(FluidSystem ); //!<The fluid systems including the information about the phases
NEW_PROP_TAG(SpatialParams); //!< The type of the soil properties object
NEW_PROP_TAG(UseMoles); //!Defines whether mole (true) or mass (false) fractions are used
NEW_PROP_TAG(ProblemEnableGravity); //!< Returns whether gravity is considered in the problem
NEW_PROP_TAG(ImplicitMassUpwindWeight);   //!< The default value of the upwind weight
//!< Returns whether the stabilization terms are included in the balance equations
NEW_PROP_TAG(ImplicitWithStabilization);
//!< Returns whether the output should be written according to rock mechanics sign convention (compressive stresses > 0)
NEW_PROP_TAG(VtkRockMechanicsSignConvention);
}
// \}
}

#endif

