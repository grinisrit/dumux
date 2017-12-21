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
 * \ingroup MPNCModel
 * \brief Adds vtk output fields specific to the twop model
 */
#ifndef DUMUX_MPNC_VTK_OUTPUT_FIELDS_HH
#define DUMUX_MPNC_VTK_OUTPUT_FIELDS_HH

namespace Dumux
{

/*!
 * \ingroup MPNCModel
 * \brief Adds vtk output fields specific to the twop model
 */
template<class TypeTag>
class MPNCVtkOutputFields
{
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);

    static constexpr int numPhases = GET_PROP_VALUE(TypeTag, NumPhases);
    static constexpr int numComponents = GET_PROP_VALUE(TypeTag, NumComponents);

public:
    template <class VtkOutputModule>
    static void init(VtkOutputModule& vtk)
    {
        for (int i = 0; i < numPhases; ++i)
            vtk.addVolumeVariable([i](const VolumeVariables& v){ return v.saturation(i); }, "S_"+ FluidSystem::phaseName(i));

       for (int i = 0; i < numPhases; ++i)
            vtk.addVolumeVariable([i](const VolumeVariables& v){ return v.pressure(i); }, "p_"+ FluidSystem::phaseName(i));

       for (int i = 0; i < numPhases; ++i)
            vtk.addVolumeVariable([i](const VolumeVariables& v){ return v.density(i); }, "rho_"+ FluidSystem::phaseName(i));

       for (int i = 0; i < numPhases; ++i)
                  vtk.addVolumeVariable([i](const VolumeVariables& v){ return v.mobility(i); },"lambda_"+ FluidSystem::phaseName(i));

        vtk.addVolumeVariable([](const VolumeVariables& v){ return v.porosity(); }, "porosity");

        for (int i = 0; i < numPhases; ++i)
            for (int j = 0; j < numComponents; ++j)
                vtk.addVolumeVariable([i,j](const VolumeVariables& v){ return v.moleFraction(i,j); },"x_"+ FluidSystem::phaseName(i) + "^" + FluidSystem::componentName(j));
    }
};

} // end namespace Dumux

#endif
