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
 * \brief Contains the quantities which are constant within a
 *        finite volume in the three-phase three-component model.
 */
#ifndef DUMUX_3P3C_VOLUME_VARIABLES_HH
#define DUMUX_3P3C_VOLUME_VARIABLES_HH

#include <dumux/common/properties.hh>
#include <dumux/material/constants.hh>
#include <dumux/material/fluidstates/compositional.hh>
#include <dumux/material/constraintsolvers/computefromreferencephase.hh>
#include <dumux/material/constraintsolvers/misciblemultiphasecomposition.hh>
#include <dumux/porousmediumflow/volumevariables.hh>
#include <dumux/discretization/methods.hh>

namespace Dumux
{

/*!
 * \ingroup ThreePThreeCModel
 * \brief Contains the quantities which are are constant within a
 *        finite volume in the three-phase three-component model.
 */
template <class TypeTag>
class ThreePThreeCVolumeVariables : public PorousMediumFlowVolumeVariables<TypeTag>
{
    using ParentType = PorousMediumFlowVolumeVariables<TypeTag>;

    using Implementation = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using ElementSolutionVector = typename GET_PROP_TYPE(TypeTag, ElementSolutionVector);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
    using MaterialLaw = typename GET_PROP_TYPE(TypeTag, MaterialLaw);
    using MaterialLawParams = typename MaterialLaw::Params;

    // constraint solvers
    using SpatialParams = typename GET_PROP_TYPE(TypeTag, SpatialParams);
    using PermeabilityType = typename SpatialParams::PermeabilityType;
    using MiscibleMultiPhaseComposition = Dumux::MiscibleMultiPhaseComposition<Scalar, FluidSystem>;
    using ComputeFromReferencePhase = Dumux::ComputeFromReferencePhase<Scalar, FluidSystem>;
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    enum {
        dim = GridView::dimension,

        numPhases = GET_PROP_VALUE(TypeTag, NumPhases),
        numComponents = GET_PROP_VALUE(TypeTag, NumComponents),

        wCompIdx = Indices::wCompIdx,
        gCompIdx = Indices::gCompIdx,
        nCompIdx = Indices::nCompIdx,

        wPhaseIdx = Indices::wPhaseIdx,
        gPhaseIdx = Indices::gPhaseIdx,
        nPhaseIdx = Indices::nPhaseIdx,

        switch1Idx = Indices::switch1Idx,
        switch2Idx = Indices::switch2Idx,
        pressureIdx = Indices::pressureIdx
    };

    // present phases
    enum {
        threePhases = Indices::threePhases,
        wPhaseOnly  = Indices::wPhaseOnly,
        gnPhaseOnly = Indices::gnPhaseOnly,
        wnPhaseOnly = Indices::wnPhaseOnly,
        gPhaseOnly  = Indices::gPhaseOnly,
        wgPhaseOnly = Indices::wgPhaseOnly
    };

    using Element = typename GridView::template Codim<0>::Entity;

    // universial gas constant
    static constexpr Scalar R = Dumux::Constants<Scalar>::R;

public:

   using FluidState = typename GET_PROP_TYPE(TypeTag, FluidState);

