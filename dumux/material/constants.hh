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
 *
 * \brief A central place for various physical constants occuring in
 *        some equations.
 */
#ifndef DUMUX_CONSTANTS_HH
#define DUMUX_CONSTANTS_HH

#include <cmath>

namespace Dumux
{

/*!
 * \brief A central place for various physical constants occuring in
 *        some equations.
 */
template<class Scalar>
class Constants
{ public:
    /*!
     * \brief The ideal gas constant [J/(mol K)]
     */
    static const Scalar R;

    /*!
     * \brief The Avogadro constant [1/mol]
     */
    static const Scalar Na;

    /*!
     * \brief The Boltzmann constant [J/K]
     */
    static const Scalar kb;

    /*!
     * \brief Speed of light in vacuum [m/s]
     */
    static const Scalar c;

    /*!
     * \brief Newtonian constant of gravitation [m^3/(kg s^2)]
     */
    static const Scalar G;

    /*!
     * \brief Planck constant [J s]
     */
    static const Scalar h;

    /*!
     * \brief Reduced Planck constant [J s]
     */
    static const Scalar hRed;
};

template<class Scalar>
const Scalar Constants<Scalar>::R = 8.314472;
template<class Scalar>
const Scalar Constants<Scalar>::Na = 6.02214179e23;
template<class Scalar>
const Scalar Constants<Scalar>::kb = Constants<Scalar>::R/Constants<Scalar>::Na;
template<class Scalar>
const Scalar Constants<Scalar>::c = 299792458;
template<class Scalar>
const Scalar Constants<Scalar>::G = 6.67428e-11;
template<class Scalar>
const Scalar Constants<Scalar>::h = 6.62606896e-34;
template<class Scalar>
const Scalar Constants<Scalar>::hRed = Constants<Scalar>::h / (2 * M_PI);
}

#endif
