// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008,2009 by Klaus Mosthaf,                               *
 *                              Andreas Lauser,                              *
 *                              Bernd Flemisch                               *
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
 * \brief Contains the quantities which are constant within a
 *        finite volume in the two-phase, two-component model.
 */
#ifndef DUMUX_2P2C_VOLUME_VARIABLES_HH
#define DUMUX_2P2C_VOLUME_VARIABLES_HH

#include <dumux/boxmodels/common/boxmodel.hh>
#include <dumux/common/math.hh>

#include <dune/common/collectivecommunication.hh>
#include <vector>
#include <iostream>

#include "2p2cproperties.hh"
#include "2p2cindices.hh"

#include <dumux/material/fluidstates/compositionalfluidstate.hh>
#include <dumux/material/constraintsolvers/computefromreferencephase.hh>
#include <dumux/material/constraintsolvers/misciblemultiphasecomposition.hh>

namespace Dumux
{

/*!
 * \ingroup TwoPTwoCModel
 * \ingroup BoxVolumeVariables
 * \brief Contains the quantities which are are constant within a
 *        finite volume in the two-phase, two-component model.
 */
template <class TypeTag>
class TwoPTwoCVolumeVariables : public BoxVolumeVariables<TypeTag>
{
    typedef BoxVolumeVariables<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) Implementation;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, MaterialLaw) MaterialLaw;
    typedef typename GET_PROP_TYPE(TypeTag, MaterialLawParams) MaterialLawParams;
    enum {
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases),
        numComponents = GET_PROP_VALUE(TypeTag, NumComponents)
    };

    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum {
        wCompIdx = Indices::lCompIdx,
        nCompIdx = Indices::gCompIdx,
        wPhaseIdx = Indices::lPhaseIdx,
        nPhaseIdx = Indices::gPhaseIdx
    };

    // present phases
    enum {
        wPhaseOnly = Indices::lPhaseOnly,
        nPhaseOnly = Indices::gPhaseOnly,
        bothPhases = Indices::bothPhases
    };

    // formulations
    enum {
        formulation = GET_PROP_VALUE(TypeTag, Formulation),
        pwSn = TwoPTwoCFormulation::plSg,
        pnSw = TwoPTwoCFormulation::pgSl
    };

    // primary variable indices
    enum {
        switchIdx = Indices::switchIdx,
        pressureIdx = Indices::pressureIdx
    };

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::template Codim<0>::Entity Element;
    enum { dim = GridView::dimension};

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef Dumux::MiscibleMultiPhaseComposition<Scalar, FluidSystem> MiscibleMultiPhaseComposition;
    typedef Dumux::ComputeFromReferencePhase<Scalar, FluidSystem> ComputeFromReferencePhase;