    /*!
     * \copydoc ImplicitVolumeVariables::update
     */
    void update(const ElementSolutionVector &elemSol,
                const Problem &problem,
                const Element &element,
                const SubControlVolume& scv)
    {
        ParentType::update(elemSol, problem, element, scv);
        const auto& priVars = ParentType::extractDofPriVars(elemSol, scv);
        const auto phasePresence = priVars.state();

        bool useConstraintSolver = GET_PROP_VALUE(TypeTag, UseConstraintSolver);

        // capillary pressure parameters
        const MaterialLawParams &materialParams =
            problem.spatialParams().materialLawParams(element, scv, elemSol);


        Scalar temp = ParentType::temperature(elemSol, problem, element, scv);
        fluidState_.setTemperature(temp);

        /* first the saturations */
        if (phasePresence == threePhases)
        {
            sw_ = priVars[switch1Idx];
            sn_ = priVars[switch2Idx];
            sg_ = 1. - sw_ - sn_;
        }
        else if (phasePresence == wPhaseOnly)
        {
            sw_ = 1.;
            sn_ = 0.;
            sg_ = 0.;
        }
        else if (phasePresence == gnPhaseOnly)
        {
            sw_ = 0.;
            sn_ = priVars[switch2Idx];
            sg_ = 1. - sn_;
        }
        else if (phasePresence == wnPhaseOnly)
        {
            sn_ = priVars[switch2Idx];
            sw_ = 1. - sn_;
            sg_ = 0.;
        }
        else if (phasePresence == gPhaseOnly)
        {
            sw_ = 0.;
            sn_ = 0.;
            sg_ = 1.;
        }
        else if (phasePresence == wgPhaseOnly)
        {
            sw_ = priVars[switch1Idx];
            sn_ = 0.;
            sg_ = 1. - sw_;
        }
        else
            DUNE_THROW(Dune::InvalidStateException, "phasePresence: " << phasePresence << " is invalid.");
        Valgrind::CheckDefined(sg_);

        fluidState_.setSaturation(wPhaseIdx, sw_);
        fluidState_.setSaturation(gPhaseIdx, sg_);
        fluidState_.setSaturation(nPhaseIdx, sn_);

        /* now the pressures */
        pg_ = priVars[pressureIdx];

        // calculate capillary pressures
        Scalar pcgw = MaterialLaw::pcgw(materialParams, sw_);
        Scalar pcnw = MaterialLaw::pcnw(materialParams, sw_);
        Scalar pcgn = MaterialLaw::pcgn(materialParams, sw_ + sn_);

        Scalar pcAlpha = MaterialLaw::pcAlpha(materialParams, sn_);
        Scalar pcNW1 = 0.0; // TODO: this should be possible to assign in the problem file

        pn_ = pg_- pcAlpha * pcgn - (1.-pcAlpha)*(pcgw - pcNW1);
        pw_ = pn_ - pcAlpha * pcnw - (1.-pcAlpha)*pcNW1;

        fluidState_.setPressure(wPhaseIdx, pw_);
        fluidState_.setPressure(gPhaseIdx, pg_);
        fluidState_.setPressure(nPhaseIdx, pn_);

        // calculate and set all fugacity coefficients. this is
        // possible because we require all phases to be an ideal
        // mixture, i.e. fugacity coefficients are not supposed to
        // depend on composition!
        typename FluidSystem::ParameterCache paramCache;
        // assert(FluidSystem::isIdealGas(gPhaseIdx));
        for (int phaseIdx = 0; phaseIdx < numPhases; ++ phaseIdx) {
            assert(FluidSystem::isIdealMixture(phaseIdx));

            for (int compIdx = 0; compIdx < numComponents; ++ compIdx) {
                Scalar phi = FluidSystem::fugacityCoefficient(fluidState_, paramCache, phaseIdx, compIdx);
                fluidState_.setFugacityCoefficient(phaseIdx, compIdx, phi);
            }
        }

        // now comes the tricky part: calculate phase composition
        if (phasePresence == threePhases) {
            // all phases are present, phase compositions are a
            // result of the the gas <-> liquid equilibrium. This is
            // the job of the "MiscibleMultiPhaseComposition"
            // constraint solver ...
            if (useConstraintSolver) {
                MiscibleMultiPhaseComposition::solve(fluidState_,
                                                     paramCache,
                                                     /*setViscosity=*/true,
                                                     /*setEnthalpy=*/false);
            }
            // ... or calculated explicitly this way ...
            // please note that we experienced some problems with un-regularized
            // partial pressures due to their calculation from fugacity coefficients -
            // that's why they are regularized below "within physically meaningful bounds"
            else {
                Scalar partPressH2O = FluidSystem::fugacityCoefficient(fluidState_,
                                                                      wPhaseIdx,
                                                                      wCompIdx) * pw_;
                if (partPressH2O > pg_) partPressH2O = pg_;
                Scalar partPressNAPL = FluidSystem::fugacityCoefficient(fluidState_,
                                                                       nPhaseIdx,
                                                                       nCompIdx) * pn_;
                if (partPressNAPL > pg_) partPressNAPL = pg_;
                Scalar partPressAir = pg_ - partPressH2O - partPressNAPL;

                Scalar xgn = partPressNAPL/pg_;
                Scalar xgw = partPressH2O/pg_;
                Scalar xgg = partPressAir/pg_;

                // actually, it's nothing else than Henry coefficient
                Scalar xwn = partPressNAPL
                             / (FluidSystem::fugacityCoefficient(fluidState_,
                                                                 wPhaseIdx,nCompIdx)
                                * pw_);
                Scalar xwg = partPressAir
                             / (FluidSystem::fugacityCoefficient(fluidState_,
                                                                 wPhaseIdx,gCompIdx)
                                * pw_);
                Scalar xww = 1.-xwg-xwn;

                Scalar xnn = 1.-2.e-10;
                Scalar xna = 1.e-10;
                Scalar xnw = 1.e-10;

                fluidState_.setMoleFraction(wPhaseIdx, wCompIdx, xww);
                fluidState_.setMoleFraction(wPhaseIdx, gCompIdx, xwg);
                fluidState_.setMoleFraction(wPhaseIdx, nCompIdx, xwn);
                fluidState_.setMoleFraction(gPhaseIdx, wCompIdx, xgw);
                fluidState_.setMoleFraction(gPhaseIdx, gCompIdx, xgg);
                fluidState_.setMoleFraction(gPhaseIdx, nCompIdx, xgn);
                fluidState_.setMoleFraction(nPhaseIdx, wCompIdx, xnw);
                fluidState_.setMoleFraction(nPhaseIdx, gCompIdx, xna);
                fluidState_.setMoleFraction(nPhaseIdx, nCompIdx, xnn);

                Scalar rhoW = FluidSystem::density(fluidState_, wPhaseIdx);
                Scalar rhoG = FluidSystem::density(fluidState_, gPhaseIdx);
                Scalar rhoN = FluidSystem::density(fluidState_, nPhaseIdx);

                fluidState_.setDensity(wPhaseIdx, rhoW);
                fluidState_.setDensity(gPhaseIdx, rhoG);
                fluidState_.setDensity(nPhaseIdx, rhoN);
            }
        }
        else if (phasePresence == wPhaseOnly) {
            // only the water phase is present, water phase composition is
            // stored explicitly.

            // extract mole fractions in the water phase
            Scalar xwg = priVars[switch1Idx];
            Scalar xwn = priVars[switch2Idx];
            Scalar xww = 1 - xwg - xwn;

            // write water mole fractions in the fluid state
            fluidState_.setMoleFraction(wPhaseIdx, wCompIdx, xww);
            fluidState_.setMoleFraction(wPhaseIdx, gCompIdx, xwg);
            fluidState_.setMoleFraction(wPhaseIdx, nCompIdx, xwn);

            // calculate the composition of the remaining phases (as
            // well as the densities of all phases). this is the job
            // of the "ComputeFromReferencePhase" constraint solver ...
            if (useConstraintSolver)
            {
                ComputeFromReferencePhase::solve(fluidState_,
                                                 paramCache,
                                                 wPhaseIdx,
                                                 /*setViscosity=*/true,
                                                 /*setEnthalpy=*/false);
            }
            // ... or calculated explicitly this way ...
            else {
                // note that the gas phase is actually not existing!
                // thus, this is used as phase switch criterion
                Scalar xgg = xwg * FluidSystem::fugacityCoefficient(fluidState_,
                                                                    wPhaseIdx,gCompIdx)
                                   * pw_ / pg_;
                Scalar xgn = xwn * FluidSystem::fugacityCoefficient(fluidState_,
                                                                    wPhaseIdx,nCompIdx)
                                   * pw_ / pg_;
                Scalar xgw = FluidSystem::fugacityCoefficient(fluidState_,
                                                              wPhaseIdx,wCompIdx)
                                   * pw_ / pg_;


                // note that the gas phase is actually not existing!
                // thus, this is used as phase switch criterion
                Scalar xnn = xwn * FluidSystem::fugacityCoefficient(fluidState_,
                                                                    wPhaseIdx,nCompIdx)
                                   * pw_;
                Scalar xna = 1.e-10;
                Scalar xnw = 1.e-10;

                fluidState_.setMoleFraction(gPhaseIdx, wCompIdx, xgw);
                fluidState_.setMoleFraction(gPhaseIdx, gCompIdx, xgg);
                fluidState_.setMoleFraction(gPhaseIdx, nCompIdx, xgn);
                fluidState_.setMoleFraction(nPhaseIdx, wCompIdx, xnw);
                fluidState_.setMoleFraction(nPhaseIdx, gCompIdx, xna);
                fluidState_.setMoleFraction(nPhaseIdx, nCompIdx, xnn);

                Scalar rhoW = FluidSystem::density(fluidState_, wPhaseIdx);
                Scalar rhoG = FluidSystem::density(fluidState_, gPhaseIdx);
                Scalar rhoN = FluidSystem::density(fluidState_, nPhaseIdx);

                fluidState_.setDensity(wPhaseIdx, rhoW);
                fluidState_.setDensity(gPhaseIdx, rhoG);
                fluidState_.setDensity(nPhaseIdx, rhoN);
            }
        }
        else if (phasePresence == gnPhaseOnly) {
            // only gas and NAPL phases are present
            // we have all (partly hypothetical) phase pressures
            // and temperature and the mole fraction of water in
            // the gas phase

            // we have all (partly hypothetical) phase pressures
            // and temperature and the mole fraction of water in
            // the gas phase
            Scalar partPressNAPL = fluidState_.fugacityCoefficient(nPhaseIdx, nCompIdx)*pn_;
            if (partPressNAPL > pg_) partPressNAPL = pg_;

            Scalar xgw = priVars[switch1Idx];
            Scalar xgn = partPressNAPL/pg_;
            Scalar xgg = 1.-xgw-xgn;

            // write mole fractions in the fluid state
            fluidState_.setMoleFraction(gPhaseIdx, wCompIdx, xgw);
            fluidState_.setMoleFraction(gPhaseIdx, gCompIdx, xgg);
            fluidState_.setMoleFraction(gPhaseIdx, nCompIdx, xgn);

            // calculate the composition of the remaining phases (as
            // well as the densities of all phases). this is the job
            // of the "ComputeFromReferencePhase" constraint solver
            ComputeFromReferencePhase::solve(fluidState_,
                                             paramCache,
                                             gPhaseIdx,
                                             /*setViscosity=*/true,
                                             /*setEnthalpy=*/false);
        }
        else if (phasePresence == wnPhaseOnly) {
            // only water and NAPL phases are present
            Scalar partPressNAPL = fluidState_.fugacityCoefficient(nPhaseIdx,nCompIdx)*pn_;
            if (partPressNAPL > pg_) partPressNAPL = pg_;
            Scalar henryC = fluidState_.fugacityCoefficient(wPhaseIdx,nCompIdx)*pw_;

            Scalar xwg = priVars[switch1Idx];
            Scalar xwn = partPressNAPL/henryC;
            Scalar xww = 1.-xwg-xwn;

            // write mole fractions in the fluid state
            fluidState_.setMoleFraction(wPhaseIdx, wCompIdx, xww);
            fluidState_.setMoleFraction(wPhaseIdx, gCompIdx, xwg);
            fluidState_.setMoleFraction(wPhaseIdx, nCompIdx, xwn);

            // calculate the composition of the remaining phases (as
            // well as the densities of all phases). this is the job
            // of the "ComputeFromReferencePhase" constraint solver
            ComputeFromReferencePhase::solve(fluidState_,
                                             paramCache,
                                             wPhaseIdx,
                                             /*setViscosity=*/true,
                                             /*setEnthalpy=*/false);
        }
        else if (phasePresence == gPhaseOnly) {
            // only the gas phase is present, gas phase composition is
            // stored explicitly here below.

            const Scalar xgw = priVars[switch1Idx];
            const Scalar xgn = priVars[switch2Idx];
            Scalar xgg = 1 - xgw - xgn;

            // write mole fractions in the fluid state
            fluidState_.setMoleFraction(gPhaseIdx, wCompIdx, xgw);
            fluidState_.setMoleFraction(gPhaseIdx, gCompIdx, xgg);
            fluidState_.setMoleFraction(gPhaseIdx, nCompIdx, xgn);

            // calculate the composition of the remaining phases (as
            // well as the densities of all phases). this is the job
            // of the "ComputeFromReferencePhase" constraint solver ...
            if (useConstraintSolver)
            {
                ComputeFromReferencePhase::solve(fluidState_,
                                                 paramCache,
                                                 gPhaseIdx,
                                                 /*setViscosity=*/true,
                                                 /*setEnthalpy=*/false);
            }
            // ... or calculated explicitly this way ...
            else {

                // note that the water phase is actually not existing!
                // thus, this is used as phase switch criterion
                Scalar xww = xgw * pg_
                             / (FluidSystem::fugacityCoefficient(fluidState_,
                                                                 wPhaseIdx,wCompIdx)
                                * pw_);
                Scalar xwn = 1.e-10;
                Scalar xwg = 1.e-10;

                // note that the NAPL phase is actually not existing!
                // thus, this is used as phase switch criterion
                Scalar xnn = xgn * pg_
                             / (FluidSystem::fugacityCoefficient(fluidState_,
                                                                 nPhaseIdx,nCompIdx)
                                * pn_);
                Scalar xna = 1.e-10;
                Scalar xnw = 1.e-10;

                fluidState_.setMoleFraction(wPhaseIdx, wCompIdx, xww);
                fluidState_.setMoleFraction(wPhaseIdx, gCompIdx, xwg);
                fluidState_.setMoleFraction(wPhaseIdx, nCompIdx, xwn);
                fluidState_.setMoleFraction(nPhaseIdx, wCompIdx, xnw);
                fluidState_.setMoleFraction(nPhaseIdx, gCompIdx, xna);
                fluidState_.setMoleFraction(nPhaseIdx, nCompIdx, xnn);

                Scalar rhoW = FluidSystem::density(fluidState_, wPhaseIdx);
                Scalar rhoG = FluidSystem::density(fluidState_, gPhaseIdx);
                Scalar rhoN = FluidSystem::density(fluidState_, nPhaseIdx);

                fluidState_.setDensity(wPhaseIdx, rhoW);
                fluidState_.setDensity(gPhaseIdx, rhoG);
                fluidState_.setDensity(nPhaseIdx, rhoN);
            }
        }
        else if (phasePresence == wgPhaseOnly) {
            // only water and gas phases are present
            Scalar xgn = priVars[switch2Idx];
            Scalar partPressH2O = fluidState_.fugacityCoefficient(wPhaseIdx, wCompIdx)*pw_;
            if (partPressH2O > pg_) partPressH2O = pg_;

            Scalar xgw = partPressH2O/pg_;
            Scalar xgg = 1.-xgn-xgw;

            // write mole fractions in the fluid state
            fluidState_.setMoleFraction(gPhaseIdx, wCompIdx, xgw);
            fluidState_.setMoleFraction(gPhaseIdx, gCompIdx, xgg);
            fluidState_.setMoleFraction(gPhaseIdx, nCompIdx, xgn);

            // calculate the composition of the remaining phases (as
            // well as the densities of all phases). this is the job
            // of the "ComputeFromReferencePhase" constraint solver ...
            if (useConstraintSolver)
            {
                ComputeFromReferencePhase::solve(fluidState_,
                                                 paramCache,
                                                 gPhaseIdx,
                                                 /*setViscosity=*/true,
                                                 /*setEnthalpy=*/false);
            }
            // ... or calculated explicitly this way ...
            else {
                // actually, it's nothing else than Henry coefficient
                Scalar xwn = xgn * pg_
                             / (FluidSystem::fugacityCoefficient(fluidState_,
                                                                 wPhaseIdx,nCompIdx)
                                * pw_);
                Scalar xwg = xgg * pg_
                             / (FluidSystem::fugacityCoefficient(fluidState_,
                                                                 wPhaseIdx,gCompIdx)
                                * pw_);
                Scalar xww = 1.-xwg-xwn;

                // note that the NAPL phase is actually not existing!
                // thus, this is used as phase switch criterion
                Scalar xnn = xgn * pg_
                             / (FluidSystem::fugacityCoefficient(fluidState_,
                                                                 nPhaseIdx,nCompIdx)
                                * pn_);
                Scalar xna = 1.e-10;
                Scalar xnw = 1.e-10;

                fluidState_.setMoleFraction(wPhaseIdx, wCompIdx, xww);
                fluidState_.setMoleFraction(wPhaseIdx, gCompIdx, xwg);
                fluidState_.setMoleFraction(wPhaseIdx, nCompIdx, xwn);
                fluidState_.setMoleFraction(nPhaseIdx, wCompIdx, xnw);
                fluidState_.setMoleFraction(nPhaseIdx, gCompIdx, xna);
                fluidState_.setMoleFraction(nPhaseIdx, nCompIdx, xnn);

                Scalar rhoW = FluidSystem::density(fluidState_, wPhaseIdx);
                Scalar rhoG = FluidSystem::density(fluidState_, gPhaseIdx);
                Scalar rhoN = FluidSystem::density(fluidState_, nPhaseIdx);

                fluidState_.setDensity(wPhaseIdx, rhoW);
                fluidState_.setDensity(gPhaseIdx, rhoG);
                fluidState_.setDensity(nPhaseIdx, rhoN);
            }
        }
        else
        {
            DUNE_THROW(Dune::InvalidStateException, "phasePresence: " << phasePresence << " is invalid.");
        }

        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            // Mobilities
            const Scalar mu =
                FluidSystem::viscosity(fluidState_,
                                       paramCache,
                                       phaseIdx);
            fluidState_.setViscosity(phaseIdx,mu);

            Scalar kr;
            kr = MaterialLaw::kr(materialParams, phaseIdx,
                                 fluidState_.saturation(wPhaseIdx),
                                 fluidState_.saturation(nPhaseIdx),
                                 fluidState_.saturation(gPhaseIdx));
            mobility_[phaseIdx] = kr / mu;
            Valgrind::CheckDefined(mobility_[phaseIdx]);
        }

