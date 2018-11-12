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
 * \ingroup RichardsModel
 * \brief This model implements a variant of the Richards'
 *        equation for quasi-twophase flow.
 *
 * In the unsaturated zone, Richards' equation
 \f[
 \frac{\partial\;\phi S_w \varrho_w}{\partial t}
 -
 \text{div} \left\lbrace
 \varrho_w \frac{k_{rw}}{\mu_w} \; \mathbf{K} \;
 \left( \text{\textbf{grad}}
 p_w - \varrho_w \textbf{g}
 \right)
 \right\rbrace
 =
 q_w,
 \f]
 * is frequently used to
 * approximate the water distribution above the groundwater level.
 *
 * It can be derived from the two-phase equations, i.e.
 \f[
 \phi\frac{\partial S_\alpha \varrho_\alpha}{\partial t}
 -
 \text{div} \left\lbrace
 \varrho_\alpha \frac{k_{r\alpha}}{\mu_\alpha}\; \mathbf{K} \;
 \left( \text{\textbf{grad}}
 p_\alpha - \varrho_\alpha \textbf{g}
 \right)
 \right\rbrace
 =
 q_\alpha,
 \f]
 * where \f$\alpha \in \{w, n\}\f$ is the fluid phase,
 * \f$\kappa \in \{ w, a \}\f$ are the components,
 * \f$\rho_\alpha\f$ is the fluid density, \f$S_\alpha\f$ is the fluid
 * saturation, \f$\phi\f$ is the porosity of the soil,
 * \f$k_{r\alpha}\f$ is the relative permeability for the fluid,
 * \f$\mu_\alpha\f$ is the fluid's dynamic viscosity, \f$\mathbf{K}\f$ is the
 * intrinsic permeability, \f$p_\alpha\f$ is the fluid pressure and
 * \f$g\f$ is the potential of the gravity field.
 *
 * In contrast to the full two-phase model, the Richards model assumes
 * gas as the non-wetting fluid and that it exhibits a much lower
 * viscosity than the (liquid) wetting phase. (For example at
 * atmospheric pressure and at room temperature, the viscosity of air
 * is only about \f$1\%\f$ of the viscosity of liquid water.) As a
 * consequence, the \f$\frac{k_{r\alpha}}{\mu_\alpha}\f$ term
 * typically is much larger for the gas phase than for the wetting
 * phase. For this reason, the Richards model assumes that
 * \f$\frac{k_{rn}}{\mu_n}\f$ is infinitly large. This implies that
 * the pressure of the gas phase is equivalent to the static pressure
 * distribution and that therefore, mass conservation only needs to be
 * considered for the wetting phase.
 *
 * The model thus choses the absolute pressure of the wetting phase
 * \f$p_w\f$ as its only primary variable. The wetting phase
 * saturation is calculated using the inverse of the capillary
 * pressure, i.e.
 \f[
 S_w = p_c^{-1}(p_n - p_w)
 \f]
 * holds, where \f$p_n\f$ is a given reference pressure. Nota bene,
 * that the last step is assumes that the capillary
 * pressure-saturation curve can be uniquely inverted, so it is not
 * possible to set the capillary pressure to zero when using the
 * Richards model!
 */

#ifndef DUMUX_RICHARDS_MODEL_HH
#define DUMUX_RICHARDS_MODEL_HH

#include <dune/common/fvector.hh>

#include <dumux/common/properties.hh>

#include <dumux/porousmediumflow/immiscible/localresidual.hh>
#include <dumux/porousmediumflow/compositional/switchableprimaryvariables.hh>
#include <dumux/material/fluidmatrixinteractions/diffusivitymillingtonquirk.hh>
#include <dumux/material/fluidmatrixinteractions/2p/thermalconductivitysomerton.hh>
#include <dumux/material/spatialparams/fv.hh>
#include <dumux/material/components/simpleh2o.hh>
#include <dumux/material/fluidsystems/h2oair.hh>
#include <dumux/material/fluidstates/immiscible.hh>

#include <dumux/porousmediumflow/properties.hh>
#include <dumux/porousmediumflow/nonisothermal/model.hh>
#include <dumux/porousmediumflow/nonisothermal/indices.hh>
#include <dumux/porousmediumflow/nonisothermal/iofields.hh>

#include "indices.hh"
#include "volumevariables.hh"
#include "iofields.hh"
#include "localresidual.hh"
#include "primaryvariableswitch.hh"

namespace Dumux {

/*!
 * \ingroup RichardsModel
 * \brief Specifies a number properties of the Richards model.
 *
 * \tparam enableDiff specifies if diffusion of water in air is to be considered.
 */
template<bool enableDiff>
struct RichardsModelTraits
{
    using Indices = RichardsIndices;

    static constexpr int numEq() { return 1; }
    static constexpr int numPhases() { return 2; }
    static constexpr int numComponents() { return 1; }

    static constexpr bool enableAdvection() { return true; }
    static constexpr bool enableMolecularDiffusion() { return enableDiff; }
    static constexpr bool enableEnergyBalance() { return false; }

