// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
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
 * \ingroup TracerTests
 * \brief Test for the tracer CC model.
 */

#include <config.h>

#include <ctime>
#include <iostream>

#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/timer.hh>
#include <dune/grid/io/file/dgfparser/dgfexception.hh>
#include <dune/grid/io/file/vtk.hh>

#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/dumuxmessage.hh>

#include <dumux/linear/seqsolverbackend.hh>
#include <dumux/linear/pdesolver.hh>

#include <dumux/assembly/fvassembler.hh>
#include <dumux/assembly/diffmethod.hh>

#include <dumux/io/vtkoutputmodule.hh>
#include <dumux/io/grid/gridmanager_yasp.hh>

#include "problem_1p.hh"
#include "problem_tracer.hh"

int main(int argc, char** argv) try
{
    using namespace Dumux;

    //! define the type tags for this problem
    using OnePTypeTag = Properties::TTag::IncompressibleTest;
    using TracerTypeTag = Properties::TTag::TracerTestCC;

    //! initialize MPI, finalize is done automatically on exit
    const auto& mpiHelper = Dune::MPIHelper::instance(argc, argv);

    //! print dumux start message
    if (mpiHelper.rank() == 0)
        DumuxMessage::print(/*firstCall=*/true);

    ////////////////////////////////////////////////////////////
    // parse the command line arguments and input file
    ////////////////////////////////////////////////////////////

    //! parse command line arguments
    Parameters::init(argc, argv);

    //////////////////////////////////////////////////////////////////////
    // try to create a grid (from the given grid file or the input file)
    /////////////////////////////////////////////////////////////////////

    // only create the grid once using the 1p type tag
    GridManager<GetPropType<OnePTypeTag, Properties::Grid>> gridManager;
    gridManager.init();

    //! we compute on the leaf grid view
    const auto& leafGridView = gridManager.grid().leafGridView();

    ////////////////////////////////////////////////////////////
    // setup & solve 1p problem on this grid
    ////////////////////////////////////////////////////////////
    Dune::Timer timer;

    //! create the finite volume grid geometry
    using GridGeometry = GetPropType<OnePTypeTag, Properties::GridGeometry>;
    auto gridGeometry = std::make_shared<GridGeometry>(leafGridView);
    gridGeometry->update();

    //! the problem (boundary conditions)
    using OnePProblem = GetPropType<OnePTypeTag, Properties::Problem>;
    auto problemOneP = std::make_shared<OnePProblem>(gridGeometry);

    //! the solution vector
    using SolutionVector = GetPropType<OnePTypeTag, Properties::SolutionVector>;
    SolutionVector p(leafGridView.size(0));

    //! the grid variables
    using OnePGridVariables = GetPropType<OnePTypeTag, Properties::GridVariables>;
    auto onePGridVariables = std::make_shared<OnePGridVariables>(problemOneP, gridGeometry);
    onePGridVariables->init(p);

    //! the assembler
    using OnePAssembler = FVAssembler<OnePTypeTag, DiffMethod::analytic>;
    auto assemblerOneP = std::make_shared<OnePAssembler>(problemOneP, gridGeometry, onePGridVariables);

    //! the linear solver
    using OnePLinearSolver = UMFPackBackend;
    auto onePLinearSolver = std::make_shared<OnePLinearSolver>();

    //! the pde system solver
    LinearPDESolver<OnePAssembler, OnePLinearSolver> onePSolver(assemblerOneP, onePLinearSolver);
    onePSolver.solve(p);

    //! write output to vtk
    using GridView = GetPropType<OnePTypeTag, Properties::GridView>;
    Dune::VTKWriter<GridView> onepWriter(leafGridView);
    onepWriter.addCellData(p, "p");
    const auto& k = problemOneP->spatialParams().getKField();
    onepWriter.addCellData(k, "permeability");
    onepWriter.write("1p");

    timer.stop();

    const auto& comm = Dune::MPIHelper::getCollectiveCommunication();
    std::cout << "Simulation took " << timer.elapsed() << " seconds on "
              << comm.size() << " processes.\n"
              << "The cumulative CPU time was " << timer.elapsed()*comm.size() << " seconds.\n";

    ////////////////////////////////////////////////////////////
    // compute volume fluxes for the tracer model
    ////////////////////////////////////////////////////////////
    using Scalar =  GetPropType<OnePTypeTag, Properties::Scalar>;
    std::vector<Scalar> volumeFlux(gridGeometry->numScvf(), 0.0);

    using FluxVariables =  GetPropType<OnePTypeTag, Properties::FluxVariables>;
    auto upwindTerm = [](const auto& volVars) { return volVars.mobility(0); };
    for (const auto& element : elements(leafGridView))
    {
        auto fvGeometry = localView(*gridGeometry);
        fvGeometry.bind(element);

        auto elemVolVars = localView(onePGridVariables->curGridVolVars());
        elemVolVars.bind(element, fvGeometry, p);

        auto elemFluxVars = localView(onePGridVariables->gridFluxVarsCache());
        elemFluxVars.bind(element, fvGeometry, elemVolVars);

        for (const auto& scvf : scvfs(fvGeometry))
        {
            const auto idx = scvf.index();

            if (!scvf.boundary())
            {
                FluxVariables fluxVars;
                fluxVars.init(*problemOneP, element, fvGeometry, elemVolVars, scvf, elemFluxVars);
                volumeFlux[idx] = fluxVars.advectiveFlux(0, upwindTerm);
            }
            else
            {
                const auto bcTypes = problemOneP->boundaryTypes(element, scvf);
                if (bcTypes.hasOnlyDirichlet())
                {
                    FluxVariables fluxVars;
                    fluxVars.init(*problemOneP, element, fvGeometry, elemVolVars, scvf, elemFluxVars);
                    volumeFlux[idx] = fluxVars.advectiveFlux(0, upwindTerm);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////
    // setup & solve tracer problem on the same grid
    ////////////////////////////////////////////////////////////

    //! the problem (initial and boundary conditions)
    using TracerProblem = GetPropType<TracerTypeTag, Properties::Problem>;
    auto tracerProblem = std::make_shared<TracerProblem>(gridGeometry);

    // set the flux from the 1p problem
    tracerProblem->spatialParams().setVolumeFlux(volumeFlux);

    //! the solution vector
    SolutionVector x;
    tracerProblem->applyInitialSolution(x);
    auto xOld = x;

    //! the grid variables
    using GridVariables = GetPropType<TracerTypeTag, Properties::GridVariables>;
    auto gridVariables = std::make_shared<GridVariables>(tracerProblem, gridGeometry);
    gridVariables->init(x);

    //! intialize the vtk output module
    VtkOutputModule<GridVariables, SolutionVector> vtkWriter(*gridVariables, x, tracerProblem->name());
    using IOFields = GetPropType<TracerTypeTag, Properties::IOFields>;
    IOFields::initOutputModule(vtkWriter); // Add model specific output fields
    using VelocityOutput = GetPropType<TracerTypeTag, Properties::VelocityOutput>;
    vtkWriter.addVelocityOutput(std::make_shared<VelocityOutput>(*gridVariables));
    vtkWriter.write(0.0);

    //! get some time loop parameters
    const auto tEnd = getParam<Scalar>("TimeLoop.TEnd");
    auto dt = getParam<Scalar>("TimeLoop.DtInitial");
    const auto maxDt = getParam<Scalar>("TimeLoop.MaxTimeStepSize");

    //! instantiate time loop
    auto timeLoop = std::make_shared<CheckPointTimeLoop<Scalar>>(0.0, dt, tEnd);
    timeLoop->setMaxTimeStepSize(maxDt);

    //! the assembler with time loop for instationary problem
    using TracerAssembler = FVAssembler<TracerTypeTag, DiffMethod::analytic, /*implicit=*/false>;
    auto assembler = std::make_shared<TracerAssembler>(tracerProblem, gridGeometry, gridVariables, timeLoop, xOld);

    //! the linear solver
    using TracerLinearSolver = ExplicitDiagonalSolver;
    auto linearSolver = std::make_shared<TracerLinearSolver>();

    //! the pde system solver
    LinearPDESolver<TracerAssembler, TracerLinearSolver> solver(assembler, linearSolver);

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // run instationary non-linear simulation
    /////////////////////////////////////////////////////////////////////////////////////////////////

    //! set some check points for the time loop
    timeLoop->setPeriodicCheckPoint(tEnd/10.0);

    //! start the time loop
    timeLoop->start(); do
    {
        // assemble, solve, update
        solver.solve(x);

        // make the new solution the old solution
        xOld = x;
        gridVariables->advanceTimeStep();

        // advance to the time loop to the next step
        timeLoop->advanceTimeStep();

        // write vtk output on check points
        if (timeLoop->isCheckPoint())
            vtkWriter.write(timeLoop->time());

        // report statistics of this time step
        timeLoop->reportTimeStep();

        // set new dt
        timeLoop->setTimeStepSize(dt);

    } while (!timeLoop->finished());

    timeLoop->finalize(leafGridView.comm());

    ////////////////////////////////////////////////////////////
    // finalize, print dumux message to say goodbye
    ////////////////////////////////////////////////////////////

    //! print dumux end message
    if (mpiHelper.rank() == 0)
        DumuxMessage::print(/*firstCall=*/false);

    return 0;

}
catch (const Dumux::ParameterException &e)
{
    std::cerr << std::endl << e << " ---> Abort!" << std::endl;
    return 1;
}
catch (const Dune::DGFException & e)
{
    std::cerr << "DGF exception thrown (" << e <<
                 "). Most likely, the DGF file name is wrong "
                 "or the DGF file is corrupted, "
                 "e.g. missing hash at end of file or wrong number (dimensions) of entries."
                 << " ---> Abort!" << std::endl;
    return 2;
}
catch (const Dune::Exception &e)
{
    std::cerr << "Dune reported error: " << e << " ---> Abort!" << std::endl;
    return 3;
}
catch (...)
{
    std::cerr << "Unknown exception thrown! ---> Abort!" << std::endl;
    return 4;
}
