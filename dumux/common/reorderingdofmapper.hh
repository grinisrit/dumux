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
 * \brief An SCSG element mapper that sorts the indices in order to optimize the matrix sparsity pattern
 * \note The reordering needs the SCOTCH library
 */
#ifndef DUMUX_REORDERING_DOF_MAPPER_HH
#define DUMUX_REORDERING_DOF_MAPPER_HH

#include <vector>
#include <dune/common/timer.hh>
#include <dune/grid/common/mapper.hh>

#include <dumux/linear/scotchbackend.hh>

namespace Dumux
{
/*!
 * \brief An SCSG element mapper that sorts the indices in order to optimize the matrix sparsity pattern
 */
template<class GridView, int codimension>
class ReorderingDofMapper
: public Dune::Mapper<typename GridView::Grid, ReorderingDofMapper<GridView, codimension>, typename GridView::IndexSet::IndexType>
{
    using Index = typename GridView::IndexSet::IndexType;
    using ParentType = typename Dune::Mapper<typename GridView::Grid, ReorderingDofMapper<GridView, codimension>, Index>;
    using Element = typename GridView::template Codim<0>::Entity;

public:

    /*!
     * \brief Construct mapper from grid and one of its index sets.
     * \param gridView A Dune GridView object.
     */
    ReorderingDofMapper (const GridView& gridView)
    : gridView_(gridView),
      indexSet_(gridView.indexSet())
    {
        static_assert(codimension == 0 || codimension == GridView::dimension, "The reordering dofMapper is only implemented for element or vertex dofs");
        update();
    }

    /*!
     * \brief Map entity to array index.
     *
     * \tparam EntityType
     * \param e Reference to codim \a EntityType entity.
     * \return An index in the range 0 ... Max number of entities in set - 1.
     */
    template<class EntityType>
    Index index (const EntityType& e) const
    {
        // map the index using the permutation obtained from the reordering algorithm
        return static_cast<Index>(permutation_[indexSet_.index(e)]);
    }

    /** @brief Map subentity of codim 0 entity to array index.

       \param e Reference to codim 0 entity.
       \param i Number of subentity of e
       \param codim Codimension of the subentity
       \return An index in the range 0 ... Max number of entities in set - 1.
     */
    Index subIndex (const Element& e, int i, unsigned int codim) const
    {
        return indexSet_.subIndex(e, i, codim);
    }

    /** @brief Return total number of entities in the entity set managed by the mapper.

       This number can be used to allocate a vector of data elements associated with the
       entities of the set. In the parallel case this number is per process (i.e. it
       may be different in different processes).

       \return Size of the entity set.
     */
    std::size_t size () const
    {
        return indexSet_.size(codimension);
    }

    /** @brief Returns true if the entity is contained in the index set

       \param e Reference to entity
       \param result integer reference where corresponding index is  stored if true
       \return true if entity is in entity set of the mapper
     */
    template<class EntityType>
    bool contains (const EntityType& e, Index& result) const
    {
        result = index(e);
        return true;
    }

    /** @brief Returns true if the entity is contained in the index set

       \param e Reference to codim 0 entity
       \param i subentity number
       \param cc subentity codim
       \param result integer reference where corresponding index is  stored if true
       \return true if entity is in entity set of the mapper
     */
    bool contains (const Element& e, int i, int cc, Index& result) const
    {
        result = indexSet_.subIndex(e, i, cc);
        return true;
    }

    /*!
     * \brief Recalculates map after mesh adaptation
     */
    void update ()
    {
        // Compute scotch reordering
        Dune::Timer watch;
        // Create the graph as an adjacency list
        std::vector<std::vector<Index>> graph(size());

        // dofs on element centers (cell-centered methods)
        if (codimension == 0)
        {
            for (const auto& element : elements(gridView_))
            {
                auto eIdx = indexSet_.index(element);
                for (const auto& intersection : intersections(gridView_, element))
                {
                    if (intersection.neighbor())
                        graph[eIdx].push_back(indexSet_.index(intersection.outside()));
                }
            }
        }
        // dof on vertices (box method)
        else
        {
            for (const auto& element : elements(gridView_))
            {
                auto eIdx = indexSet_.index(element);
                for (int vIdxLocal = 0; vIdxLocal < element.subEntities(codimension); ++vIdxLocal)
                {
                    auto vIdxGlobal = indexSet_.subIndex(element, vIdxLocal, codimension);
                    graph[vIdxGlobal].push_back(eIdx);
                }
            }
        }

        permutation_ = ScotchBackend<Index>::computeGPSReordering(graph);
        std::cout << "Scotch backend reordered index set of size " << size()
                  << " in " << watch.elapsed() << " seconds." << std::endl;
    }

private:
    // GridView is needed to keep the IndexSet valid
    const GridView gridView_;
    const typename GridView::IndexSet& indexSet_;
    // the map resulting from the reordering
    std::vector<int> permutation_;
};

//! Redordering dof mapper for vertex-centered methods, box method
template<class GridView>
using BoxReorderingDofMapper = ReorderingDofMapper<GridView, GridView::dimension>;

//! Reordering dof mapper for the cell-centered methods
template<class GridView>
using CCReorderingDofMapper = ReorderingDofMapper<GridView, 0>;

} // end namespace Dumux

#endif
