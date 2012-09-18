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
 * \ingroup ThreePThreeCNIModel
 */
/*!
 * \file
 *
 * \brief Defines default values for most properties required by the 3p3cni
 *        box model.
 */
#ifndef DUMUX_3P3CNI_PROPERTY_DEFAULTS_HH
#define DUMUX_3P3CNI_PROPERTY_DEFAULTS_HH

#include <dumux/boxmodels/3p3c/3p3cpropertydefaults.hh>

#include "3p3cnimodel.hh"
#include "3p3cniproblem.hh"
#include "3p3cniindices.hh"
#include "3p3cnilocalresidual.hh"
#include "3p3cnivolumevariables.hh"
#include "3p3cnifluxvariables.hh"

namespace Dumux
{

namespace Properties
{
//////////////////////////////////////////////////////////////////
// Property values
//////////////////////////////////////////////////////////////////

SET_INT_PROP(BoxThreePThreeCNI, NumEq, 4); //!< set the number of equations to 4

//! Use the 3p3cni local jacobian operator for the 3p3cni model
SET_TYPE_PROP(BoxThreePThreeCNI,
              LocalResidual,
              ThreePThreeCNILocalResidual<TypeTag>);

//! the Model property
SET_TYPE_PROP(BoxThreePThreeCNI, Model, ThreePThreeCNIModel<TypeTag>);

//! the VolumeVariables property
SET_TYPE_PROP(BoxThreePThreeCNI, VolumeVariables, ThreePThreeCNIVolumeVariables<TypeTag>);


//! the FluxVariables property
SET_TYPE_PROP(BoxThreePThreeCNI, FluxVariables, ThreePThreeCNIFluxVariables<TypeTag>);

//! The indices required by the non-isothermal 3p3c model
SET_TYPE_PROP(BoxThreePThreeCNI, Indices, ThreePThreeCNIIndices<TypeTag, 0>);

//! DEPRECATED ThreePThreeCIndices and ThreePThreeCNIIndices properties
SET_TYPE_PROP(BoxThreePThreeCNI, ThreePThreeCIndices, typename GET_PROP_TYPE(TypeTag, Indices));
SET_TYPE_PROP(BoxThreePThreeCNI, ThreePThreeCNIIndices, typename GET_PROP_TYPE(TypeTag, Indices));

//! DEPRECATED SpatialParameters property
SET_TYPE_PROP(BoxThreePThreeCNI, SpatialParameters, typename GET_PROP_TYPE(TypeTag, SpatialParams));

}

}
#endif
