// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2011 by Holger Class                                      *
 *   Copyright (C) 2010 by Andreas Lauser                                    *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief Binary coefficients for water and tce.
 */
#ifndef DUMUX_BINARY_COEFF_H2O_HEAVYOIL_HH
#define DUMUX_BINARY_COEFF_H2O_HEAVYOIL_HH

#include <dumux/material/components/h2o.hh>
#include <dumux/material/components/heavyoil.hh>

namespace Dumux
{
namespace BinaryCoeff
{

/*!
 * \brief Binary coefficients for water and heavy oil as in SAGD processes
 */
class H2O_HeavyOil
{
public:
    /*!
     * \brief Henry coefficent \f$[N/m^2]\f$  for heavy oil in liquid water.
     *
     * See:
     *
     */

    template <class Scalar>
    static Scalar henryOilInWater(Scalar temperature)
    {
        // values copied from TCE, TODO: improve this!!
        Scalar dumuxH = 1.5e-1 / 101.325; // unit [(mol/m^3)/Pa]
        dumuxH *= 18.02e-6;  //multiplied by molar volume of reference phase = water
        return 1.0/dumuxH; // [Pa]
    }

    /*!
     * \brief Henry coefficent \f$[N/m^2]\f$  for water in liquid heavy oil.
     *
     * See:
     *
     */

    template <class Scalar>
    static Scalar henryWaterInOil(Scalar temperature)
    {
        // arbitrary, TODO: improve it!!
        return 1.0e8; // [Pa]
    }


    /*!
     * \brief Binary diffusion coefficent [m^2/s] for molecular water and heavy oil.
     *
     */
    template <class Scalar>
    static Scalar gasDiffCoeff(Scalar temperature, Scalar pressure)
    {
        return 1e-6; // [m^2/s] This is just an order of magnitude. Please improve it!
    }

    /*!
     * \brief Diffusion coefficent [m^2/s] for tce in liquid water.
     *
     * \todo
     */
    template <class Scalar>
    static Scalar liquidDiffCoeff(Scalar temperature, Scalar pressure)
    {
        return 1.e-9;  // This is just an order of magnitude. Please improve it!
    }
};

}
} // end namespace

#endif
