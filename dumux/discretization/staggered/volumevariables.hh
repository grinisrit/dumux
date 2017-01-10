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
 * \brief Base class for the model specific class which provides
 *        access to all volume averaged quantities.
 */
#ifndef DUMUX_DISCRETIZATION_STAGGERED_VOLUME_VARIABLES_HH
#define DUMUX_DISCRETIZATION_STAGGERED_VOLUME_VARIABLES_HH

#include <dumux/implicit/properties.hh>
#include <dumux/common/valgrind.hh>

namespace Dumux
{

namespace Properties
{
NEW_PROP_TAG(FluidSystem);
NEW_PROP_TAG(Indices);
NEW_PROP_TAG(EnableEnergyTransport);
}

// forward declaration
template <class TypeTag, bool enableEnergyBalance>
class StaggeredVolumeVariablesImplementation;

/*!
 * \ingroup ImplicitVolumeVariables
 * \brief Base class for the model specific class which provides
 *        access to all volume averaged quantities. The volume variables base class
 *        is specialized for isothermal and non-isothermal models.
 */
template <class TypeTag>
using StaggeredVolumeVariables = StaggeredVolumeVariablesImplementation<TypeTag, GET_PROP_VALUE(TypeTag, EnableEnergyTransport)>;

/*!
 * \ingroup StaggeredVolumeVariables
 * \brief The isothermal base class
 */
template<class TypeTag>
class StaggeredVolumeVariablesImplementation<TypeTag, false>
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using CellCenterPrimaryVariables = typename GET_PROP_TYPE(TypeTag, CellCenterPrimaryVariables);
    using FacePrimaryVariables = typename GET_PROP_TYPE(TypeTag, FacePrimaryVariables);

public:

    /*!
     * \brief Update all quantities for a given control volume
     *
     * \param priVars A vector containing the primary variables for the control volume
     * \param problem The object specifying the problem which ought to
     *                be simulated
     * \param element An element which contains part of the control volume
     * \param fvGeometry The finite volume geometry for the element
     * \param scvIdx Local index of the sub control volume which is inside the element
     * \param isOldSol Specifies whether this is the previous solution or the current one
     *
     * \todo Eliminate the 'isOldSol' parameter. This implies that the
     *       'pseudo-primary variables' must be somehow be stored
     *       inside the PrimaryVariables. (e.g. we need to know the
     *       phase state in the 2p2c model)
     */
    void update(const CellCenterPrimaryVariables &ccPriVars,
                const Problem &problem,
                const Element &element,
                const SubControlVolume &scv)
    {
        extrusionFactor_ = problem.boxExtrusionFactor(element, scv);
        ccPriVars_ = ccPriVars;
    }



    /*!
     * \brief Return the vector of primary variables
     */
    CellCenterPrimaryVariables ccPriVars() const
    { return ccPriVars_; }

    /*!
     * \brief Return a component of primary variable vector
     *
     * \param pvIdx The index of the primary variable of interest
     */
    Scalar ccPriVar(const int pvIdx) const
    { return ccPriVars_[pvIdx]; }


    /*!
     * \brief Return how much the sub-control volume is extruded.
     *
     * This means the factor by which a lower-dimensional (1D or 2D)
     * entity needs to be expanded to get a full dimensional cell. The
     * default is 1.0 which means that 1D problems are actually
     * thought as pipes with a cross section of 1 m^2 and 2D problems
     * are assumed to extend 1 m to the back.
     */
    Scalar extrusionFactor() const
    { return extrusionFactor_; }

    //! The temperature is obtained from the problem as a constant for isothermal models
    static Scalar temperature(const CellCenterPrimaryVariables &priVars,
                              const Problem& problem,
                              const Element &element,
                              const SubControlVolume &scv)
    {
        return problem.temperatureAtPos(scv.dofPosition());
    }

    //! The phase enthalpy is zero for isothermal models
    //! This is needed for completing the fluid state
    template<class FluidState, class ParameterCache>
    static Scalar enthalpy(const FluidState& fluidState,
                           const ParameterCache& paramCache,
                           const int phaseIdx)
    {
        return 0;
    }

private:
    Scalar extrusionFactor_;

    CellCenterPrimaryVariables ccPriVars_;
};

//! The non-isothermal implicit volume variables base class
template <class TypeTag>
class StaggeredVolumeVariablesImplementation<TypeTag, true>
: public StaggeredVolumeVariablesImplementation<TypeTag, false>
{
    using ParentType = StaggeredVolumeVariablesImplementation<TypeTag, false>;
    using Implementation = typename GET_PROP_TYPE(TypeTag, VolumeVariables);
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Element = typename GridView::template Codim<0>::Entity;
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using FluidSystem = typename GET_PROP_TYPE(TypeTag, FluidSystem);
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    using CellCenterPrimaryVariables = typename GET_PROP_TYPE(TypeTag, CellCenterPrimaryVariables);

    enum { temperatureIdx = Indices::temperatureIdx };

public:

    /*!
     * \brief Update all quantities for a given control volume
     *
     * \param priVars A vector containing the primary variables for the control volume
     * \param problem The object specifying the problem which ought to
     *                be simulated
     * \param element An element which contains part of the control volume
     * \param fvGeometry The finite volume geometry for the element
     * \param scvIdx Local index of the sub control volume which is inside the element
     */
    void update(const CellCenterPrimaryVariables &ccPriVars,
                const Problem &problem,
                const Element &element,
                const SubControlVolume &scv)
    {
        ParentType::update(ccPriVars, problem, element, scv);
    }

    /*!
     * \brief Returns the total internal energy of a phase in the
     *        sub-control volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar internalEnergy(const int phaseIdx) const
    { return asImp_().fluidState().internalEnergy(phaseIdx); }

    /*!
     * \brief Returns the total enthalpy of a phase in the sub-control
     *        volume.
     *
     * \param phaseIdx The phase index
     */
    Scalar enthalpy(const int phaseIdx) const
    { return asImp_().fluidState().enthalpy(phaseIdx); }


    /*!
     * \brief Returns the thermal conductivity \f$\mathrm{[W/(m*K)]}\f$ of a fluid phase in
     *        the sub-control volume.
     */
    Scalar fluidThermalConductivity(const int phaseIdx) const
    { return FluidSystem::thermalConductivity(asImp_().fluidState(), phaseIdx); }


    //! The temperature is a primary variable for non-isothermal models
    static Scalar temperature(const CellCenterPrimaryVariables &priVars,
                              const Problem& problem,
                              const Element &element,
                              const SubControlVolume &scv)
    {
        return priVars[temperatureIdx];
    }

    //! The phase enthalpy is zero for isothermal models
    //! This is needed for completing the fluid state
    template<class FluidState, class ParameterCache>
    static Scalar enthalpy(const FluidState& fluidState,
                           const ParameterCache& paramCache,
                           const int phaseIdx)
    {
        return FluidSystem::enthalpy(fluidState, paramCache, phaseIdx);
    }

protected:
    const Implementation &asImp_() const
    { return *static_cast<const Implementation*>(this); }
    Implementation &asImp_()
    { return *static_cast<Implementation*>(this); }
};

} // end namespace Dumux

#endif