public:
    //! The type of the object returned by the fluidState() method
    typedef Dumux::CompositionalFluidState<Scalar, FluidSystem> FluidState;

    /*!
     * \brief Update all quantities for a given control volume.
     *
     * \param priVars The primary variables
     * \param problem The problem
     * \param element The element
     * \param fvGeometry The finite-volume geometry in the box scheme
     * \param scvIdx The local index of the SCV (sub-control volume)
     * \param isOldSol Evaluate function with solution of current or previous time step
     */
    void update(const PrimaryVariables &priVars,
                const Problem &problem,
                const Element &element,
                const FVElementGeometry &fvGeometry,
                const int scvIdx,
                const bool isOldSol)
    {
        ParentType::update(priVars,
                           problem,
                           element,
                           fvGeometry,
                           scvIdx,
                           isOldSol);

        completeFluidState(priVars, problem, element, fvGeometry, scvIdx, fluidState_, isOldSol);

        /////////////
        // calculate the remaining quantities
        /////////////
        const MaterialLawParams &materialParams =
            problem.spatialParams().materialLawParams(element, fvGeometry, scvIdx);

        // Second instance of a parameter cache.
        // Could be avoided if diffusion coefficients also
        // became part of the fluid state.
        typename FluidSystem::ParameterCache paramCache;
        paramCache.updateAll(fluidState_);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            // relative permeabilities
            Scalar kr;
            if (phaseIdx == wPhaseIdx)
                kr = MaterialLaw::krw(materialParams, saturation(wPhaseIdx));
            else // ATTENTION: krn requires the wetting phase saturation
                // as parameter!
                kr = MaterialLaw::krn(materialParams, saturation(wPhaseIdx));
            relativePermeability_[phaseIdx] = kr;
            Valgrind::CheckDefined(relativePermeability_[phaseIdx]);

            // binary diffusion coefficents
            diffCoeff_[phaseIdx] =
                FluidSystem::binaryDiffusionCoefficient(fluidState_,
                                                        paramCache,
                                                        phaseIdx,
                                                        wCompIdx,
                                                        nCompIdx);
            Valgrind::CheckDefined(diffCoeff_[phaseIdx]);
        }

        // porosity
        porosity_ = problem.spatialParams().porosity(element,
                                                         fvGeometry,
                                                         scvIdx);
        Valgrind::CheckDefined(porosity_);

        // energy related quantities not contained in the fluid state
        asImp_().updateEnergy_(priVars, problem, element, fvGeometry, scvIdx, isOldSol);
    }

    /*!
     * \copydoc BoxModel::completeFluidState
     * \param isOldSol Specifies whether this is the previous solution or the current one
     */
    static void completeFluidState(const PrimaryVariables& primaryVariables,
                                   const Problem& problem,
                                   const Element& element,
                                   const FVElementGeometry& fvGeometry,
                                   int scvIdx,
                                   FluidState& fluidState,
                                   bool isOldSol = false)
    {
        Scalar t = Implementation::temperature_(primaryVariables, problem, element,
                                                fvGeometry, scvIdx);
        fluidState.setTemperature(t);

        int globalVertIdx = problem.model().dofMapper().map(element, scvIdx, dim);
        int phasePresence = problem.model().phasePresence(globalVertIdx, isOldSol);

        /////////////
        // set the saturations
        /////////////
        Scalar Sg;
        if (phasePresence == nPhaseOnly)
            Sg = 1.0;
        else if (phasePresence == wPhaseOnly) {
            Sg = 0.0;
        }
        else if (phasePresence == bothPhases) {
            if (formulation == pwSn)
                Sg = primaryVariables[switchIdx];
            else if (formulation == pnSw)
                Sg = 1.0 - primaryVariables[switchIdx];
            else DUNE_THROW(Dune::InvalidStateException, "Formulation: " << formulation << " is invalid.");
        }
        else DUNE_THROW(Dune::InvalidStateException, "phasePresence: " << phasePresence << " is invalid.");
        fluidState.setSaturation(wPhaseIdx, 1 - Sg);
        fluidState.setSaturation(nPhaseIdx, Sg);

        /////////////
        // set the pressures of the fluid phases
        /////////////

        // calculate capillary pressure
        const MaterialLawParams &materialParams =
            problem.spatialParams().materialLawParams(element, fvGeometry, scvIdx);
        Scalar pC = MaterialLaw::pC(materialParams, 1 - Sg);

        if (formulation == pwSn) {
            fluidState.setPressure(wPhaseIdx, primaryVariables[pressureIdx]);
            fluidState.setPressure(nPhaseIdx, primaryVariables[pressureIdx] + pC);
        }
        else if (formulation == pnSw) {
            fluidState.setPressure(nPhaseIdx, primaryVariables[pressureIdx]);
            fluidState.setPressure(wPhaseIdx, primaryVariables[pressureIdx] - pC);
        }
        else DUNE_THROW(Dune::InvalidStateException, "Formulation: " << formulation << " is invalid.");

        /////////////
        // calculate the phase compositions
        /////////////
        typename FluidSystem::ParameterCache paramCache;

        // now comes the tricky part: calculate phase compositions
        if (phasePresence == bothPhases) {
            // both phases are present, phase compositions are a
            // result of the the nonwetting <-> wetting equilibrium. This is
            // the job of the "MiscibleMultiPhaseComposition"
            // constraint solver
            MiscibleMultiPhaseComposition::solve(fluidState,
                                                 paramCache,
                                                 /*setViscosity=*/true,
                                                 /*setInternalEnergy=*/false);

        }
        else if (phasePresence == nPhaseOnly) {
            // only the nonwetting phase is present, i.e. nonwetting phase
            // composition is stored explicitly.

            // extract _mass_ fractions in the nonwetting phase
            Scalar massFractionN[numComponents];
            massFractionN[wCompIdx] = primaryVariables[switchIdx];
            massFractionN[nCompIdx] = 1 - massFractionN[wCompIdx];

            // calculate average molar mass of the nonwetting phase
            Scalar M1 = FluidSystem::molarMass(wCompIdx);
            Scalar M2 = FluidSystem::molarMass(nCompIdx);
            Scalar X2 = massFractionN[nCompIdx];
            Scalar avgMolarMass = M1*M2/(M2 + X2*(M1 - M2));

            // convert mass to mole fractions and set the fluid state
            fluidState.setMoleFraction(nPhaseIdx, wCompIdx, massFractionN[wCompIdx]*avgMolarMass/M1);
            fluidState.setMoleFraction(nPhaseIdx, nCompIdx, massFractionN[nCompIdx]*avgMolarMass/M2);

            // calculate the composition of the remaining phases (as
            // well as the densities of all phases). this is the job
            // of the "ComputeFromReferencePhase" constraint solver
            ComputeFromReferencePhase::solve(fluidState,
                                             paramCache,
                                             nPhaseIdx,
                                             /*setViscosity=*/true,
                                             /*setInternalEnergy=*/false);
        }
        else if (phasePresence == wPhaseOnly) {
            // only the wetting phase is present, i.e. wetting phase
            // composition is stored explicitly.

            // extract _mass_ fractions in the nonwetting phase
            Scalar massFractionW[numComponents];
            massFractionW[nCompIdx] = primaryVariables[switchIdx];
            massFractionW[wCompIdx] = 1 - massFractionW[nCompIdx];

            // calculate average molar mass of the nonwetting phase
            Scalar M1 = FluidSystem::molarMass(wCompIdx);
            Scalar M2 = FluidSystem::molarMass(nCompIdx);
            Scalar X2 = massFractionW[nCompIdx];
            Scalar avgMolarMass = M1*M2/(M2 + X2*(M1 - M2));

            // convert mass to mole fractions and set the fluid state
            fluidState.setMoleFraction(wPhaseIdx, wCompIdx, massFractionW[wCompIdx]*avgMolarMass/M1);
            fluidState.setMoleFraction(wPhaseIdx, nCompIdx, massFractionW[nCompIdx]*avgMolarMass/M2);

            // calculate the composition of the remaining phases (as
            // well as the densities of all phases). this is the job
            // of the "ComputeFromReferencePhase" constraint solver
            ComputeFromReferencePhase::solve(fluidState,
                                             paramCache,
                                             wPhaseIdx,
                                             /*setViscosity=*/true,
                                             /*setInternalEnergy=*/false);
        }

        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            // compute and set the enthalpy
            Scalar h = Implementation::enthalpy_(fluidState, paramCache, phaseIdx);
            fluidState.setEnthalpy(phaseIdx, h);
        }
    }

    /*!
     * \brief Returns the phase state for the control-volume.
     */
    const FluidState &fluidState() const
    { return fluidState_; }

    /*!
     * \brief Returns the effective saturation of a given phase within
     *        the control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar saturation(int phaseIdx) const
    { return fluidState_.saturation(phaseIdx); }

    /*!
     * \brief Returns the mass density of a given phase within the
     *        control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar density(int phaseIdx) const
    { return fluidState_.density(phaseIdx); }

    /*!
     * \brief Returns the mass density of a given phase within the
     *        control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar molarDensity(int phaseIdx) const
    { return fluidState_.density(phaseIdx) / fluidState_.averageMolarMass(phaseIdx); }

    /*!
     * \brief Returns the effective pressure of a given phase within
     *        the control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar pressure(int phaseIdx) const
    { return fluidState_.pressure(phaseIdx); }

    /*!
     * \brief Returns temperature inside the sub-control volume.
     *
     * Note that we assume thermodynamic equilibrium, i.e. the
     * temperature of the rock matrix and of all fluid phases are
     * identical.
     */
    Scalar temperature() const
    { return fluidState_.temperature(/*phaseIdx=*/0); }

    /*!
     * \brief Returns the relative permeability of a given phase within
     *        the control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar relativePermeability(int phaseIdx) const
    {
        return relativePermeability_[phaseIdx];
    }

    /*!
     * \brief Returns the effective mobility of a given phase within
     *        the control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar mobility(int phaseIdx) const
    {
        return relativePermeability_[phaseIdx]/fluidState_.viscosity(phaseIdx);
    }

    /*!
     * \brief Returns the effective capillary pressure within the control volume.
     */
    Scalar capillaryPressure() const
    { return fluidState_.pressure(nPhaseIdx) - fluidState_.pressure(wPhaseIdx); }

    /*!
     * \brief Returns the average porosity within the control volume.
     */
    Scalar porosity() const
    { return porosity_; }

    /*!
     * \brief Returns the binary diffusion coefficients for a phase
     */
    Scalar diffCoeff(int phaseIdx) const
    { return diffCoeff_[phaseIdx]; }


protected:
    static Scalar temperature_(const PrimaryVariables &priVars,
                            const Problem& problem,
                            const Element &element,
                            const FVElementGeometry &fvGeometry,
                            int scvIdx)
    {
        return problem.boxTemperature(element, fvGeometry, scvIdx);
    }

    template<class ParameterCache>
    static Scalar enthalpy_(const FluidState& fluidState,
                            const ParameterCache& paramCache,
                            int phaseIdx)
    {
        return 0;
    }

    /*!
     * \brief Called by update() to compute the energy related quantities
     */
    void updateEnergy_(const PrimaryVariables &sol,
                       const Problem &problem,
                       const Element &element,
                       const FVElementGeometry &fvGeometry,
                       int vertIdx,
                       bool isOldSol)
    { }

    Scalar porosity_;        //!< Effective porosity within the control volume
    Scalar relativePermeability_[numPhases];  //!< Relative permeability within the control volume
    Scalar diffCoeff_[numPhases]; //!< Binary diffusion coefficients for the phases
    FluidState fluidState_;

private:
    Implementation &asImp_()
    { return *static_cast<Implementation*>(this); }

    const Implementation &asImp_() const
    { return *static_cast<const Implementation*>(this); }


};

} // end namepace

#endif
