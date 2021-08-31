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
 * \ingroup NavierStokesTests
 * \brief Test for the instationary staggered grid Navier-Stokes model
 *        with analytical solution.
 */

#include <config.h>

#include <ctime>
#include <iostream>
#include <type_traits>
#include <tuple>

#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/timer.hh>
#include <dune/grid/io/file/vtk.hh>

#include <dumux/assembly/staggeredfvassembler.hh>
#include <dumux/assembly/diffmethod.hh>
#include <dumux/common/dumuxmessage.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/properties.hh>
#include <dumux/io/grid/gridmanager_yasp.hh>
#include <dumux/io/staggeredvtkoutputmodule.hh>
#include <dumux/linear/seqsolverbackend.hh>
#include <dumux/linear/linearsolvertraits.hh>
#include <dumux/linear/istlsolverfactorybackend.hh>
#include <dumux/nonlinear/newtonsolver.hh>

#include <test/freeflow/navierstokes/analyticalsolutionvectors.hh>
#include <test/freeflow/navierstokes/errors.hh>

#include "properties.hh"

/*!
* \brief Creates analytical solution.
* Returns a tuple of the analytical solution for the pressure, the velocity and the velocity at the faces
* \param time the time at which to evaluate the analytical solution
* \param problem the problem for which to evaluate the analytical solution
*/
template<class Problem>
auto createSource(const Problem& problem)
{
    using Scalar = double;
    using Indices = typename Problem::Indices;

    const auto& gridGeometry = problem.gridGeometry();
    std::array<std::vector<Scalar>, Problem::ModelTraits::numEq()> source;

    for (auto& component : source)
    {
        component.resize(gridGeometry.numCellCenterDofs());
    }

    auto fvGeometry = localView(gridGeometry);
    for (const auto& element : elements(gridGeometry.gridView()))
    {
        fvGeometry.bindElement(element);
        for (auto&& scv : scvs(fvGeometry))
        {
            const auto ccDofIdx = scv.dofIndex();
            const auto ccDofPosition = scv.dofPosition();

            const auto sourceAtPosVal = problem.sourceAtPos(ccDofPosition);

            source[Indices::momentumXBalanceIdx][ccDofIdx] = sourceAtPosVal[Indices::momentumXBalanceIdx];
            source[Indices::momentumYBalanceIdx][ccDofIdx] = sourceAtPosVal[Indices::momentumYBalanceIdx];
        }
    }

    return source;
}

