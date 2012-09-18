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
 * \brief Adaption of the BOX scheme to the three-phase three-component flow model.
 *
 * The model is designed for simulating three fluid phases with water, gas, and
 * a liquid contaminant (NAPL - non-aqueous phase liquid)
 */
#ifndef DUMUX_3P3C_MODEL_HH
#define DUMUX_3P3C_MODEL_HH

#include <dumux/material/fluidstates/compositionalfluidstate.hh>

#include "3p3cproperties.hh"
#include "3p3clocalresidual.hh"
// #include "3p3cproblem.hh"

namespace Dumux
{
/*!
 * \ingroup ThreePThreeCModel
 * \brief Adaption of the BOX scheme to the three-phase three-component flow model.
 *
 * This model implements three-phase three-component flow of three fluid phases
 * \f$\alpha \in \{ water, gas, NAPL \}\f$ each composed of up to three components
 * \f$\kappa \in \{ water, air, contaminant \}\f$. The standard multiphase Darcy
 * approach is used as the equation for the conservation of momentum:
 * \f[
 v_\alpha = - \frac{k_{r\alpha}}{\mu_\alpha} \mbox{\bf K}
 \left(\text{grad}\, p_\alpha - \varrho_{\alpha} \mbox{\bf g} \right)
 * \f]
 *
 * By inserting this into the equations for the conservation of the
 * components, one transport equation for each component is obtained as
 * \f{eqnarray*}
 && \phi \frac{\partial (\sum_\alpha \varrho_{\text{mol}, \alpha} x_\alpha^\kappa
 S_\alpha )}{\partial t}
 - \sum\limits_\alpha \text{div} \left\{ \frac{k_{r\alpha}}{\mu_\alpha}
 \varrho_{\text{mol}, \alpha} x_\alpha^\kappa \mbox{\bf K}
 (\text{grad}\, p_\alpha - \varrho_{\text{mass}, \alpha} \mbox{\bf g}) \right\}
 \nonumber \\
 \nonumber \\
 && - \sum\limits_\alpha \text{div} \left\{ D_{pm}^\kappa \varrho_{\text{mol},
 \alpha } \text{grad}\, x_\alpha^\kappa \right\}
 - q^\kappa = 0 \qquad \forall \kappa , \; \forall \alpha
 \f}
 *
 * Note that these balance equations are molar.
 *
 * The equations are discretized using a fully-coupled vertex
 * centered finite volume (BOX) scheme as spatial scheme and
 * the implicit Euler method as temporal discretization.
 *
 * The model uses commonly applied auxiliary conditions like
 * \f$S_w + S_n + S_g = 1\f$ for the saturations and
 * \f$x^w_\alpha + x^a_\alpha + x^c_\alpha = 1\f$ for the mole fractions.
 * Furthermore, the phase pressures are related to each other via
 * capillary pressures between the fluid phases, which are functions of
 * the saturation, e.g. according to the approach of Parker et al.
 *
 * The used primary variables are dependent on the locally present fluid phases
 * An adaptive primary variable switch is included. The phase state is stored for all nodes
 * of the system. The following cases can be distinguished:
 * <ul>
 *  <li> All three phases are present: Primary variables are two saturations \f$(S_w\f$ and \f$S_n)\f$, and a pressure, in this case \f$p_g\f$. </li>
 *  <li> Only the water phase is present: Primary variables are now the mole fractions of air and contaminant in the water phase \f$(x_w^a\f$ and \f$x_w^c)\f$, as well as the gas pressure, which is, of course, in a case where only the water phase is present, just the same as the water pressure. </li>
 *  <li> Gas and NAPL phases are present: Primary variables \f$(S_n\f$, \f$x_g^w\f$, \f$p_g)\f$. </li>
 *  <li> Water and NAPL phases are present: Primary variables \f$(S_n\f$, \f$x_w^a\f$, \f$p_g)\f$. </li>
 *  <li> Only gas phase is present: Primary variables \f$(x_g^w\f$, \f$x_g^c\f$, \f$p_g)\f$. </li>
 *  <li> Water and gas phases are present: Primary variables \f$(S_w\f$, \f$x_w^g\f$, \f$p_g)\f$. </li>
 * </ul>
 */
template<class TypeTag>
class ThreePThreeCModel: public GET_PROP_TYPE(TypeTag, BaseModel)
{
    typedef typename GET_PROP_TYPE(TypeTag, BaseModel) ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, SolutionVector) SolutionVector;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    enum {
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld,

        numPhases = GET_PROP_VALUE(TypeTag, NumPhases),
        numComponents = GET_PROP_VALUE(TypeTag, NumComponents),

        pressureIdx = Indices::pressureIdx,
        switch1Idx = Indices::switch1Idx,
        switch2Idx = Indices::switch2Idx,


        wPhaseIdx = Indices::wPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx,
        gPhaseIdx = Indices::gPhaseIdx,

        wCompIdx = Indices::wCompIdx,
        nCompIdx = Indices::nCompIdx,
        gCompIdx = Indices::gCompIdx,

        threePhases = Indices::threePhases,
        wPhaseOnly  = Indices::wPhaseOnly,
        gnPhaseOnly = Indices::gnPhaseOnly,
        wnPhaseOnly = Indices::wnPhaseOnly,
        gPhaseOnly  = Indices::gPhaseOnly,
        wgPhaseOnly = Indices::wgPhaseOnly

    };