        // material dependent parameters for NAPL adsorption
        bulkDensTimesAdsorpCoeff_ =
            MaterialLaw::bulkDensTimesAdsorpCoeff(materialParams);

        /* compute the diffusion coefficient
         * \note This is the part of the diffusion coefficient determined by the fluid state, e.g.
         *       important if they are tabularized. In the diffusive flux computation (e.g. Fick's law)
         *       this gets converted into an effecient coefficient depending on saturation and porosity.
         *       We can then add a normalized tensorial component
         *       e.g. obtained from DTI from the spatial params (currently not implemented)
         */
        setDiffusionCoefficient_(gPhaseIdx, wCompIdx, FluidSystem::diffusionCoefficient(fluidState_, paramCache, gPhaseIdx, wCompIdx));
        setDiffusionCoefficient_(gPhaseIdx, nCompIdx, FluidSystem::diffusionCoefficient(fluidState_, paramCache, gPhaseIdx, nCompIdx));
        setDiffusionCoefficient_(wPhaseIdx, gCompIdx, FluidSystem::diffusionCoefficient(fluidState_, paramCache, wPhaseIdx, gCompIdx));
        setDiffusionCoefficient_(wPhaseIdx, nCompIdx, FluidSystem::diffusionCoefficient(fluidState_, paramCache, wPhaseIdx, nCompIdx));
        // no diffusion in NAPL phase considered  at the moment
        setDiffusionCoefficient_(nPhaseIdx, wCompIdx, 0.0);
        setDiffusionCoefficient_(nPhaseIdx, gCompIdx, 0.0);