    template<class FluidSystem, class SolidSystem = void>
    static std::string primaryVariableName(int pvIdx, int state)
    {
        if (state == Indices::gasPhaseOnly)
            return "x^" + FluidSystem::componentName(FluidSystem::comp0Idx)
                   + "_" + FluidSystem::phaseName(FluidSystem::phase1Idx);
        else
            return IOName::pressure<FluidSystem>(FluidSystem::phase0Idx);
    }
};

/*!
 * \ingroup RichardsModel
 * \brief Traits class for the Richards model.
 *
 * \tparam PV The type used for primary variables
 * \tparam FSY The fluid system type
 * \tparam FST The fluid state type
 * \tparam PT The type used for permeabilities
 * \tparam MT The model traits
 */
template<class PV, class FSY, class FST, class SSY, class SST, class PT, class MT>
struct RichardsVolumeVariablesTraits
{
    using PrimaryVariables = PV;
    using FluidSystem = FSY;
    using FluidState = FST;
    using SolidSystem = SSY;
    using SolidState = SST;
    using PermeabilityType = PT;
    using ModelTraits = MT;
};

// \{
///////////////////////////////////////////////////////////////////////////
// properties for the isothermal Richards model.
///////////////////////////////////////////////////////////////////////////
namespace Properties {

//////////////////////////////////////////////////////////////////
// Type tags
//////////////////////////////////////////////////////////////////

//! The type tags for the implicit isothermal one-phase two-component problems
NEW_TYPE_TAG(Richards, INHERITS_FROM(PorousMediumFlow));
NEW_TYPE_TAG(RichardsNI, INHERITS_FROM(Richards));

//////////////////////////////////////////////////////////////////
// Properties values
//////////////////////////////////////////////////////////////////

//! The local residual operator
SET_TYPE_PROP(Richards, LocalResidual, RichardsLocalResidual<TypeTag>);

//! Set the vtk output fields specific to this model
SET_PROP(Richards, IOFields)
{
private:
    static constexpr bool enableWaterDiffusionInAir
        = GET_PROP_VALUE(TypeTag, EnableWaterDiffusionInAir);

public:
    using type = RichardsIOFields<enableWaterDiffusionInAir>;
};

//! The model traits
SET_TYPE_PROP(Richards, ModelTraits, RichardsModelTraits<GET_PROP_VALUE(TypeTag, EnableWaterDiffusionInAir)>);

//! Set the volume variables property
SET_PROP(Richards, VolumeVariables)
{
private:
    using PV = typename GET_PROP_TYPE(TypeTag, PrimaryVariables);
    using FSY = typename GET_PROP_TYPE(TypeTag, FluidSystem);
    using FST = typename GET_PROP_TYPE(TypeTag, FluidState);
    using SSY = typename GET_PROP_TYPE(TypeTag, SolidSystem);
    using SST = typename GET_PROP_TYPE(TypeTag, SolidState);
    using MT = typename GET_PROP_TYPE(TypeTag, ModelTraits);
    using PT = typename GET_PROP_TYPE(TypeTag, SpatialParams)::PermeabilityType;

    using Traits = RichardsVolumeVariablesTraits<PV, FSY, FST, SSY, SST, PT, MT>;
public:
    using type = RichardsVolumeVariables<Traits>;
};

//! The default richards model computes no diffusion in the air phase
//! Turning this on leads to the extended Richards equation (see e.g. Vanderborght et al. 2017)
SET_BOOL_PROP(Richards, EnableWaterDiffusionInAir, false);

//! Use the model after Millington (1961) for the effective diffusivity
SET_TYPE_PROP(Richards, EffectiveDiffusivityModel,
              DiffusivityMillingtonQuirk<typename GET_PROP_TYPE(TypeTag, Scalar)>);

//! The primary variables vector for the richards model
SET_PROP(Richards, PrimaryVariables)
{
private:
    using PrimaryVariablesVector = Dune::FieldVector<typename GET_PROP_TYPE(TypeTag, Scalar),
                                                     GET_PROP_TYPE(TypeTag, ModelTraits)::numEq()>;
public:
    using type = SwitchablePrimaryVariables<PrimaryVariablesVector, int>;
};

//! The primary variable switch for the richards model
SET_TYPE_PROP(Richards, PrimaryVariableSwitch, ExtendedRichardsPrimaryVariableSwitch);

//! The primary variable switch for the richards model
// SET_BOOL_PROP(Richards, ProblemUsePrimaryVariableSwitch, false);

/*!
 *\brief The fluid system used by the model.
 *
 * By default this uses the H2O-Air fluid system with Simple H2O (constant density and viscosity).
 */
SET_PROP(Richards, FluidSystem)
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using type = FluidSystems::H2OAir<Scalar,
                                      Components::SimpleH2O<Scalar>,
                                      FluidSystems::H2OAirDefaultPolicy</*fastButSimplifiedRelations=*/true>>;
};

/*!
 * \brief The fluid state which is used by the volume variables to
 *        store the thermodynamic state. This should be chosen
 *        appropriately for the model ((non-)isothermal, equilibrium, ...).
 *        This can be done in the problem.
 */
SET_PROP(Richards, FluidState)
{
private:
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
public:
    using type = ImmiscibleFluidState<Scalar, FluidSystem>;
};

//! Somerton is used as default model to compute the effective thermal heat conductivity
SET_PROP(RichardsNI, ThermalConductivityModel)
{
private:
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
public:
    using type = ThermalConductivitySomerton<Scalar>;
};

/////////////////////////////////////////////////////
// Property values for non-isothermal Richars model
/////////////////////////////////////////////////////

//! set non-isothermal model traits
SET_PROP(RichardsNI, ModelTraits)
{
private:
    using IsothermalTraits = RichardsModelTraits<GET_PROP_VALUE(TypeTag, EnableWaterDiffusionInAir)>;
public:
    using type = PorousMediumFlowNIModelTraits<IsothermalTraits>;
};

//! Set the vtk output fields specific to th non-isothermal model
SET_PROP(RichardsNI, IOFields)
{
    static constexpr bool enableWaterDiffusionInAir
        = GET_PROP_VALUE(TypeTag, EnableWaterDiffusionInAir);
    using RichardsIOF = RichardsIOFields<enableWaterDiffusionInAir>;
    using type = EnergyIOFields<RichardsIOF>;
};

// \}
} // end namespace Properties
} // end namespace Dumux

#endif