    typedef typename GridView::template Codim<dim>::Entity Vertex;
    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<0>::Iterator ElementIterator;
    typedef typename GridView::template Codim<dim>::Iterator VertexIterator;

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
public:
    /*!
     * \brief Initialize the static data with the initial solution.
     *
     * \param problem The problem to be solved
     */
    void init(Problem &problem)
    {
        ParentType::init(problem);

        staticVertexDat_.resize(this->gridView_().size(dim));

        setSwitched_(false);

        VertexIterator it = this->gridView_().template begin<dim> ();
        const VertexIterator &endit = this->gridView_().template end<dim> ();
        for (; it != endit; ++it)
        {
            int globalIdx = this->dofMapper().map(*it);
            const GlobalPosition &globalPos = it->geometry().corner(0);

            // initialize phase presence
            staticVertexDat_[globalIdx].phasePresence
                = this->problem_().initialPhasePresence(*it, globalIdx,
                                                        globalPos);
            staticVertexDat_[globalIdx].wasSwitched = false;

            staticVertexDat_[globalIdx].oldPhasePresence
                = staticVertexDat_[globalIdx].phasePresence;
        }
    }

    /*!
     * \brief Compute the total storage inside one phase of all
     *        conservation quantities.
     *
     * \param dest Contains the storage of each component for one phase
     * \param phaseIdx The phase index
     */
    void globalPhaseStorage(PrimaryVariables &dest, int phaseIdx)
    {
        dest = 0;

        ElementIterator elemIt = this->gridView_().template begin<0>();
        const ElementIterator elemEndIt = this->gridView_().template end<0>();
        for (; elemIt != elemEndIt; ++elemIt) {
            this->localResidual().evalPhaseStorage(*elemIt, phaseIdx);

            for (int i = 0; i < elemIt->template count<dim>(); ++i)
                dest += this->localResidual().residual(i);
        }

        if (this->gridView_().comm().size() > 1)
            dest = this->gridView_().comm().sum(dest);
    }

    /*!
     * \brief Called by the update() method if applying the newton
     *         method was unsuccessful.
     */
    void updateFailed()
    {
        ParentType::updateFailed();

        setSwitched_(false);
        resetPhasePresence_();
    };

    /*!
     * \brief Returns the relative weight of a primary variable for
     *        calculating relative errors.
     *
     * \param globalVertexIdx The global vertex index
     * \param pvIdx The primary variable index
     */
    Scalar primaryVarWeight(int globalVertexIdx, int pvIdx) const
    {
        if (Indices::pressureIdx == pvIdx)
            return std::min(1.0/this->prevSol()[globalVertexIdx][pvIdx], 1.0);
        return 1;
    }

    /*!
     * \brief Called by the problem if a time integration was
     *        successful, post processing of the solution is done and the
     *        result has been written to disk.
     *
     * This should prepare the model for the next time integration.
     */
    void advanceTimeLevel()
    {
        ParentType::advanceTimeLevel();

        // update the phase state
        updateOldPhasePresence_();
        setSwitched_(false);
    }

    /*!
     * \brief Return true if the primary variables were switched for
     *        at least one vertex after the last timestep.
     */
    bool switched() const
    {
        return switchFlag_;
    }

