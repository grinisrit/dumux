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
 * \brief Element-wise calculation of the local residual for problems
 *        using compositional fully implicit model.
 */
#ifndef DUMUX_COMPOSITIONAL_LOCAL_RESIDUAL_HH
#define DUMUX_COMPOSITIONAL_LOCAL_RESIDUAL_HH

namespace Dumux
{

namespace Properties
{
NEW_PROP_TAG(ReplaceCompEqIdx);
} // end namespace Properties

/*!
 * \ingroup Implicit
 * \ingroup ImplicitLocalResidual
 * \brief Element-wise calculation of the local residual for problems
 *        using compositional fully implicit model.
 *
 */
template<class TypeTag>
class CompositionalLocalResidual: public GET_PROP_TYPE(TypeTag, BaseLocalResidual)
{
    using ParentType = typename GET_PROP_TYPE(TypeTag, BaseLocalResidual);
    using Implementation = typename GET_PROP_TYPE(TypeTag, LocalResidual);
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using PrimaryVariables = typename GET_PROP_TYPE(TypeTag, PrimaryVariables);
    using FluxVariables = typename GET_PROP_TYPE(TypeTag, FluxVariables);
    using FluxVariablesCache = typename GET_PROP_TYPE(TypeTag, FluxVariablesCache);
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    using BoundaryTypes = typename GET_PROP_TYPE(TypeTag, BoundaryTypes);
    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using VolumeVariables = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using EnergyLocalResidual = typename GET_PROP_TYPE(TypeTag, EnergyLocalResidual);

    static const int numPhases = GET_PROP_VALUE(TypeTag, NumPhases);
    static const int numComponents = GET_PROP_VALUE(TypeTag, NumComponents);

    enum { conti0EqIdx = Indices::conti0EqIdx };

    //! The index of the component balance equation that gets replaced with the total mass balance
    static const int replaceCompEqIdx = GET_PROP_VALUE(TypeTag, ReplaceCompEqIdx);

public:

    CompositionalLocalResidual()
    : ParentType()
    {
        // retrieve the upwind weight for the mass conservation equations. Use the value
        // specified via the property system as default, and overwrite
        // it by the run-time parameter from the Dune::ParameterTree
        upwindWeight_ = GET_PARAM_FROM_GROUP(TypeTag, Scalar, Implicit, MassUpwindWeight);
    }
    /*!
     * \brief Evaluate the amount of all conservation quantities
     *        (e.g. phase mass) within a sub-control volume.
     *
     * The result should be averaged over the volume (e.g. phase mass
     * inside a sub control volume divided by the volume)
     *
     *  \param storage The mass of the component within the sub-control volume
     *  \param scvIdx The SCV (sub-control-volume) index
     *  \param usePrevSol Evaluate function with solution of current or previous time step
     */
    PrimaryVariables computeStorage(const SubControlVolume& scv,
                                    const VolumeVariables& volVars) const
    {
        PrimaryVariables storage(0.0);

        // compute storage term of all components within all phases
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
        {
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            {
                auto eqIdx = conti0EqIdx + compIdx;
                if (replaceCompEqIdx != eqIdx)
                {
                    storage[eqIdx] += volVars.porosity()
                                      * volVars.saturation(phaseIdx)
                                      * volVars.molarDensity(phaseIdx)
                                      * volVars.moleFraction(phaseIdx, compIdx);
                }
                else
                {
                    storage[eqIdx] += volVars.molarDensity(phaseIdx)
                                      * volVars.saturation(phaseIdx)
                                      * volVars.porosity();
                }
            }

            //! The energy storage in the fluid phase with index phaseIdx
            EnergyLocalResidual::fluidPhaseStorage(storage, scv, volVars, phaseIdx);
        }

        //! The energy storage in the solid matrix
        EnergyLocalResidual::solidPhaseStorage(storage, scv, volVars);

        return storage;
    }

    /*!
     * \brief Evaluates the total flux of all conservation quantities
     *        over a face of a sub-control volume.
     *
     * \param flux The flux over the SCV (sub-control-volume) face for each component
     * \param fIdx The index of the SCV face
     * \param onBoundary A boolean variable to specify whether the flux variables
     *        are calculated for interior SCV faces or boundary faces, default=false
     */
    PrimaryVariables computeFlux(const Element& element,
                                 const FVElementGeometry& fvGeometry,
                                 const ElementVolumeVariables& elemVolVars,
                                 const SubControlVolumeFace& scvf,
                                 const FluxVariablesCache& fluxVarsCache)
    {
        FluxVariables fluxVars;
        fluxVars.initAndComputeFluxes(this->problem(),
                                      element,
                                      fvGeometry,
                                      elemVolVars,
                                      scvf,
                                      fluxVarsCache);

        // get upwind weights into local scope
        auto w = upwindWeight_;
        PrimaryVariables flux(0.0);

        // advective fluxes
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
        {
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            {
                auto upwindRule = [w, phaseIdx, compIdx](const VolumeVariables& up, const VolumeVariables& dn)
                {
                    return ( w )*up.molarDensity(phaseIdx)
                                *up.moleFraction(phaseIdx, compIdx)
                                *up.mobility(phaseIdx)
                          +(1-w)*dn.molarDensity(phaseIdx)
                                *dn.moleFraction(phaseIdx, compIdx)
                                *dn.mobility(phaseIdx);
                };

                // get equation index
                auto eqIdx = conti0EqIdx + compIdx;
                if (eqIdx != replaceCompEqIdx)
                {
                    flux[eqIdx] += fluxVars.advectiveFlux(phaseIdx, upwindRule);

                    if (phaseIdx != compIdx)
                    {
                        const auto diffFlux = fluxVars.molecularDiffusionFlux(phaseIdx, compIdx);
                        flux[eqIdx] += diffFlux;
                        flux[conti0EqIdx + phaseIdx] -= diffFlux;
                    }
                }
                else
                {
                    auto upwindRuleTotal = [w, phaseIdx, compIdx](const VolumeVariables& up, const VolumeVariables& dn)
                    {
                        return ( w )*up.molarDensity(phaseIdx)
                                    *up.mobility(phaseIdx)
                              +(1-w)*dn.molarDensity(phaseIdx)
                                    *dn.mobility(phaseIdx);
                    };

                    flux[eqIdx] += fluxVars.advectiveFlux(phaseIdx, upwindRuleTotal);

                }
            }

            //! Add advective phase energy fluxes. For isothermal model the contribution is zero.
            EnergyLocalResidual::heatConvectionFlux(flux, fluxVars, phaseIdx, w);
        }

        //! Add diffusive energy fluxes. For isothermal model the contribution is zero.
        EnergyLocalResidual::heatConductionFlux(flux, fluxVars);

        return flux;
    }

protected:
    Implementation *asImp_()
    { return static_cast<Implementation *> (this); }

    const Implementation *asImp_() const
    { return static_cast<const Implementation *> (this); }

private:
    Scalar upwindWeight_;
};

} // end namespace Dumux

#endif