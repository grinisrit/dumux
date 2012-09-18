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
 * \brief Tutorial problem for a fully coupled twophase box model.
 */
#ifndef DUMUX_EX2_TUTORIAL_PROBLEM_COUPLED_HH    // guardian macro /*@\label{tutorial-coupled:guardian1}@*/
#define DUMUX_EX2_TUTORIAL_PROBLEM_COUPLED_HH    // guardian macro /*@\label{tutorial-coupled:guardian2}@*/

// The numerical model
#include <dumux/boxmodels/2p/2pmodel.hh>

// The base prous media box problem
#include <dumux/boxmodels/common/porousmediaboxproblem.hh>

// The DUNE grid used
#include <dune/grid/alugrid.hh>

// Spatially dependent parameters
#include "ex2_tutorialspatialparams_coupled.hh"

// The components that are used
#include <dumux/material/components/h2o.hh>
#include <dumux/material/components/lnapl.hh>
#include <dumux/common/cubegridcreator.hh>

namespace Dumux{
// Forward declaration of the problem class
template <class TypeTag>
class Ex2TutorialProblemCoupled;

namespace Properties {
// Create a new type tag for the problem
NEW_TYPE_TAG(Ex2TutorialProblemCoupled, INHERITS_FROM(BoxTwoP, Ex2TutorialSpatialParamsCoupled)); /*@\label{tutorial-coupled:create-type-tag}@*/

// Set the "Problem" property
SET_PROP(Ex2TutorialProblemCoupled, Problem) /*@\label{tutorial-coupled:set-problem}@*/
{ typedef Dumux::Ex2TutorialProblemCoupled<TypeTag> type;};

// Set grid and the grid creator to be used
SET_TYPE_PROP(Ex2TutorialProblemCoupled, Grid, Dune::ALUCubeGrid</*dim=*/2,2>); /*@\label{tutorial-coupled:set-grid}@*/
SET_TYPE_PROP(Ex2TutorialProblemCoupled, GridCreator, Dumux::CubeGridCreator<TypeTag>); /*@\label{tutorial-coupled:set-gridcreator}@*/

// Set the wetting phase
SET_PROP(Ex2TutorialProblemCoupled, WettingPhase) /*@\label{tutorial-coupled:2p-system-start}@*/
{
private: typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
public: typedef Dumux::LiquidPhase<Scalar, Dumux::H2O<Scalar> > type; /*@\label{tutorial-coupled:wettingPhase}@*/
};

// Set the non-wetting phase
SET_PROP(Ex2TutorialProblemCoupled, NonwettingPhase)
{
private: typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
public: typedef Dumux::LiquidPhase<Scalar, Dumux::LNAPL<Scalar> > type; /*@\label{tutorial-coupled:nonwettingPhase}@*/
}; /*@\label{tutorial-coupled:2p-system-end}@*/

SET_TYPE_PROP(Ex2TutorialProblemCoupled, FluidSystem, Dumux::TwoPImmiscibleFluidSystem<TypeTag>);
// Disable gravity
SET_BOOL_PROP(Ex2TutorialProblemCoupled, EnableGravity, false); /*@\label{tutorial-coupled:gravity}@*/
}

/*!
 * \ingroup TwoPBoxModel
 *
 * \brief  Tutorial problem for a fully coupled twophase box model.
 */
template <class TypeTag>
class Ex2TutorialProblemCoupled : public PorousMediaBoxProblem<TypeTag> /*@\label{tutorial-coupled:def-problem}@*/
{
    typedef PorousMediaBoxProblem<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    // Grid dimension
    enum { dim = GridView::dimension };

    // Types from DUNE-Grid
    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<dim>::Entity Vertex;
    typedef typename GridView::Intersection Intersection;
    typedef Dune::FieldVector<Scalar, dim> GlobalPosition;

