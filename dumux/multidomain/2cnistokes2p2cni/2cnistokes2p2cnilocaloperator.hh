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
 * \brief This local operator extends the 2cstokes2p2clocaloperator
 *        by non-isothermal conditions.
 */
#ifndef DUMUX_TWOCNISTOKES2P2CNILOCALOPERATOR_HH
#define DUMUX_TWOCNISTOKES2P2CNILOCALOPERATOR_HH

#include <dumux/multidomain/2cstokes2p2c/2cstokes2p2clocaloperator.hh>

//#include <dumux/freeflow/stokesncni/stokesncnimodel.hh>
//#include <dumux/implicit/2p2cnicoupling/2p2cnicouplingmodel.hh>

namespace Dumux {

/*!
 * \brief The extension of the local operator for the coupling of a two-component Stokes model
 *        and a two-phase two-component Darcy model for non-isothermal conditions.
 */
template<class TypeTag>
class TwoCNIStokesTwoPTwoCNILocalOperator :
        public TwoCStokesTwoPTwoCLocalOperator<TypeTag>
{
public:
    typedef typename GET_PROP_TYPE(TypeTag, Problem) GlobalProblem;

    // Get the TypeTags of the subproblems
    typedef typename GET_PROP_TYPE(TypeTag, SubDomain1TypeTag) Stokes2cniTypeTag;
    typedef typename GET_PROP_TYPE(TypeTag, SubDomain2TypeTag) TwoPTwoCNITypeTag;

    typedef typename GET_PROP_TYPE(Stokes2cniTypeTag, FluxVariables) BoundaryVariables1;
    typedef typename GET_PROP_TYPE(TwoPTwoCNITypeTag, FluxVariables) BoundaryVariables2;

    // Multidomain Grid and Subgrid types
    typedef typename GET_PROP_TYPE(TypeTag, MultiDomainGrid) MDGrid;
    typedef typename GET_PROP_TYPE(Stokes2cniTypeTag, GridView) Stokes2cniGridView;
    typedef typename GET_PROP_TYPE(TwoPTwoCNITypeTag, GridView) TwoPTwoCNIGridView;

    typedef typename Stokes2cniGridView::template Codim<0>::Entity SDElement1;
    typedef typename TwoPTwoCNIGridView::template Codim<0>::Entity SDElement2;

    typedef typename GET_PROP_TYPE(Stokes2cniTypeTag, Indices) Stokes2cniIndices;
    typedef typename GET_PROP_TYPE(TwoPTwoCNITypeTag, Indices) TwoPTwoCNIIndices;

    enum { dim = MDGrid::dimension };
    enum {
        energyEqIdx1 = Stokes2cniIndices::energyEqIdx          //!< Index of the energy balance equation
    };
    enum {  numComponents = Stokes2cniIndices::numComponents };
    enum { // indices in the Darcy domain
        numPhases2 = GET_PROP_VALUE(TwoPTwoCNITypeTag, NumPhases),

        // equation index
        energyEqIdx2 = TwoPTwoCNIIndices::energyEqIdx,      //!< Index of the energy balance equation

        wPhaseIdx2 = TwoPTwoCNIIndices::wPhaseIdx,          //!< Index for the liquid phase
        nPhaseIdx2 = TwoPTwoCNIIndices::nPhaseIdx           //!< Index for the gas phase
    };

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef Dune::FieldVector<Scalar, dim> DimVector;       //!< A field vector with dim entries
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

    typedef TwoCStokesTwoPTwoCLocalOperator<TypeTag> ParentType;

    TwoCNIStokesTwoPTwoCNILocalOperator(GlobalProblem& globalProblem)
        : ParentType(globalProblem)
    { }

    static const bool doAlphaCoupling = true;
    static const bool doPatternCoupling = true;

    //! \copydoc Dumux::TwoCStokesTwoPTwoCLocalOperator::evalCoupling12()
    template<typename LFSU1, typename LFSU2, typename RES1, typename RES2, typename CParams>
    void evalCoupling12(const LFSU1& lfsu_s, const LFSU2& lfsu_n,
                        const int vertInElem1, const int vertInElem2,
                        const SDElement1& sdElement1, const SDElement2& sdElement2,
                        const BoundaryVariables1& boundaryVars1, const BoundaryVariables2& boundaryVars2,
                        const CParams &cParams,
                        RES1& couplingRes1, RES2& couplingRes2) const
    {
        const DimVector& globalPos1 = cParams.fvGeometry1.subContVol[vertInElem1].global;
        const DimVector& bfNormal1 = boundaryVars1.face().normal;
        const Scalar normalMassFlux1 = boundaryVars1.normalVelocity() *
            cParams.elemVolVarsCur1[vertInElem1].density();
        GlobalProblem& globalProblem = this->globalProblem();

        // evaluate coupling of mass and momentum balances
        ParentType::evalCoupling12(lfsu_s, lfsu_n,
                                   vertInElem1, vertInElem2,
                                   sdElement1, sdElement2,
                                   boundaryVars1, boundaryVars2,
                                   cParams,
                                   couplingRes1, couplingRes2);

        if (cParams.boundaryTypes2.isCouplingInflow(energyEqIdx2))
        {
            if (globalProblem.sdProblem1().isCornerPoint(globalPos1))
            {
                const Scalar convectiveFlux =
                    normalMassFlux1 *
                    cParams.elemVolVarsCur1[vertInElem1].enthalpy();
                const Scalar conductiveFlux =
                    bfNormal1 *
                    boundaryVars1.temperatureGrad() *
                    (boundaryVars1.thermalConductivity() + boundaryVars1.thermalEddyConductivity());
                Scalar diffusiveFlux = 0.0;
                for (int compIdx=0; compIdx < numComponents; compIdx++)
                {
                    diffusiveFlux += boundaryVars1.moleFractionGrad(compIdx)
                                     * boundaryVars1.face().normal
                                     *(boundaryVars1.diffusionCoeff(compIdx) + boundaryVars1.eddyDiffusivity())
                                     * boundaryVars1.molarDensity()
                                     * FluidSystem::molarMass(compIdx) // Multiplied by molarMass [kg/mol] to convert from [mol/m^3 s] to [kg/m^3 s]
                                     * boundaryVars1.componentEnthalpy(compIdx);
                }
                couplingRes2.accumulate(lfsu_n.child(energyEqIdx2), vertInElem2,
                                        -(convectiveFlux - diffusiveFlux - conductiveFlux));
            }
            else
            {
                // the energy flux from the stokes domain
                couplingRes2.accumulate(lfsu_n.child(energyEqIdx2), vertInElem2,
                                        globalProblem.localResidual1().residual(vertInElem1)[energyEqIdx1]);
            }
        }
        if (cParams.boundaryTypes2.isCouplingOutflow(energyEqIdx2))
        {
            // set residualDarcy[energyEqIdx2] = T in 2p2cnilocalresidual.hh
            couplingRes2.accumulate(lfsu_n.child(energyEqIdx2), vertInElem2,
                                    -cParams.elemVolVarsCur1[vertInElem1].temperature());
        }
    }

    //! \copydoc Dumux::TwoCStokesTwoPTwoCLocalOperator::evalCoupling21()
    template<typename LFSU1, typename LFSU2, typename RES1, typename RES2, typename CParams>
    void evalCoupling21(const LFSU1& lfsu_s, const LFSU2& lfsu_n,
                        const int vertInElem1, const int vertInElem2,
                        const SDElement1& sdElement1, const SDElement2& sdElement2,
                        const BoundaryVariables1& boundaryVars1, const BoundaryVariables2& boundaryVars2,
                        const CParams &cParams,
                        RES1& couplingRes1, RES2& couplingRes2) const
    {
        GlobalProblem& globalProblem = this->globalProblem();

        // evaluate coupling of mass and momentum balances
        ParentType::evalCoupling21(lfsu_s, lfsu_n,
                                   vertInElem1, vertInElem2,
                                   sdElement1, sdElement2,
                                   boundaryVars1, boundaryVars2,
                                   cParams,
                                   couplingRes1, couplingRes2);

        // TODO: this should be done only once
        const DimVector& globalPos2 = cParams.fvGeometry2.subContVol[vertInElem2].global;
        //        const DimVector& bfNormal2 = boundaryVars2.face().normal;
        DimVector normalMassFlux2(0.);

        // velocity*normal*area*rho
        for (int phaseIdx=0; phaseIdx<numPhases2; ++phaseIdx)
            normalMassFlux2[phaseIdx] = -boundaryVars2.volumeFlux(phaseIdx)*
                cParams.elemVolVarsCur2[vertInElem2].density(phaseIdx);
        ////////////////////////////////////////

        if (cParams.boundaryTypes1.isCouplingOutflow(energyEqIdx1))
        {
            // set residualStokes[energyIdx1] = T in stokes2cnilocalresidual.hh
            couplingRes1.accumulate(lfsu_s.child(energyEqIdx1), vertInElem1,
                                    -cParams.elemVolVarsCur2[vertInElem2].temperature());
        }
        if (cParams.boundaryTypes1.isCouplingInflow(energyEqIdx1))
        {
            if (globalProblem.sdProblem2().isCornerPoint(globalPos2))
            {
                const Scalar convectiveFlux = normalMassFlux2[nPhaseIdx2] *
                    cParams.elemVolVarsCur2[vertInElem2].enthalpy(nPhaseIdx2)
                    +
                    normalMassFlux2[wPhaseIdx2] *
                    cParams.elemVolVarsCur2[vertInElem2].enthalpy(wPhaseIdx2);
                const Scalar conductiveFlux = boundaryVars2.normalMatrixHeatFlux();

                couplingRes1.accumulate(lfsu_s.child(energyEqIdx1), vertInElem1,
                                        -(convectiveFlux - conductiveFlux));
            }
            else
            {
                couplingRes1.accumulate(lfsu_s.child(energyEqIdx1), vertInElem1,
                                        globalProblem.localResidual2().residual(vertInElem2)[energyEqIdx2]);
            }
        }
    }
};
} // end namespace Dumux

#endif // DUMUX_TWOCNISTOKES2P2CNILOCALOPERATOR_HH