    /*!
     * \brief Returns the phase presence of the current or the old solution of a vertex.
     *
     * \param globalVertexIdx The global vertex index
     * \param oldSol Evaluate function with solution of current or previous time step
     */
    int phasePresence(int globalVertexIdx, bool oldSol) const
    {
        return
            oldSol
            ? staticVertexDat_[globalVertexIdx].oldPhasePresence
            : staticVertexDat_[globalVertexIdx].phasePresence;
    }

    /*!
     * \brief Append all quantities of interest which can be derived
     *        from the solution of the current time step to the VTK
     *        writer.
     *
     * \param sol The solution vector
     * \param writer The writer for multi-file VTK datasets
     */
    template<class MultiWriter>
    void addOutputVtkFields(const SolutionVector &sol,
                            MultiWriter &writer)
    {
        typedef Dune::BlockVector<Dune::FieldVector<double, 1> > ScalarField;

        // create the required scalar fields
        unsigned numVertices = this->problem_().gridView().size(dim);

        ScalarField *saturation[numPhases];
        ScalarField *pressure[numPhases];
        ScalarField *density[numPhases];

        for (int phaseIdx = 0; phaseIdx < numPhases; ++ phaseIdx) {
            saturation[phaseIdx] = writer.allocateManagedBuffer(numVertices);
            pressure[phaseIdx] = writer.allocateManagedBuffer(numVertices);
            density[phaseIdx] = writer.allocateManagedBuffer(numVertices);
        }

        ScalarField *phasePresence = writer.allocateManagedBuffer (numVertices);
        ScalarField *moleFraction[numPhases][numComponents];
        for (int i = 0; i < numPhases; ++i)
            for (int j = 0; j < numComponents; ++j)
                moleFraction[i][j] = writer.allocateManagedBuffer (numVertices);
        ScalarField *temperature = writer.allocateManagedBuffer (numVertices);
        ScalarField *poro = writer.allocateManagedBuffer(numVertices);
        ScalarField *perm = writer.allocateManagedBuffer(numVertices);

        unsigned numElements = this->gridView_().size(0);
        ScalarField *rank =
            writer.allocateManagedBuffer (numElements);

        FVElementGeometry fvGeometry;
        VolumeVariables volVars;

        ElementIterator elemIt = this->gridView_().template begin<0>();
        ElementIterator elemEndIt = this->gridView_().template end<0>();
        for (; elemIt != elemEndIt; ++elemIt)
        {
            int idx = this->problem_().elementMapper().map(*elemIt);
            (*rank)[idx] = this->gridView_().comm().rank();
            fvGeometry.update(this->gridView_(), *elemIt);

            int numVerts = elemIt->template count<dim> ();
            for (int i = 0; i < numVerts; ++i)
            {
                int globalIdx = this->vertexMapper().map(*elemIt, i, dim);
                volVars.update(sol[globalIdx],
                               this->problem_(),
                               *elemIt,
                               fvGeometry,
                               i,
                               false);

                for (int phaseIdx = 0; phaseIdx < numPhases; ++ phaseIdx) {
                    (*saturation[phaseIdx])[globalIdx] = volVars.fluidState().saturation(phaseIdx);
                    (*pressure[phaseIdx])[globalIdx] = volVars.fluidState().pressure(phaseIdx);
                    (*density[phaseIdx])[globalIdx] = volVars.fluidState().density(phaseIdx);
                }

                for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
                    for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                        (*moleFraction[phaseIdx][compIdx])[globalIdx] =
                            volVars.fluidState().moleFraction(phaseIdx,
                                                              compIdx);

                        Valgrind::CheckDefined((*moleFraction[phaseIdx][compIdx])[globalIdx]);
                    }
                }

                (*poro)[globalIdx] = volVars.porosity();
                (*perm)[globalIdx] = volVars.permeability();
                (*temperature)[globalIdx] = volVars.temperature();
                (*phasePresence)[globalIdx] = staticVertexDat_[globalIdx].phasePresence;
            }

        }

        writer.attachVertexData(*saturation[wPhaseIdx], "Sw");
        writer.attachVertexData(*saturation[nPhaseIdx], "Sn");
        writer.attachVertexData(*saturation[gPhaseIdx], "Sg");
        writer.attachVertexData(*pressure[wPhaseIdx], "pw");
        writer.attachVertexData(*pressure[nPhaseIdx], "pn");
        writer.attachVertexData(*pressure[gPhaseIdx], "pg");
        writer.attachVertexData(*density[wPhaseIdx], "rhow");
        writer.attachVertexData(*density[nPhaseIdx], "rhon");
        writer.attachVertexData(*density[gPhaseIdx], "rhog");

        for (int i = 0; i < numPhases; ++i)
        {
            for (int j = 0; j < numComponents; ++j)
            {
                std::ostringstream oss;
                oss << "x^"
                    << FluidSystem::phaseName(i)
                    << "_"
                    << FluidSystem::componentName(j);
                writer.attachVertexData(*moleFraction[i][j], oss.str().c_str());
            }
        }
        writer.attachVertexData(*poro, "porosity");
        writer.attachVertexData(*perm, "permeability");
        writer.attachVertexData(*temperature, "temperature");
        writer.attachVertexData(*phasePresence, "phase presence");
        writer.attachCellData(*rank, "process rank");
    }

    /*!
     * \brief Write the current solution to a restart file.
     *
     * \param outStream The output stream of one vertex for the restart file
     * \param vert The vertex
     */
    void serializeEntity(std::ostream &outStream, const Vertex &vert)
    {
        // write primary variables
        ParentType::serializeEntity(outStream, vert);

        int vertIdx = this->dofMapper().map(vert);
        if (!outStream.good())
            DUNE_THROW(Dune::IOError, "Could not serialize vertex " << vertIdx);

        outStream << staticVertexDat_[vertIdx].phasePresence << " ";
    }

    /*!
     * \brief Reads the current solution for a vertex from a restart
     *        file.
     *
     * \param inStream The input stream of one vertex from the restart file
     * \param vert The vertex
     */
    void deserializeEntity(std::istream &inStream, const Vertex &vert)
    {
        // read primary variables
        ParentType::deserializeEntity(inStream, vert);

        // read phase presence
        int vertIdx = this->dofMapper().map(vert);
        if (!inStream.good())
            DUNE_THROW(Dune::IOError,
                       "Could not deserialize vertex " << vertIdx);

        inStream >> staticVertexDat_[vertIdx].phasePresence;
        staticVertexDat_[vertIdx].oldPhasePresence
            = staticVertexDat_[vertIdx].phasePresence;

    }

    /*!
     * \brief Update the static data of all vertices in the grid.
     *
     * \param curGlobalSol The current global solution
     * \param oldGlobalSol The previous global solution
     */
    void updateStaticData(SolutionVector &curGlobalSol,
                          const SolutionVector &oldGlobalSol)
    {
        bool wasSwitched = false;

        for (unsigned i = 0; i < staticVertexDat_.size(); ++i)
            staticVertexDat_[i].visited = false;

        FVElementGeometry fvGeometry;
        static VolumeVariables volVars;
        ElementIterator it = this->gridView_().template begin<0> ();
        const ElementIterator &endit = this->gridView_().template end<0> ();
        for (; it != endit; ++it)
        {
            fvGeometry.update(this->gridView_(), *it);
            for (int i = 0; i < fvGeometry.numVertices; ++i)
            {
                int globalIdx = this->vertexMapper().map(*it, i, dim);

                if (staticVertexDat_[globalIdx].visited)
                    continue;

                staticVertexDat_[globalIdx].visited = true;
                volVars.update(curGlobalSol[globalIdx],
                               this->problem_(),
                               *it,
                               fvGeometry,
                               i,
                               false);
                const GlobalPosition &global = it->geometry().corner(i);
                if (primaryVarSwitch_(curGlobalSol,
                                      volVars,
                                      globalIdx,
                                      global))
                    wasSwitched = true;
            }
        }

        // make sure that if there was a variable switch in an
        // other partition we will also set the switch flag
        // for our partition.
        if (this->gridView_().comm().size() > 1)
            wasSwitched = this->gridView_().comm().max(wasSwitched);

        setSwitched_(wasSwitched);
    }