        // porosity & permeabilty
        porosity_ = problem.spatialParams().porosity(element, scv, elemSol);
        permeability_ = problem.spatialParams().permeability(element, scv, elemSol);

        // compute and set the enthalpy
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
        {
            Scalar h = ParentType::enthalpy(fluidState_, paramCache, phaseIdx);
            fluidState_.setEnthalpy(phaseIdx, h);
        }
    }

    /*!
     * \brief Returns the phase state for the control volume.
     */
    const FluidState &fluidState() const
    { return fluidState_; }

    /*!
     * \brief Returns the effective saturation of a given phase within
     *        the control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar saturation(const int phaseIdx) const
    { return fluidState_.saturation(phaseIdx); }

    /*!
     * \brief Returns the mass fraction of a given component in a
     *        given phase within the control volume in \f$[-]\f$.
     *
     * \param phaseIdx The phase index
     * \param compIdx The component index
     */
    Scalar massFraction(const int phaseIdx, const int compIdx) const
    { return fluidState_.massFraction(phaseIdx, compIdx); }

    /*!
     * \brief Returns the mole fraction of a given component in a
     *        given phase within the control volume in \f$[-]\f$.
     *
     * \param phaseIdx The phase index
     * \param compIdx The component index
     */
    Scalar moleFraction(const int phaseIdx, const int compIdx) const
    { return fluidState_.moleFraction(phaseIdx, compIdx); }