int main(int argc, char** argv)
{
    using namespace Dumux;

    // define the type tag for this problem
    using TypeTag = Properties::TTag::SincosTest;

    // initialize MPI, finalize is done automatically on exit
    const auto& mpiHelper = Dune::MPIHelper::instance(argc, argv);

    // print dumux start message
    if (mpiHelper.rank() == 0)
        DumuxMessage::print(/*firstCall=*/true);

    // parse command line arguments and input file
    Parameters::init(argc, argv);

    // try to create a grid (from the given grid file or the input file)
    GridManager<GetPropType<TypeTag, Properties::Grid>> gridManager;
    gridManager.init();

    ////////////////////////////////////////////////////////////
    // run instationary non-linear problem on this grid
    ////////////////////////////////////////////////////////////

    // we compute on the leaf grid view
    const auto& leafGridView = gridManager.grid().leafGridView();

    // create the finite volume grid geometry
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    auto gridGeometry = std::make_shared<GridGeometry>(leafGridView);

    // get some time loop parameters
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    const auto tEnd = getParam<Scalar>("TimeLoop.TEnd");
    const auto maxDt = getParam<Scalar>("TimeLoop.MaxTimeStepSize");
    auto dt = getParam<Scalar>("TimeLoop.DtInitial");

    // instantiate time loop
    auto timeLoop = std::make_shared<TimeLoop<Scalar>>(0.0, dt, tEnd);
    timeLoop->setMaxTimeStepSize(maxDt);

    // the problem (initial and boundary conditions)
    using Problem = GetPropType<TypeTag, Properties::Problem>;
    auto problem = std::make_shared<Problem>(gridGeometry);
    problem->updateTimeStepSize(timeLoop->timeStepSize());

    // the solution vector
    using SolutionVector = GetPropType<TypeTag, Properties::SolutionVector>;
    SolutionVector x;
    x[GridGeometry::cellCenterIdx()].resize(gridGeometry->numCellCenterDofs());
    x[GridGeometry::faceIdx()].resize(gridGeometry->numFaceDofs());
    problem->applyInitialSolution(x);
    auto xOld = x;

    // the grid variables
    using GridVariables = GetPropType<TypeTag, Properties::GridVariables>;
    auto gridVariables = std::make_shared<GridVariables>(problem, gridGeometry);
    gridVariables->init(x);

    // initialize the vtk output module
    StaggeredVtkOutputModule<GridVariables, SolutionVector> vtkWriter(*gridVariables, x, problem->name());
    using IOFields = GetPropType<TypeTag, Properties::IOFields>;
    IOFields::initOutputModule(vtkWriter); // Add model specific output fields

    auto source = createSource(*problem);
    auto sourceX = source[Problem::Indices::momentumXBalanceIdx];
    auto sourceY = source[Problem::Indices::momentumYBalanceIdx];
    vtkWriter.addField(sourceX, "sourceX");
    vtkWriter.addField(sourceY, "sourceY");

    NavierStokesAnalyticalSolutionVectors analyticalSolVectors(problem, 0.0);
    vtkWriter.addField(analyticalSolVectors.getAnalyticalPressureSolution(), "pressureExact");
    vtkWriter.addField(analyticalSolVectors.getAnalyticalVelocitySolution(), "velocityExact");
    vtkWriter.addFaceField(analyticalSolVectors.getAnalyticalVelocitySolutionOnFace(), "faceVelocityExact");

    vtkWriter.write(0.0);

    const bool isStationary = getParam<bool>("Problem.IsStationary");

    // the assembler with time loop for instationary problem
    using Assembler = StaggeredFVAssembler<TypeTag, DiffMethod::numeric>;
    auto assembler = isStationary ? std::make_shared<Assembler>(problem, gridGeometry, gridVariables)
                                  : std::make_shared<Assembler>(problem, gridGeometry, gridVariables, timeLoop, xOld);

    // the linear solver
    using LinearSolver = LINEARSOLVER;
    auto linearSolver = std::make_shared<LinearSolver>();

    // the non-linear solver
    using NewtonSolver = Dumux::NewtonSolver<Assembler, LinearSolver>;
    NewtonSolver nonLinearSolver(assembler, linearSolver);

    // the discrete L2 and Linfity errors
    const bool printErrors = getParam<bool>("Problem.PrintErrors", false);
    const NavierStokesErrors<Problem> errors(problem);
    const NavierStokesErrorCSVWriter<Problem> errorCSVWriter(problem);

    if (isStationary)
    {
        // linearize & solve
        Dune::Timer timer;
        nonLinearSolver.solve(x);

        // print discrete L2 and Linfity errors
        if (printErrors)
        {
            using PrimaryVariables = GetPropType<TypeTag, Properties::PrimaryVariables>;

            PrimaryVariables l2NormAbs(0.0);
            PrimaryVariables l2NormRel(0.0);
            PrimaryVariables lInfinityNormAbs(0.0);
            PrimaryVariables lInfinityNormRel(0.0);

            errors.calculateErrors(l2NormAbs, l2NormRel, lInfinityNormAbs, lInfinityNormRel, x);

            if (printErrors)
            {
                errorCSVWriter.printErrors(l2NormAbs, l2NormRel, lInfinityNormAbs, lInfinityNormRel);
            }

            const bool printConvergenceTestFile = getParam<bool>("Problem.PrintConvergenceTestFile", false);
            if (printConvergenceTestFile)
            {
                const NavierStokesErrorConvergenceTestFileWriter<Problem> errorConvergenceTestFileWriter(problem);
                errorConvergenceTestFileWriter.printConvergenceTestFile(l2NormAbs);
            }
        }

        // write vtk output
        analyticalSolVectors.update();
        vtkWriter.write(1.0);

        timer.stop();

        const auto& comm = Dune::MPIHelper::getCollectiveCommunication();
        std::cout << "Simulation took " << timer.elapsed() << " seconds on "
                << comm.size() << " processes.\n"
                << "The cumulative CPU time was " << timer.elapsed()*comm.size() << " seconds.\n";
    }
    else
    {
        // time loop
        timeLoop->start(); do
        {
            // solve the non-linear system with time step control
            nonLinearSolver.solve(x, *timeLoop);

            // make the new solution the old solution
            xOld = x;
            gridVariables->advanceTimeStep();

            // print discrete L2 and Linfity errors
            if (printErrors)
            {
                using PrimaryVariables = GetPropType<TypeTag, Properties::PrimaryVariables>;

                PrimaryVariables l2NormAbs(0.0);
                PrimaryVariables l2NormRel(0.0);
                PrimaryVariables lInfinityNormAbs(0.0);
                PrimaryVariables lInfinityNormRel(0.0);

                errors.calculateErrors(l2NormAbs, l2NormRel, lInfinityNormAbs, lInfinityNormRel, x,timeLoop->time() + timeLoop->timeStepSize());
                errorCSVWriter.printErrors(l2NormAbs, l2NormRel, lInfinityNormAbs, lInfinityNormRel, timeLoop->time() + timeLoop->timeStepSize());
            }

            // advance to the time loop to the next step
            timeLoop->advanceTimeStep();
            problem->updateTime(timeLoop->time());
            analyticalSolVectors.update(timeLoop->time());

            // write vtk output
            vtkWriter.write(timeLoop->time());

            // report statistics of this time step
            timeLoop->reportTimeStep();

            // set new dt as suggested by newton solver
            timeLoop->setTimeStepSize(nonLinearSolver.suggestTimeStepSize(timeLoop->timeStepSize()));
            problem->updateTimeStepSize(timeLoop->timeStepSize());

        } while (!timeLoop->finished());

        timeLoop->finalize(leafGridView.comm());
    }

    ////////////////////////////////////////////////////////////
    // finalize, print dumux message to say goodbye
    ////////////////////////////////////////////////////////////

    // print dumux end message
    if (mpiHelper.rank() == 0)
    {
        Parameters::print();
        DumuxMessage::print(/*firstCall=*/false);
    }

    return 0;
} // end main
