// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 20010 by Benjamin Faigle                                  *
 *   Copyright (C) 2007-2008 by Bernd Flemisch                               *
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
 * \ingroup IMPETtests
 * \brief test for the sequential 2p2c model
 */
#include "config.h"
#include "test_dec2p2cproblem.hh"
#include <dumux/common/start.hh>

#include <dumux/common/structuredgridcreator.hh>

#include <dune/grid/common/gridinfo.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/mpihelper.hh>
#include <iostream>

/*!
 * \brief Provides an interface for customizing error messages associated with
 *        reading in parameters.
 *
 * \param progName  The name of the program, that was tried to be started.
 * \param errorMsg  The error message that was issued by the start function.
 *                  Comprises the thing that went wrong and a general help message.
 */
void usage(const char *progName, const std::string &errorMsg)
{
    if (errorMsg.size() > 0) {
        std::string errorMessageOut = "\nUsage: ";
                    errorMessageOut += progName;
                    errorMessageOut += " [options]\n";
                    errorMessageOut += errorMsg;
                    errorMessageOut += "\n\nThe List of Mandatory arguments for this program is:\n"
                                        "\t-tEnd                          The end of the simulation. [s] \n"
                                        "\t-dtInitial                     The initial timestep size. [s] \n"
                                        "\t-Grid.numberOfCellsX           Resolution in x-direction [-]\n"
                                        "\t-Grid.numberOfCellsY           Resolution in y-direction [-]\n"
                                        "\t-Grid.numberOfCellsZ           Resolution in z-direction [-]\n"
                                        "\t-Grid.upperRightX              Dimension of the grid [m]\n"
                                        "\t-Grid.upperRightY              Dimension of the grid [m]\n"
                                        "\t-Grid.upperRightZ              Dimension of the grid [m]\n";
        std::cout << errorMessageOut
                  << "\n";
    }
}

////////////////////////
// the main function
////////////////////////
int main(int argc, char** argv)
{
    typedef TTAG(TestDecTwoPTwoCProblem) ProblemTypeTag;
    return Dumux::startWithParameters<ProblemTypeTag>(argc, argv, usage);
}

//! \cond INTERNAL
// set the GridCreator property
namespace Dumux {
namespace Properties {
SET_TYPE_PROP(TestDecTwoPTwoCProblem, GridCreator, CubeGridCreator<TypeTag>);
}}
//! \endcond