    /*!
     * \brief Returns the mass density of a given phase within the
     *        control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar density(const int phaseIdx) const
    { return fluidState_.density(phaseIdx); }

    /*!
     * \brief Returns the molar density of a given phase within the
     *        control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar molarDensity(const int phaseIdx) const
    { return fluidState_.density(phaseIdx) / fluidState_.averageMolarMass(phaseIdx); }

    /*!
     * \brief Returns the effective pressure of a given phase within
     *        the control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar pressure(const int phaseIdx) const
    { return fluidState_.pressure(phaseIdx); }

    /*!
     * \brief Returns temperature inside the sub-control volume.
     *
     * Note that we assume thermodynamic equilibrium, i.e. the
     * temperatures of the rock matrix and of all fluid phases are
     * identical.
     */
    Scalar temperature() const
    { return fluidState_.temperature(/*phaseIdx=*/0); }

    /*!
     * \brief Returns the effective mobility of a given phase within
     *        the control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar mobility(const int phaseIdx) const
    {
        return mobility_[phaseIdx];
    }

    /*!
     * \brief Returns the effective capillary pressure within the control volume.
     */
    Scalar capillaryPressure() const
    { return fluidState_.capillaryPressure(); }

    /*!
     * \brief Returns the average porosity within the control volume.
     */
    Scalar porosity() const
    { return porosity_; }

