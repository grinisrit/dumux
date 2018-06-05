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
 * \brief @copybrief Dumux::FluidSystems::SteamN2CaO2H2
 */
#ifndef DUMUX_STEAM_N2_CAO2H2_SYSTEM_HH
#define DUMUX_STEAM_N2_CAO2H2_SYSTEM_HH

#include <cassert>

#include <dune/common/exceptions.hh>

#include <dumux/common/valgrind.hh>

#include <dumux/material/idealgas.hh>
#include <dumux/material/fluidsystems/base.hh>
#include <dumux/material/components/n2.hh>
#include <dumux/material/components/h2o.hh>
#include <dumux/material/components/cao.hh>
#include <dumux/material/components/cao2h2.hh>
#include <dumux/material/binarycoefficients/h2o_n2.hh>
#include <dumux/material/components/tabulatedcomponent.hh>

namespace Dumux {
namespace FluidSystems {

/*!
 * \ingroup Fluidsystems
 *
 * \brief A compositional one-phase fluid system with \f$H_2O\f$ \f$N_2\f$ as gaseous components
 *              and \f$CaO\f$  and \f$Ca(OH)_2/f$ as solid components drawn for thermo-chemical
 *              heat storage.
 *
 *  This fluidsystem is applied by default with the tabulated version of
 *  water of the IAPWS-formulation. However, the IAPWS-formulation has to be
 *  adapted, if to higher temperatures and higher pressures occur.
 */

template <class Scalar,
           class H2Otype = Dumux::Components::TabulatedComponent<Dumux::Components::H2O<Scalar>>,
          bool useComplexRelations=true>
class SteamN2CaO2H2
: public BaseFluidSystem<Scalar, SteamN2CaO2H2<Scalar, H2Otype, useComplexRelations> >
{
    using ThisType = SteamN2CaO2H2<Scalar, H2Otype, useComplexRelations>;
    using Base =  BaseFluidSystem <Scalar, ThisType>;

    using IdealGas = Dumux::IdealGas<Scalar>;

public:
    using H2O = H2Otype;
    using H2O_N2 = Dumux::BinaryCoeff::H2O_N2;
    using N2 = Dumux::Components::N2<Scalar>;

    using CaO = Dumux::Components::CaO<Scalar>;
    using CaO2H2 =  Dumux::Components::CaO2H2<Scalar>;

    // the type of parameter cache objects. this fluid system does not
    using ParameterCache = Dumux::NullParameterCache;

    //! Number of phases in the fluid system
    static constexpr int numPhases = 1; //gas phase: N2 and steam
    static constexpr int numComponents = 2; // H2O, Air

    static constexpr int gasPhaseIdx = 0;  //!< index of the gas phase
    static constexpr int phase0Idx = gasPhaseIdx; //!< index of the only phase

    static constexpr int N2Idx = 0;
    static constexpr int H2OIdx = 1;
    static constexpr int comp0Idx = N2Idx;
    static constexpr int comp1Idx = H2OIdx;


    /****************************************
     * Fluid phase related static parameters
     ****************************************/
    /*!
     * \brief Return the human readable name of a fluid phase
     *
     * \param phaseIdx The index of the fluid phase to consider
     */
    static std::string phaseName(int phaseIdx)
    {
        switch (phaseIdx) {
        case gasPhaseIdx: return "gas";
        }
        DUNE_THROW(Dune::InvalidStateException, "Invalid phase index " << phaseIdx);
    }

    /*!
     * \brief Returns whether the fluids are miscible
     */
    static constexpr bool isMiscible()
    { return true; }

     /*!
     * \brief Return whether a phase is liquid
     *
     * \param phaseIdx The index of the fluid phase to consider
     */
    static constexpr bool isGas (int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);
        return phaseIdx == gasPhaseIdx;
    }

    /*!
     * \brief Returns true if and only if a fluid phase is assumed to
     *        be an ideal mixture.
     *
     * We define an ideal mixture as a fluid phase where the fugacity
     * coefficients of all components times the pressure of the phase
     * are independent on the fluid composition. This assumption is true
     * if Henry's law and Rault's law apply. If you are unsure what
     * this function should return, it is safe to return false. The
     * only damage done will be (slightly) increased computation times
     * in some cases.
     *
     * \param phaseIdx The index of the fluid phase to consider
     */
    static constexpr bool isIdealMixture(int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);
        // we assume no interaction between gas molecules of different
        // components

        return true;
    }
    /*!
     * \brief Returns true if and only if a fluid phase is assumed to
     *        be compressible.
     *
     * Compressible means that the partial derivative of the density
     * to the fluid pressure is always larger than zero.
     *
     * \param phaseIdx The index of the fluid phase to consider
     */
    static constexpr bool isCompressible(int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);