protected:
    /*!
     * \brief Data which is attached to each vertex and is not only
     *        stored locally.
     */
    struct StaticVars
    {
        int phasePresence;
        bool wasSwitched;

        int oldPhasePresence;
        bool visited;
    };

    /*!
     * \brief Reset the current phase presence of all vertices to the old one.
     *
     * This is done after an update failed.
     */
    void resetPhasePresence_()
    {
        int numVertices = this->gridView_().size(dim);
        for (int i = 0; i < numVertices; ++i)
        {
            staticVertexDat_[i].phasePresence
                = staticVertexDat_[i].oldPhasePresence;
            staticVertexDat_[i].wasSwitched = false;
        }
    }

    /*!
     * \brief Set the old phase of all verts state to the current one.
     */
    void updateOldPhasePresence_()
    {
        int numVertices = this->gridView_().size(dim);
        for (int i = 0; i < numVertices; ++i)
        {
            staticVertexDat_[i].oldPhasePresence
                = staticVertexDat_[i].phasePresence;
            staticVertexDat_[i].wasSwitched = false;
        }
    }

    /*!
     * \brief Set whether there was a primary variable switch after in
     *        the last timestep.
     */
    void setSwitched_(bool yesno)
    {
        switchFlag_ = yesno;
    }

    //  perform variable switch at a vertex; Returns true if a
    //  variable switch was performed.
    bool primaryVarSwitch_(SolutionVector &globalSol,
                           const VolumeVariables &volVars,
                           int globalIdx,
                           const GlobalPosition &globalPos)
    {
        // evaluate primary variable switch
        bool wouldSwitch = false;
        int phasePresence = staticVertexDat_[globalIdx].phasePresence;
        int newPhasePresence = phasePresence;

        // check if a primary var switch is necessary
        if (phasePresence == threePhases)
        {
            Scalar Smin = 0;
            if (staticVertexDat_[globalIdx].wasSwitched)
                Smin = -0.01;

            if (volVars.saturation(gPhaseIdx) <= Smin)
            {
                wouldSwitch = true;
                // gas phase disappears
                std::cout << "Gas phase disappears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", Sg: "
                          << volVars.saturation(gPhaseIdx) << std::endl;
                newPhasePresence = wnPhaseOnly;

                globalSol[globalIdx][switch1Idx]
                    = volVars.fluidState().moleFraction(wPhaseIdx, gCompIdx);
            }
            else if (volVars.saturation(wPhaseIdx) <= Smin)
            {
                wouldSwitch = true;
                // water phase disappears
                std::cout << "Water phase disappears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", Sw: "
                          << volVars.saturation(wPhaseIdx) << std::endl;
                newPhasePresence = gnPhaseOnly;

                globalSol[globalIdx][switch1Idx]
                    = volVars.fluidState().moleFraction(gPhaseIdx, wCompIdx);
            }
            else if (volVars.saturation(nPhaseIdx) <= Smin)
            {
                wouldSwitch = true;
                // NAPL phase disappears
                std::cout << "NAPL phase disappears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", Sn: "
                          << volVars.saturation(nPhaseIdx) << std::endl;
                newPhasePresence = wgPhaseOnly;

                globalSol[globalIdx][switch2Idx]
                    = volVars.fluidState().moleFraction(gPhaseIdx, nCompIdx);
            }
        }
        else if (phasePresence == wPhaseOnly)
        {
            bool gasFlag = 0;
            bool nonwettingFlag = 0;
            // calculate fractions of the partial pressures in the
            // hypothetical gas phase
            Scalar xwg = volVars.fluidState().moleFraction(gPhaseIdx, wCompIdx);
            Scalar xgg = volVars.fluidState().moleFraction(gPhaseIdx, gCompIdx);
            Scalar xng = volVars.fluidState().moleFraction(gPhaseIdx, nCompIdx);
            /* take care:
               for xgg in case wPhaseOnly we compute xgg=henry_air*x2w
               for xwg in case wPhaseOnly we compute xwg=pwsat
               for xng in case wPhaseOnly we compute xng=henry_NAPL*x1w
            */

            Scalar xgMax = 1.0;
            if (xwg + xgg + xng > xgMax)
                wouldSwitch = true;
            if (staticVertexDat_[globalIdx].wasSwitched)
                xgMax *= 1.02;

            // if the sum of the mole fractions would be larger than
            // 100%, gas phase appears
            if (xwg + xgg + xng > xgMax)
            {
                // gas phase appears
                std::cout << "gas phase appears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", xwg + xgg + xng: "
                          << xwg + xgg + xng << std::endl;
                gasFlag = 1;
            }

            // calculate fractions in the hypothetical NAPL phase
            Scalar xnn = volVars.fluidState().moleFraction(nPhaseIdx, nCompIdx);
            /* take care:
               for xnn in case wPhaseOnly we compute xnn=henry_mesitylene*x1w,
               where a hypothetical gas pressure is assumed for the Henry
               x0n is set to NULL  (all NAPL phase is dirty)
               x2n is set to NULL  (all NAPL phase is dirty)
            */

            Scalar xnMax = 1.0;
            if (xnn > xnMax)
                wouldSwitch = true;
            if (staticVertexDat_[globalIdx].wasSwitched)
                xnMax *= 1.02;

            // if the sum of the hypothetical mole fractions would be larger than
            // 100%, NAPL phase appears
            if (xnn > xnMax)
            {
                // NAPL phase appears
                std::cout << "NAPL phase appears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", xnn: "
                          << xnn << std::endl;
                nonwettingFlag = 1;
            }

            if ((gasFlag == 1) && (nonwettingFlag == 0))
            {
                newPhasePresence = wgPhaseOnly;
                globalSol[globalIdx][switch1Idx] = 0.9999;
                globalSol[globalIdx][switch2Idx] = 0.0001;
            }
            else if ((gasFlag == 1) && (nonwettingFlag == 1))
            {
                newPhasePresence = threePhases;
                globalSol[globalIdx][switch1Idx] = 0.9999;
                globalSol[globalIdx][switch2Idx] = 0.0001;
            }
            else if ((gasFlag == 0) && (nonwettingFlag == 1))
            {
                newPhasePresence = wnPhaseOnly;
                globalSol[globalIdx][switch1Idx]
                    = volVars.fluidState().moleFraction(wPhaseIdx, gCompIdx);
                globalSol[globalIdx][switch2Idx] = 0.0001;
            }
        }
        else if (phasePresence == gnPhaseOnly)
        {
            bool nonwettingFlag = 0;
            bool wettingFlag = 0;

            Scalar Smin = 0.0;
            if (staticVertexDat_[globalIdx].wasSwitched)
                Smin = -0.01;

            if (volVars.saturation(nPhaseIdx) <= Smin)
            {
                wouldSwitch = true;
                // NAPL phase disappears
                std::cout << "NAPL phase disappears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", Sn: "
                          << volVars.saturation(nPhaseIdx) << std::endl;
                nonwettingFlag = 1;
            }


            // calculate fractions of the hypothetical water phase
            Scalar xww = volVars.fluidState().moleFraction(wPhaseIdx, wCompIdx);
            /*
              take care:, xww, if no water is present, then take xww=xwg*pg/pwsat .
              If this is larger than 1, then water appears
            */
            Scalar xwMax = 1.0;
            if (xww > xwMax)
                wouldSwitch = true;
            if (staticVertexDat_[globalIdx].wasSwitched)
                xwMax *= 1.02;

            // if the sum of the mole fractions would be larger than
            // 100%, gas phase appears
            if (xww > xwMax)
            {
                // water phase appears
                std::cout << "water phase appears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", xww=xwg*pg/pwsat : "
                          << xww << std::endl;
                wettingFlag = 1;
            }

            if ((wettingFlag == 1) && (nonwettingFlag == 0))
            {
                newPhasePresence = threePhases;
                globalSol[globalIdx][switch1Idx] = 0.0001;
                globalSol[globalIdx][switch2Idx] = volVars.saturation(nPhaseIdx);
            }
            else if ((wettingFlag == 1) && (nonwettingFlag == 1))
            {
                newPhasePresence = wgPhaseOnly;
                globalSol[globalIdx][switch1Idx] = 0.0001;
                globalSol[globalIdx][switch2Idx]
                    = volVars.fluidState().moleFraction(gPhaseIdx, nCompIdx);
            }
            else if ((wettingFlag == 0) && (nonwettingFlag == 1))
            {
                newPhasePresence = gPhaseOnly;
                globalSol[globalIdx][switch1Idx]
                    = volVars.fluidState().moleFraction(gPhaseIdx, wCompIdx);
                globalSol[globalIdx][switch2Idx]
                    = volVars.fluidState().moleFraction(gPhaseIdx, nCompIdx);
            }
        }
        else if (phasePresence == wnPhaseOnly)
        {
            bool nonwettingFlag = 0;
            bool gasFlag = 0;

            Scalar Smin = 0.0;
            if (staticVertexDat_[globalIdx].wasSwitched)
                Smin = -0.01;

            if (volVars.saturation(nPhaseIdx) <= Smin)
            {
                wouldSwitch = true;
                // NAPL phase disappears
                std::cout << "NAPL phase disappears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", Sn: "
                          << volVars.saturation(nPhaseIdx) << std::endl;
                nonwettingFlag = 1;
            }

            // calculate fractions of the partial pressures in the
            // hypothetical gas phase
            Scalar xwg = volVars.fluidState().moleFraction(gPhaseIdx, wCompIdx);
            Scalar xgg = volVars.fluidState().moleFraction(gPhaseIdx, gCompIdx);
            Scalar xng = volVars.fluidState().moleFraction(gPhaseIdx, nCompIdx);
            /* take care:
               for xgg in case wPhaseOnly we compute xgg=henry_air*x2w
               for xwg in case wPhaseOnly we compute xwg=pwsat
               for xng in case wPhaseOnly we compute xng=henry_NAPL*x1w
            */
            Scalar xgMax = 1.0;
            if (xwg + xgg + xng > xgMax)
                wouldSwitch = true;
            if (staticVertexDat_[globalIdx].wasSwitched)
                xgMax *= 1.02;

            // if the sum of the mole fractions would be larger than
            // 100%, gas phase appears
            if (xwg + xgg + xng > xgMax)
            {
                // gas phase appears
                std::cout << "gas phase appears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", xwg + xgg + xng: "
                          << xwg + xgg + xng << std::endl;
                gasFlag = 1;
            }

            if ((gasFlag == 1) && (nonwettingFlag == 0))
            {
                newPhasePresence = threePhases;
                globalSol[globalIdx][switch1Idx] = volVars.saturation(wPhaseIdx);
                globalSol[globalIdx][switch2Idx] = volVars.saturation(nPhaseIdx);;
            }
            else if ((gasFlag == 1) && (nonwettingFlag == 1))
            {
                newPhasePresence = wgPhaseOnly;
                globalSol[globalIdx][switch1Idx] = volVars.saturation(wPhaseIdx);
                globalSol[globalIdx][switch2Idx]
                    = volVars.fluidState().moleFraction(gPhaseIdx, nCompIdx);
            }
            else if ((gasFlag == 0) && (nonwettingFlag == 1))
            {
                newPhasePresence = wPhaseOnly;
                globalSol[globalIdx][switch1Idx]
                    = volVars.fluidState().moleFraction(wPhaseIdx, gCompIdx);
                globalSol[globalIdx][switch2Idx]
                    = volVars.fluidState().moleFraction(wPhaseIdx, nCompIdx);
            }
        }
        else if (phasePresence == gPhaseOnly)
        {
            bool nonwettingFlag = 0;
            bool wettingFlag = 0;

            // calculate fractions in the hypothetical NAPL phase
            Scalar xnn = volVars.fluidState().moleFraction(nPhaseIdx, nCompIdx);
            /*
              take care:, xnn, if no NAPL phase is there, take xnn=xng*pg/pcsat
              if this is larger than 1, then NAPL appears
            */

            Scalar xnMax = 1.0;
            if (xnn > xnMax)
                wouldSwitch = true;
            if (staticVertexDat_[globalIdx].wasSwitched)
                xnMax *= 1.02;

            // if the sum of the hypothetical mole fraction would be larger than
            // 100%, NAPL phase appears
            if (xnn > xnMax)
            {
                // NAPL phase appears
                std::cout << "NAPL phase appears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", xnn: "
                          << xnn << std::endl;
                nonwettingFlag = 1;
            }
            // calculate fractions of the hypothetical water phase
            Scalar xww = volVars.fluidState().moleFraction(wPhaseIdx, wCompIdx);
            /*
              take care:, xww, if no water is present, then take xww=xwg*pg/pwsat .
              If this is larger than 1, then water appears
            */
            Scalar xwMax = 1.0;
            if (xww > xwMax)
                wouldSwitch = true;
            if (staticVertexDat_[globalIdx].wasSwitched)
                xwMax *= 1.02;

            // if the sum of the mole fractions would be larger than
            // 100%, gas phase appears
            if (xww > xwMax)
            {
                // water phase appears
                std::cout << "water phase appears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", xww=xwg*pg/pwsat : "
                          << xww << std::endl;
                wettingFlag = 1;
            }
            if ((wettingFlag == 1) && (nonwettingFlag == 0))
            {
                newPhasePresence = wgPhaseOnly;
                globalSol[globalIdx][switch1Idx] = 0.0001;
                globalSol[globalIdx][switch2Idx]
                    = volVars.fluidState().moleFraction(gPhaseIdx, nCompIdx);
            }
            else if ((wettingFlag == 1) && (nonwettingFlag == 1))
            {
                newPhasePresence = threePhases;
                globalSol[globalIdx][switch1Idx] = 0.0001;
                globalSol[globalIdx][switch2Idx] = 0.0001;
            }
            else if ((wettingFlag == 0) && (nonwettingFlag == 1))
            {
                newPhasePresence = gnPhaseOnly;
                globalSol[globalIdx][switch1Idx]
                    = volVars.fluidState().moleFraction(gPhaseIdx, wCompIdx);
                globalSol[globalIdx][switch2Idx] = 0.0001;
            }
        }
        else if (phasePresence == wgPhaseOnly)
        {
            bool nonwettingFlag = 0;
            bool gasFlag = 0;
            bool wettingFlag = 0;

            // get the fractions in the hypothetical NAPL phase
            Scalar xnn = volVars.fluidState().moleFraction(nPhaseIdx, nCompIdx);

            // take care: if the NAPL phase is not present, take
            // xnn=xng*pg/pcsat if this is larger than 1, then NAPL
            // appears
            Scalar xnMax = 1.0;
            if (xnn > xnMax)
                wouldSwitch = true;
            if (staticVertexDat_[globalIdx].wasSwitched)
                xnMax *= 1.02;

            // if the sum of the hypothetical mole fraction would be larger than
            // 100%, NAPL phase appears
            if (xnn > xnMax)
            {
                // NAPL phase appears
                std::cout << "NAPL phase appears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", xnn: "
                          << xnn << std::endl;
                nonwettingFlag = 1;
            }

            Scalar Smin = -1.e-6;
            if (staticVertexDat_[globalIdx].wasSwitched)
                Smin = -0.01;

            if (volVars.saturation(gPhaseIdx) <= Smin)
            {
                wouldSwitch = true;
                // gas phase disappears
                std::cout << "Gas phase disappears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", Sg: "
                          << volVars.saturation(gPhaseIdx) << std::endl;
                gasFlag = 1;
            }

            Smin = 0.0;
            if (staticVertexDat_[globalIdx].wasSwitched)
                Smin = -0.01;

            if (volVars.saturation(wPhaseIdx) <= Smin)
            {
                wouldSwitch = true;
                // gas phase disappears
                std::cout << "Water phase disappears at vertex " << globalIdx
                          << ", coordinates: " << globalPos << ", Sw: "
                          << volVars.saturation(wPhaseIdx) << std::endl;
                wettingFlag = 1;
            }

            if ((gasFlag == 0) && (nonwettingFlag == 1) && (wettingFlag == 1))
            {
                newPhasePresence = gnPhaseOnly;
                globalSol[globalIdx][switch1Idx]
                    = volVars.fluidState().moleFraction(gPhaseIdx, wCompIdx);
                globalSol[globalIdx][switch2Idx] = 0.0001;
            }
            else if ((gasFlag == 0) && (nonwettingFlag == 1) && (wettingFlag == 0))
            {
                newPhasePresence = threePhases;
                globalSol[globalIdx][switch1Idx] = volVars.saturation(wPhaseIdx);
                globalSol[globalIdx][switch2Idx] = 0.0;
            }
            else if ((gasFlag == 1) && (nonwettingFlag == 0) && (wettingFlag == 0))
            {
                newPhasePresence = wPhaseOnly;
                globalSol[globalIdx][switch1Idx]
                    = volVars.fluidState().moleFraction(wPhaseIdx, gCompIdx);
                globalSol[globalIdx][switch2Idx]
                    = volVars.fluidState().moleFraction(wPhaseIdx, nCompIdx);
            }
            else if ((gasFlag == 0) && (nonwettingFlag == 0) && (wettingFlag == 1))
            {
                newPhasePresence = gPhaseOnly;
                globalSol[globalIdx][switch1Idx]
                    = volVars.fluidState().moleFraction(gPhaseIdx, wCompIdx);
                globalSol[globalIdx][switch2Idx]
                    = volVars.fluidState().moleFraction(gPhaseIdx, nCompIdx);
            }
        }

        staticVertexDat_[globalIdx].phasePresence = newPhasePresence;
        staticVertexDat_[globalIdx].wasSwitched = wouldSwitch;
        return phasePresence != newPhasePresence;
    }

    // parameters given in constructor
    std::vector<StaticVars> staticVertexDat_;
    bool switchFlag_;
};

}

#include "3p3cpropertydefaults.hh"

#endif