    // Dumux specific types
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;

public:
    Ex2TutorialProblemCoupled(TimeManager &timeManager,
                           const GridView &gridView)
        : ParentType(timeManager, gridView)
        , eps_(3e-6)
    {
    }

    //! Specifies the problem name. This is used as a prefix for files
    //! generated by the simulation.
    const char *name() const
    { return "tutorial_coupled"; }

    //! Returns true if a restart file should be written.
    bool shouldWriteRestartFile() const /*@\label{tutorial-coupled:restart}@*/
    { return false; }

    //! Returns true if the current solution should be written to disk
    //! as a VTK file
    bool shouldWriteOutput() const /*@\label{tutorial-coupled:output}@*/
    {
        return
            this->timeManager().timeStepIndex() > 0 &&
            (this->timeManager().timeStepIndex() % 1 == 0);
    }

    //! Returns the temperature within a finite volume. We use constant
    //! 10 degrees Celsius.
    Scalar temperature() const
    { return 283.15; };

    //! Specifies which kind of boundary condition should be used for
    //! which equation for a finite volume on the boundary.
    void boundaryTypes(BoundaryTypes &bcTypes, const Vertex &vertex) const
    {
        const GlobalPosition &globalPos = vertex.geometry().center();
        if (globalPos[0] < eps_) // Dirichlet conditions on left boundary
           bcTypes.setAllDirichlet();
        else // neuman for the remaining boundaries
           bcTypes.setAllNeumann();

    }

    //! Evaluates the Dirichlet boundary conditions for a finite volume
    //! on the grid boundary. Here, the 'values' parameter stores
    //! primary variables.
    void dirichlet(PrimaryVariables &values, const Vertex &vertex) const
    {
        values[Indices::pwIdx] = 5e5; // 500000 Pa = 5 bar
        values[Indices::SnIdx] = 1.0; // 100 % oil saturation on left boundary
    }

    //! Evaluates the boundary conditions for a Neumann boundary
    //! segment. Here, the 'values' parameter stores the mass flux in
    //! [kg/(m^2 * s)] in normal direction of each phase. Negative
    //! values mean influx.
    void neumann(PrimaryVariables &values,
                 const Element &element,
                 const FVElementGeometry &fvGeometry,
                 const Intersection &is,
                 int scvIdx,
                 int boundaryFaceIdx) const
    {
        const GlobalPosition &globalPos =
            fvGeometry.boundaryFace[boundaryFaceIdx].ipGlobal;
        Scalar right = this->bboxMax()[0];
        // extraction of oil on the right boundary for approx. 1.e6 seconds
        if (globalPos[0] > right - eps_) {
            // oil outflux of 30 g/(m * s) on the right boundary.
            values[Indices::contiWEqIdx] = 2e-4;
            values[Indices::contiNEqIdx] = 0;
        } else {
            // no-flow on the remaining Neumann-boundaries.
            values[Indices::contiWEqIdx] = 0;
            values[Indices::contiNEqIdx] = 0;
        }
    }

    //! Evaluates the initial value for a control volume. For this
    //! method, the 'values' parameter stores primary variables.
    void initial(PrimaryVariables &values,
                 const Element &element,
                 const FVElementGeometry &fvGeometry,
                 int scvIdx) const
    {
        values[Indices::pwIdx] = 5e5; // 500 kPa = 5 bar
        values[Indices::SnIdx] = 0.0;
    }

    //! Evaluates the source term for all phases within a given
    //! sub-control-volume. In this case, the 'values' parameter
    //! stores the rate mass generated or annihilated per volume unit
    //! in [kg / (m^3 * s)]. Positive values mean that mass is created.
    void source(PrimaryVariables &values,
                const Element &element,
                const FVElementGeometry &fvGeometry,
                int scvIdx) const
    {
        values[Indices::contiWEqIdx] = 0.0;
        values[Indices::contiNEqIdx]= 0.0;
    }

private:
    // small epsilon value
    Scalar eps_;
};
}

#endif
