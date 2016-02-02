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
 * \brief A twophase fluid system with water and nitrogen as components.
 */
#ifndef DUMUX_H2O_N2_FLUID_SYSTEM_KINETIC_HH
#define DUMUX_H2O_N2_FLUID_SYSTEM_KINETIC_HH

#include <dumux/material/fluidsystems/h2on2.hh>
#include <dumux/material/components/simpleh2o.hh>
#include <dumux/material/components/h2o.hh>
#include <dumux/material/components/n2.hh>
#include <dumux/material/components/tabulatedcomponent.hh>
#include <dumux/material/idealgas.hh>

#include <dumux/material/binarycoefficients/h2o_n2.hh>


#include <dumux/material/fluidsystems/base.hh>

namespace Dumux
{

namespace FluidSystems
{
/*!
 * \ingroup Fluidsystems
 * \brief A twophase fluid system with water and nitrogen as components.
 */
template <class Scalar, bool useComplexRelations = true>
class H2ON2Kinetic :
    public FluidSystems::H2ON2<Scalar, useComplexRelations>
{
private:
    typedef FluidSystems::H2ON2<Scalar, useComplexRelations> ParentType;

    // convenience typedefs
//    typedef typename ParentType::StaticParameters SP;
    typedef Dumux::IdealGas<Scalar> IdealGas;
public:
//    typedef typename ParentType::ParameterCache MutableParameters;
    //! The type of parameter cache objects
    typedef Dumux::NullParameterCache ParameterCache;

    //! Index of the solid phase
    static constexpr int sPhaseIdx = 2;

//    /*!
//     * \brief Initialize the fluid system's static parameters
//     */
//    static void init(Scalar tempMin, Scalar tempMax, unsigned nTemp,
//                     Scalar pressMin, Scalar pressMax, unsigned nPress)
//    {
//        ParentType::init(tempMin, tempMax, nTemp,
//                         pressMin, pressMax, nPress);
//    }

    static const char *phaseName(int phaseIdx)
    {
        if (phaseIdx == sPhaseIdx)
            return "s";

        return ParentType::phaseName(phaseIdx);
    }

    /*!
     * \brief Give the enthalpy of a *component* in a phase.
     * \param fluidState A container with the current (physical) state of the fluid
     * \param phaseIdx The index of the phase to consider
     * \param compIdx The index of the component to consider
     */
    template <class FluidState>
    static Scalar componentEnthalpy(FluidState &fluidState,
                                    const int phaseIdx,
                                    const int compIdx)
    {
        const Scalar T = fluidState.temperature(phaseIdx);
        const Scalar p = fluidState.pressure(phaseIdx);
        Valgrind::CheckDefined(T);
        Valgrind::CheckDefined(p);
        switch (phaseIdx){
            case ParentType::wPhaseIdx:
                switch(compIdx){
                case ParentType::H2OIdx:
                    return ParentType::H2O::liquidEnthalpy(T, p);
                case ParentType::N2Idx:
                    return ParentType::N2::gasEnthalpy(T, p);
                default:
                    DUNE_THROW(Dune::NotImplemented,
                               "wrong index");
                    break;
                }// end switch compIdx
                break;
            case ParentType::nPhaseIdx:
                switch(compIdx){
                case ParentType::H2OIdx:
                    return ParentType::H2O::gasEnthalpy(T, p);
                case ParentType::N2Idx:
                    return ParentType::N2::gasEnthalpy(T, p);
                default:
                    DUNE_THROW(Dune::NotImplemented,
                               "wrong index");
                    break;
                }// end switch compIdx
                break;
            default:
                DUNE_THROW(Dune::NotImplemented,
                           "wrong index");
                break;
        }// end switch phaseIdx
    }


    /*!
     * \brief Returns the equilibrium mole fraction of a component in the other phase.
     *
     *        I.e. this is a (2-phase) solution of the problem:  I know the composition of a
     *        phase and want to know the composition of the other phase.
     *
     *        \param fluidState A container with the current (physical) state of the fluid
     *        \param paramCache A container for iterative calculation of fluid composition
     *        \param referencePhaseIdx The index of the phase for which composition is known.
     *        \param calcCompIdx The component for which the composition in the other phase is to be
     *               calculated.
     */
    template <class FluidState>
    static void calculateEquilibriumMoleFractionOtherPhase(FluidState & fluidState,
                                                    const ParameterCache & paramCache,
                                                    const unsigned int referencePhaseIdx,
                                                    const unsigned int calcCompIdx)
    {
        const unsigned int nPhaseIdx   = ParentType::nPhaseIdx;
        const unsigned int wPhaseIdx   = ParentType::wPhaseIdx;
        const unsigned int nCompIdx    = ParentType::nCompIdx;
        const unsigned int wCompIdx    = ParentType::wCompIdx;

        assert(0 <= referencePhaseIdx and referencePhaseIdx < ParentType::numPhases);
        assert(0 <= calcCompIdx and calcCompIdx < ParentType::numComponents);

        const unsigned int numPhases    = ParentType::numPhases;
        const unsigned int numComponents= ParentType::numComponents;
        static_assert(( (numComponents==numPhases)  and (numPhases==2) ),
                      "This function requires that the number of fluid phases is equal "
                      "to the number of components");

        const Scalar temperature = fluidState.temperature(/*phaseIdx=*/0);

        // the index of the other phase
        // for the 2-phase case, this is easy: nPhase in, wPhase set   ; wPhase in, nPhase set
        const unsigned int otherPhaseIdx = referencePhaseIdx==wPhaseIdx ? nPhaseIdx : wPhaseIdx ;

        // idea: - the mole fraction of a component in a phase is known.
        //       - by means of functional relations, the mole fraction of the
        //         same component in the other phase can be calculated.

        // therefore in: phaseIdx, compIdx, out: mole fraction of compIdx in otherPhaseIdx

        const Scalar pn     = fluidState.pressure(nPhaseIdx);

        // the switch is based on what is known:
        // the mole fraction of one component in one phase
        switch (referencePhaseIdx)
        {
        case wPhaseIdx :
            switch (calcCompIdx)
            {
            case wCompIdx :
            {
                // wPhase, wComp comes in: we hand back the concentration in the other phase: nPhase, wComp
                const Scalar pv  = ParentType::H2O::vaporPressure(temperature) ;
                const Scalar xww = fluidState.moleFraction(referencePhaseIdx, calcCompIdx) ; // known from reference phase
                const Scalar xnw = pv / pn * xww ; // mole fraction in the other phase
                Valgrind::CheckDefined(xnw);
                fluidState.setMoleFraction(otherPhaseIdx, calcCompIdx, xnw) ;
                return;
            }

            case nCompIdx :
            {
                // wPhase, nComp comes in: we hand back the concentration in the other phase: nPhase, nComp
                const Scalar H      = Dumux::BinaryCoeff::H2O_N2::henry(temperature) ; // Pa
                const Scalar xwn    = fluidState.moleFraction(referencePhaseIdx, calcCompIdx) ; // known from reference phase
                const Scalar xnn    = H / pn * xwn; // mole fraction in the other phase
                Valgrind::CheckDefined(xnn);
                fluidState.setMoleFraction(otherPhaseIdx, calcCompIdx, xnn) ;
                return;
            }

            default: DUNE_THROW(Dune::NotImplemented, "wrong index");
            break;
            }
            break;

        case nPhaseIdx :
            switch (calcCompIdx)
            {
            case wCompIdx :
            {
                // nPhase, wComp comes in: we hand back the concentration in the other phase: wPhase, wComp
                const Scalar pv  = ParentType::H2O::vaporPressure(temperature) ;
                const Scalar xnw = fluidState.moleFraction(referencePhaseIdx, calcCompIdx) ;// known from reference phase
                const Scalar xww = pn / pv * xnw ; // mole fraction in the other phase
                Valgrind::CheckDefined(xww);
                fluidState.setMoleFraction(otherPhaseIdx, calcCompIdx, xww) ;
                return;
            }

            case nCompIdx :
            {
                // nPhase, nComp comes in: we hand back the concentration in the other phase: wPhase, nComp
                const Scalar H      = Dumux::BinaryCoeff::H2O_N2::henry(temperature) ; // Pa
                const Scalar xnn    = fluidState.moleFraction(referencePhaseIdx, calcCompIdx) ;// known from reference phase
                const Scalar xwn    = pn / H * xnn ; // mole fraction in the other phase
                Valgrind::CheckDefined(xwn);
                fluidState.setMoleFraction(otherPhaseIdx, calcCompIdx, xwn) ;
                return ;
            }

            default:
                DUNE_THROW(Dune::NotImplemented, "wrong index");
            }

            DUNE_THROW(Dune::NotImplemented, "wrong index");
        }
    }

    /*!
     * \brief Calculates the equilibrium composition for a given temperature and pressure.
     *
     *        In general a system of equations needs to be solved for this. In the case of
     *        a 2 component system, this can done by hand.
     *
     *        If this system was to be described with more components, and /or if a matrix is
     *        to be assembled like e.g. MiscibleMultiPhaseComposition ConstraintSolver,
     *        a function describing the chemical potentials of the components in the respective
     *        phases was needed.
     *
     *        In the case of Henry, Raoult this would be
     *
     *        ----- n-Comp w-Comp
     *
     *        nPhase pn \f$\mathrm{x_n^n}\f$ pn \f$\mathrm{x_n^w}\f$
     *
     *        wPhase pv \f$\mathrm{w_w^w}\f$ H \f$\mathrm{x_w^n}\f$
     *
     *
     *        Plus additional relations for additional components.
     *
     *        Basically the same structure of the matrix can be used, but the thing that is the
     *        same in both phases is the chemical potential, not the fugacity coefficient.
     *
     *        \param fluidState A container with the current (physical) state of the fluid
     *        \param paramCache A container for iterative calculation of fluid composition
     */
    template <class FluidState>
    static void calculateEquilibriumMoleFractions(FluidState & fluidState,
                                                  const ParameterCache & paramCache)
    {
        const unsigned int nPhaseIdx    = ParentType::nPhaseIdx;
        const unsigned int wPhaseIdx    = ParentType::wPhaseIdx;
        const unsigned int nCompIdx     = ParentType::nCompIdx;
        const unsigned int wCompIdx     = ParentType::wCompIdx;
        const unsigned int numPhases    = ParentType::numPhases;
        const unsigned int numComponents= ParentType::numComponents;

        static_assert(( (numComponents==numPhases)  and (numPhases==2) ),
                      "This function requires that the number fluid phases is equal "
                      "to the number of components");

        const Scalar temperature        = fluidState.temperature(/*phaseIdx=*/0);
        const Scalar pn                 = fluidState.pressure(nPhaseIdx);
        const Scalar satVapPressure     = ParentType::H2O::vaporPressure(temperature);
        const Scalar henry              = Dumux::BinaryCoeff::H2O_N2::henry(temperature);

        Scalar x[numPhases][numComponents] ;
        x[nPhaseIdx][wCompIdx]  = ( satVapPressure*(henry - pn) )  / ( pn*(henry-satVapPressure) ) ;
        x[nPhaseIdx][nCompIdx]  = 1. - x[nPhaseIdx][wCompIdx];
        x[wPhaseIdx][nCompIdx]  = x[nPhaseIdx][nCompIdx] * pn / henry;
        x[wPhaseIdx][wCompIdx]  = x[nPhaseIdx][wCompIdx] * pn / satVapPressure;

        for (unsigned int phaseIdx = 0; phaseIdx < numPhases; phaseIdx++)
            for (unsigned int compIdx = 0; compIdx < numComponents; compIdx++)
            {
                Valgrind::CheckDefined(x[phaseIdx][compIdx]);
                fluidState.setMoleFraction(phaseIdx, compIdx, x[phaseIdx][compIdx]) ;
            }
    }

    /*!
     * \brief Give the Henry constant for a component in a phase. \f$\mathrm{[Pa]}\f$
     * \param temperature The given temperature
     */
    static Scalar henry(Scalar temperature)
    {
        return Dumux::BinaryCoeff::H2O_N2::henry(temperature) ; // Pa
    }

    /*!
     * \brief Give the vapor pressure of a component above one phase. \f$\mathrm{[Pa]}\f$
     * \param temperature The given temperature
     */
    static Scalar vaporPressure(Scalar temperature)
    {
        return ParentType::H2O::vaporPressure(temperature); // Pa // 1e-20 ; //
    }
};
}// end namespace Fluidsystem
} // end namespace Dumux

#endif