    /*!
     * \brief Returns the adsorption information.
     */
    Scalar bulkDensTimesAdsorpCoeff() const
    { return bulkDensTimesAdsorpCoeff_; }

    /*!
     * \brief Returns the average permeability within the control volume in \f$[m^2]\f$.
     */
    const PermeabilityType& permeability() const
    { return permeability_; }

    /*!
     * \brief Returns the diffusion coeffiecient
     */
    Scalar diffusionCoefficient(int phaseIdx, int compIdx) const
    {
        if (compIdx < phaseIdx)
            return diffCoefficient_[phaseIdx][compIdx];
        else if (compIdx > phaseIdx)
            return diffCoefficient_[phaseIdx][compIdx-1];
        else
            DUNE_THROW(Dune::InvalidStateException, "Diffusion coeffiecient called for phaseIdx = compIdx");
    }

protected:

    Scalar sw_, sg_, sn_, pg_, pw_, pn_;

    Scalar moleFrac_[numPhases][numComponents];
    Scalar massFrac_[numPhases][numComponents];

    Scalar porosity_;        //!< Effective porosity within the control volume
    PermeabilityType permeability_; //!< Effective permeability within the control volume
    Scalar mobility_[numPhases];  //!< Effective mobility within the control volume
    Scalar bulkDensTimesAdsorpCoeff_; //!< the basis for calculating adsorbed NAPL
    FluidState fluidState_;

private:
    void setDiffusionCoefficient_(int phaseIdx, int compIdx, Scalar d)
    {
        if (compIdx < phaseIdx)
            diffCoefficient_[phaseIdx][compIdx] = std::move(d);
        else if (compIdx > phaseIdx)
            diffCoefficient_[phaseIdx][compIdx-1] = std::move(d);
        else if (phaseIdx == nPhaseIdx)
            diffCoefficient_[phaseIdx][compIdx-1] = 0;
        else
            DUNE_THROW(Dune::InvalidStateException, "Diffusion coeffiecient for phaseIdx = compIdx doesn't exist");
    }

    std::array<std::array<Scalar, numComponents-1>, numPhases> diffCoefficient_;

    Implementation &asImp_()
    { return *static_cast<Implementation*>(this); }

    const Implementation &asImp_() const
    { return *static_cast<const Implementation*>(this); }
};

} // end namespace Dumux

#endif