        return true;
    }

    /*!
     * \brief Returns true if and only if a fluid phase is assumed to
     *        be an ideal gas.
     *
     * \param phaseIdx The index of the fluid phase to consider
     */
    static constexpr bool isIdealGas(int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);

        // let the fluid decide
        return H2O::gasIsIdeal() && N2::gasIsIdeal();
    }

    /****************************************
    * Component related static parameters
    ****************************************/
    /*!
     * \brief Return the human readable name of a component
     *
     * \param compIdx The index of the component to consider
     */
    static std::string componentName(int compIdx)
    {
        switch (compIdx)
        {
        case H2OIdx: return "H2O";
        case N2Idx: return "N2";
        }
        DUNE_THROW(Dune::InvalidStateException, "Invalid component index " << compIdx);
    }

    /*!
     * \brief Return the molar mass of a component in \f$\mathrm{[kg/mol]}\f$.
     *
     * \param compIdx The index of the component to consider
     */
    static Scalar molarMass(int compIdx)
    {
        switch (compIdx)
        {
        case H2OIdx: return H2O::molarMass();
        case N2Idx: return N2::molarMass();
        }
        DUNE_THROW(Dune::InvalidStateException, "Invalid component index " << compIdx);
    }


    /****************************************
     * thermodynamic relations
     ****************************************/
    /*!
     *\brief Initialize the fluid system's static parameters generically
     *
     * If a tabulated H2O component is used, we do our best to create
     * tables that always work.
     */
    static void init()
    {
        init(/*tempMin=*/473.15,
             /*tempMax=*/723.0,
             /*numTemptempSteps=*/25,
             /*startPressure=*/0,
             /*endPressure=*/9e6,
             /*pressureSteps=*/200);
    }

   /*!
    * \brief Initialize the fluid system's static parameters using
    *        problem specific temperature and pressure ranges
    *
    * \param tempMin The minimum temperature used for tabulation of water \f$\mathrm{[K]}\f$
    * \param tempMax The maximum temperature used for tabulation of water \f$\mathrm{[K]}\f$
    * \param nTemp The number of ticks on the temperature axis of the  table of water
    * \param pressMin The minimum pressure used for tabulation of water \f$\mathrm{[Pa]}\f$
    * \param pressMax The maximum pressure used for tabulation of water \f$\mathrm{[Pa]}\f$
    * \param nPress The number of ticks on the pressure axis of the  table of water
    */
    static void init(Scalar tempMin, Scalar tempMax, unsigned nTemp,
                      Scalar pressMin, Scalar pressMax, unsigned nPress)
    {
        if (useComplexRelations)
            std::cout << "Using complex H2O-N2-CaO2H2 fluid system\n";
        else
            std::cout << "Using fast H2O-N2-CaO2H2 fluid system\n";

        if (H2O::isTabulated) {
            std::cout << "Initializing tables for the H2O fluid properties ("
                        << nTemp*nPress
                        << " entries).\n";

            H2O::init(tempMin, tempMax, nTemp,
                                pressMin, pressMax, nPress);
        }
    }

    /*!
     * \brief Given a phase's composition, temperature, pressure, and
     *        the partial pressures of all components, return its
     *        density \f$\mathrm{[kg/m^3]}\f$.
     *
     * \param phaseIdx index of the phase
     * \param temperature phase temperature in \f$\mathrm{[K]}\f$
     * \param pressure phase pressure in \f$\mathrm{[Pa]}\f$
     * \param fluidState the fluid state
     *
     * Equation given in:
     * - Batzle & Wang (1992) \cite batzle1992
     * - cited by: Bachu & Adams (2002)
     *   "Equations of State for basin geofluids" \cite adams2002
     */
    using Base::density;
    template <class FluidState>
    static Scalar density(const FluidState &fluidState,
                          int phaseIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);

        Scalar T = fluidState.temperature(phaseIdx);
        Scalar p = fluidState.pressure(phaseIdx);

        Scalar sumMoleFrac = 0;
        for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            sumMoleFrac += fluidState.moleFraction(phaseIdx, compIdx);

        if (!useComplexRelations)
            // for the gas phase assume an ideal gas
            return
                IdealGas::molarDensity(T, p)
                * fluidState.averageMolarMass(gasPhaseIdx)
                / std::max(1e-5, sumMoleFrac);
        else
        {
            return
                (H2O::gasDensity(T, fluidState.partialPressure(gasPhaseIdx, H2OIdx)) +
                N2::gasDensity(T, fluidState.partialPressure(gasPhaseIdx, N2Idx)));
        }
      }

    /*!
     * \brief Calculate the dynamic viscosity of a fluid phase \f$\mathrm{[Pa*s]}\f$
     *
     * \param fluidState An arbitrary fluid state
     * \param phaseIdx The index of the fluid phase to consider
     *
     * \note For the viscosity of the phases the contribution of the minor
     *       component is neglected. This contribution is probably not big, but somebody
     *       would have to find out its influence.
     */
    using Base::viscosity;
    template <class FluidState>
    static Scalar viscosity(const FluidState &fluidState,
                            int phaseIdx)
    {
        assert(0 <= phaseIdx  && phaseIdx < numPhases);

        Scalar T = fluidState.temperature(phaseIdx);
        Scalar p = fluidState.pressure(phaseIdx);

        // Wilke method (Reid et al.):
        Scalar muResult = 0;
        const Scalar mu[numComponents] = {
            H2O::gasViscosity(T, H2O::vaporPressure(T)),
            N2::gasViscosity(T, p)
        };

        const Scalar M[numComponents] = {
            H2O::molarMass(),
            N2::molarMass()
        };

        Scalar sumx = 0.0;
        for (int compIdx = 0; compIdx < 2; ++compIdx)
            sumx += fluidState.moleFraction(phaseIdx, compIdx);

        sumx = std::max(1e-10, sumx);

        for (int i = 0; i < numComponents; ++i) {
            Scalar divisor = 0;
            for (int j = 0; j < numComponents; ++j) {
                Scalar phiIJ = 1 + sqrt(mu[i]/mu[j]) * pow(M[j]/M[i], 1/4.0);
                phiIJ *= phiIJ;
                phiIJ /= sqrt(8*(1 + M[i]/M[j]));
                divisor += fluidState.moleFraction(phaseIdx, j)/sumx * phiIJ;
            }
            muResult += fluidState.moleFraction(phaseIdx, i)/sumx * mu[i] / divisor;
        }
        return muResult;
    }

    /*!
     * \brief Given a phase's composition, temperature and pressure,
     *        return the binary diffusion coefficient \f$\mathrm{[m^2/s]}\f$ for components
     *        \f$\mathrm{i}\f$ and \f$\mathrm{j}\f$ in this phase.
     *
     * \param fluidState An arbitrary fluid state
     * \param phaseIdx The index of the fluid phase to consider
     * \param compIIdx The index of the first component to consider
     * \param compJIdx The index of the second component to consider
     */
    using Base::binaryDiffusionCoefficient;
    template <class FluidState>
    static Scalar binaryDiffusionCoefficient(const FluidState &fluidState,
                                             int phaseIdx,
                                             int compIIdx,
                                             int compJIdx)
    {
        assert(0 <= phaseIdx && phaseIdx < numPhases);
        assert(0 <= compIIdx && compIIdx < numComponents);
        assert(0 <= compJIdx && compJIdx < numComponents);

        Scalar temperature = fluidState.temperature(phaseIdx);
        Scalar pressure = fluidState.pressure(phaseIdx);

        assert(phaseIdx == gasPhaseIdx);

        if (compIIdx != N2Idx)
            std::swap(compIIdx, compJIdx);

        Scalar result = 0.0;
        if(compJIdx == H2OIdx)
        result = H2O_N2::gasDiffCoeff(temperature, pressure);

        else
            DUNE_THROW(Dune::NotImplemented, "Binary diffusion coefficient of components "
                                                 << compIIdx << " and " << compJIdx
                                                 << " in phase " << phaseIdx);
        Valgrind::CheckDefined(result);
        return result;
    }

    /*!
     * \brief Given a phase's composition, temperature and pressure,
     *        return its specific enthalpy \f$\mathrm{[J/kg]}\f$.
     * \param fluidState The fluid state
     * \param phaseIdx The index of the phase
     *
     * See:
     * Class 2000
     * Theorie und numerische Modellierung nichtisothermer Mehrphasenprozesse in NAPL-kontaminierten porösen Medien
     * Chapter 2.1.13 Innere Energie, Wäremekapazität, Enthalpie \cite A3:class:2001
     *
     * Formula (2.42):
     * the specific enthalpy of a gas phase result from the sum of (enthalpies*mass fraction) of the components
     */
    using Base::enthalpy;
    template <class FluidState>
    static Scalar enthalpy(const FluidState &fluidState,
                           int phaseIdx)
    {
        assert(phaseIdx == gasPhaseIdx);

        Scalar T = fluidState.temperature(phaseIdx);
        Scalar p = fluidState.pressure(phaseIdx);

        Scalar XN2 = fluidState.massFraction(gasPhaseIdx, N2Idx);
        Scalar XH2O = fluidState.massFraction(gasPhaseIdx, H2OIdx);

        Scalar result = 0;
        result += XH2O * H2O::gasEnthalpy(T, p);
        result += XN2 * N2::gasEnthalpy(T, p);
        Valgrind::CheckDefined(result);

        return result;
    }

    /*!
    * \brief Returns the specific enthalpy \f$\mathrm{[J/kg]}\f$ of a component in a specific phase
    * \param fluidState The fluid state
    * \param phaseIdx The index of the phase
    * \param componentIdx The index of the component
    */
    template <class FluidState>
    static Scalar componentEnthalpy(const FluidState &fluidState,
                                    int phaseIdx,
                                    int componentIdx)
    {
        Scalar T = fluidState.temperature(gasPhaseIdx);
        Scalar p = fluidState.pressure(gasPhaseIdx);
        Valgrind::CheckDefined(T);
        Valgrind::CheckDefined(p);

        if (componentIdx ==  H2OIdx)
        {
            return H2O::gasEnthalpy(T, p);
        }
        else if (componentIdx == N2Idx)
        {
            return N2::gasEnthalpy(T, p);
        }
    }

    //for the boundary condition T = 573.15 K;
    template <class FluidState>
    static Scalar componentEnthalpyBorder(const FluidState &fluidState,
                                    int phaseIdx,
                                    int componentIdx)
    {
        Scalar T = 573.15;
        Scalar p = fluidState.pressure(gasPhaseIdx);
        Valgrind::CheckDefined(T);
        Valgrind::CheckDefined(p);

        if (componentIdx ==  H2OIdx)
        {
            return H2O::gasEnthalpy(T, p);
        }
        else if (componentIdx == N2Idx)
        {
            return N2::gasEnthalpy(T, p);
        }
    }

    /*!
     * \brief Thermal conductivity of a fluid phase \f$\mathrm{[W/(m K)]}\f$.
     * \param phaseIdx The index of the fluid phase to consider
     *
     * \note For the thermal conductivity of the phases the contribution of the minor
     *       component is neglected. This contribution is probably not big, but somebody
     *       would have to find out its influence.
     */
    using Base::thermalConductivity;
    template <class FluidState>
    static Scalar thermalConductivity(const FluidState &fluidState,
                                      int phaseIdx)
    {
        assert(0 <= phaseIdx  && phaseIdx < numPhases);
        Scalar temperature  = fluidState.temperature(phaseIdx) ;
        Scalar pressure = fluidState.pressure(phaseIdx);

        // Isobaric Properties for Air and Carbondioxide in: NIST Standard
        // Reference Database Number 69, Eds. P.J. Linstrom and
        // W.G. Mallard evaluated at p=.1 MPa, does not
        // change dramatically with p
        // and can be interpolated linearly with temperature
        Scalar lambdaPureN2 = N2::gasThermalConductivity(temperature, pressure);

        if (useComplexRelations){
            Scalar xN2 = fluidState.moleFraction(phaseIdx, N2Idx);
            Scalar xH2O = fluidState.moleFraction(phaseIdx, H2OIdx);
            Scalar lambdaN2 = xN2 * lambdaPureN2;
                // Assuming Raoult's, Daltons law and ideal gas
                // in order to obtain the partial density of water in the air phase
            if(xH2O <= 0+ 1e-6) return lambdaN2;

            Scalar partialPressure  = pressure * xH2O;
            Scalar lambdaH2O = xH2O * H2O::gasThermalConductivity(temperature, partialPressure);

            return lambdaN2 + lambdaH2O;
        }
        else
        return lambdaPureN2; // conductivity of Air [W / (m K ) ]
    }

    /*!
     * \brief Specific isobaric heat capacity of a fluid phase.
     *        \f$\mathrm{[J/(kg*K)}\f$.
     * \param fluidState An abitrary fluid state
     * \param phaseIdx The index of the fluid phase to consider
     *
     * \note The calculation of the isobaric heat capacity is preliminary. A better
     *       description of the influence of the composition on the phase property
     *       has to be found.
     */
    using Base::heatCapacity;
    template <class FluidState>
    static Scalar heatCapacity(const FluidState &fluidState,
                               int phaseIdx)
    {
        const Scalar temperature  = fluidState.temperature(phaseIdx);
        const Scalar pressure = fluidState.pressure(phaseIdx);

        Scalar c_pN2;
        Scalar c_pH2O;
        // let the water and air components do things their own way
        c_pN2= N2::gasHeatCapacity(fluidState.temperature(phaseIdx),
                                    fluidState.pressure(phaseIdx)
                                    * fluidState.moleFraction(phaseIdx, N2Idx));

        c_pH2O = H2O::gasHeatCapacity(fluidState.temperature(phaseIdx),
                                    fluidState.pressure(phaseIdx)
                                    * fluidState.moleFraction(phaseIdx, H2OIdx));

        return c_pH2O*fluidState.moleFraction(gasPhaseIdx, H2OIdx) + c_pN2*fluidState.moleFraction(gasPhaseIdx, N2Idx);
    }

};

} // end namespace
} // end namespace

#endif
