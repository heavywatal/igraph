/* -*- mode: C -*-  */
/*
   IGraph library.
   Copyright (C) 2006-2012  Gabor Csardi <csardi.gabor@gmail.com>
   334 Harvard street, Cambridge, MA 02139 USA

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA

*/

/* The original version of this file was written by Joerg Reichardt
   The original copyright notice follows here */

/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Tue Jul 13 11:26:47 CEST 2004
    copyright            : (C) 2004 by
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "NetDataTypes.h"
#include "NetRoutines.h"
#include "pottsmodel_2.h"

#include "igraph_community.h"
#include "igraph_components.h"
#include "igraph_error.h"
#include "igraph_interface.h"
#include "igraph_random.h"

#include "core/interruption.h"
#include "core/exceptions.h"

static igraph_error_t igraph_i_community_spinglass_orig(
        const igraph_t *graph,
        const igraph_vector_t *weights,
        igraph_real_t *modularity,
        igraph_real_t *temperature,
        igraph_vector_int_t *membership,
        igraph_vector_int_t *csize,
        igraph_integer_t spins,
        igraph_bool_t parupdate,
        igraph_real_t starttemp,
        igraph_real_t stoptemp,
        igraph_real_t coolfact,
        igraph_spincomm_update_t update_rule,
        igraph_real_t gamma);

static igraph_error_t igraph_i_community_spinglass_negative(
        const igraph_t *graph,
        const igraph_vector_t *weights,
        igraph_real_t *modularity,
        igraph_real_t *temperature,
        igraph_vector_int_t *membership,
        igraph_vector_int_t *csize,
        igraph_integer_t spins,
        igraph_bool_t parupdate,
        igraph_real_t starttemp,
        igraph_real_t stoptemp,
        igraph_real_t coolfact,
        igraph_spincomm_update_t update_rule,
        igraph_real_t gamma,
        igraph_real_t gamma_minus);

/**
 * \function igraph_community_spinglass
 * \brief Community detection based on statistical mechanics.
 *
 * This function implements the community structure detection
 * algorithm proposed by Joerg Reichardt and Stefan Bornholdt.
 * The algorithm is described in their paper: Statistical Mechanics of
 * Community Detection, http://arxiv.org/abs/cond-mat/0603718 .
 *
 * </para><para>
 * From version 0.6, igraph also supports an extension to
 * the algorithm that allows negative edge weights. This is described
 * in  V. A. Traag and Jeroen Bruggeman: Community detection in networks
 * with positive and negative links, http://arxiv.org/abs/0811.2329 .
 *
 * \param graph The input graph, it may be directed but the direction
 *     of the edges is ignored by the algorithm.
 * \param weights The vector giving the edge weights, it may be \c NULL,
 *     in which case all edges are weighted equally. The edge weights
 *     must be positive unless using the \c IGRAPH_SPINCOMM_IMP_NEG
 *     implementation.
 * \param modularity Pointer to a real number, if not \c NULL then the
 *     modularity score of the solution will be stored here. This is the
 *     gereralized modularity, taking into account the resolution parameter
 *     \p gamma. See \ref igraph_modularity() for details.
 * \param temperature Pointer to a real number, if not \c NULL then
 *     the temperature at the end of the algorithm will be stored
 *     here.
 * \param membership Pointer to an initialized vector or \c NULL. If
 *     not \c NULL then the result of the clustering will be stored
 *     here. For each vertex, the number of its cluster is given, with the
 *     first cluster numbered zero. The vector will be resized as
 *     needed.
 * \param csize Pointer to an initialized vector or \c NULL. If not \c
 *     NULL then the sizes of the clusters will stored here in cluster
 *     number order. The vector will be resized as needed.
 * \param spins Integer giving the number of spins, i.e. the maximum
 *     number of clusters. Even if the number of spins is high the number of
 *     clusters in the result might be small.
 * \param parupdate A Boolean constant, whether to update all spins in
 *     parallel. It is not implemented in the \c IGRAPH_SPINCOMM_INP_NEG
 *     implementation.
 * \param starttemp Real number, the temperature at the start. A reasonable
 *     default is 1.0.
 * \param stoptemp Real number, the algorithm stops at this temperature. A
 *     reasonable default is 0.01.
 * \param coolfact Real number, the cooling factor for the simulated
 *     annealing. A reasonable default is 0.99.
 * \param update_rule The type of the update rule. Possible values: \c
 *     IGRAPH_SPINCOMM_UPDATE_SIMPLE and \c
 *     IGRAPH_SPINCOMM_UPDATE_CONFIG. Basically this parameter defines
 *     the null model based on which the actual clustering is done. If
 *     this is \c IGRAPH_SPINCOMM_UPDATE_SIMPLE then the random graph
 *     (i.e. G(n,p)), if it is \c IGRAPH_SPINCOMM_UPDATE then the
 *     configuration model is used. The configuration means that the
 *     baseline for the clustering is a random graph with the same
 *     degree distribution as the input graph.
 * \param gamma Real number. The gamma parameter of the algorithm,
 *     acting as a resolution parameter. Smaller values typically lead to
 *     larger clusters, larger values typically lead to smaller clusters.
 * \param implementation Constant, chooses between the two
 *     implementations of the spin-glass algorithm that are included
 *     in igraph. \c IGRAPH_SPINCOMM_IMP_ORIG selects the original
 *     implementation, this is faster, \c IGRAPH_SPINCOMM_INP_NEG selects
 *     an implementation that allows negative edge weights.
 * \param gamma_minus Real number. Parameter for the \c IGRAPH_SPINCOMM_IMP_NEG
 *     implementation. This acts as a resolution parameter for the negative part
 *     of the network. Smaller values of \p gamma_minus leads to fewer negative
 *     edges within clusters. If this argument is set to zero, the algorithm
 *     reduces to a graph coloring algorithm when all edges have negative
 *     weights, using the number of spins as the number of colors.
 * \return Error code.
 *
 * \sa \ref igraph_community_spinglass_single() for calculating the community
 * of a single vertex.
 *
 * Time complexity: TODO.
 *
 */

igraph_error_t igraph_community_spinglass(const igraph_t *graph,
                               const igraph_vector_t *weights,
                               igraph_real_t *modularity,
                               igraph_real_t *temperature,
                               igraph_vector_int_t *membership,
                               igraph_vector_int_t *csize,
                               igraph_integer_t spins,
                               igraph_bool_t parupdate,
                               igraph_real_t starttemp,
                               igraph_real_t stoptemp,
                               igraph_real_t coolfact,
                               igraph_spincomm_update_t update_rule,
                               igraph_real_t gamma,
                               igraph_spinglass_implementation_t implementation,
                               igraph_real_t gamma_minus) {

    IGRAPH_HANDLE_EXCEPTIONS(
        switch (implementation) {
        case IGRAPH_SPINCOMM_IMP_ORIG:
            return igraph_i_community_spinglass_orig(graph, weights, modularity,
                    temperature, membership, csize,
                    spins, parupdate, starttemp,
                    stoptemp, coolfact, update_rule,
                    gamma);
            break;
        case IGRAPH_SPINCOMM_IMP_NEG:
            return igraph_i_community_spinglass_negative(graph, weights, modularity,
                    temperature, membership, csize,
                    spins, parupdate, starttemp,
                    stoptemp, coolfact,
                    update_rule, gamma,
                    gamma_minus);
            break;
        default:
            IGRAPH_ERROR("Unknown implementation in spinglass community detection.",
                         IGRAPH_EINVAL);
        }
    );
}

static igraph_error_t igraph_i_community_spinglass_orig(
        const igraph_t *graph,
        const igraph_vector_t *weights,
        igraph_real_t *modularity,
        igraph_real_t *temperature,
        igraph_vector_int_t *membership,
        igraph_vector_int_t *csize,
        igraph_integer_t spins,
        igraph_bool_t parupdate,
        igraph_real_t starttemp,
        igraph_real_t stoptemp,
        igraph_real_t coolfact,
        igraph_spincomm_update_t update_rule,
        igraph_real_t gamma) {

    igraph_integer_t no_of_nodes = igraph_vcount(graph);
    igraph_integer_t changes, runs;
    igraph_bool_t use_weights = false;
    bool zeroT;
    double kT, acc, prob;

    /* Check arguments */

    if (spins < 2) {
        IGRAPH_ERROR("Number of spins must be at least 2.", IGRAPH_EINVAL);
    }
    if (update_rule != IGRAPH_SPINCOMM_UPDATE_SIMPLE &&
        update_rule != IGRAPH_SPINCOMM_UPDATE_CONFIG) {
        IGRAPH_ERROR("Invalid update rule for spinglass community detection.", IGRAPH_EINVAL);
    }
    if (weights) {
        if (igraph_vector_size(weights) != igraph_ecount(graph)) {
            IGRAPH_ERROR("Invalid weight vector length.", IGRAPH_EINVAL);
        }
        use_weights = true;
        if (igraph_vector_size(weights) > 0 && igraph_vector_min(weights) < 0) {
            IGRAPH_ERROR(
                "Weights must not be negative when using the original implementation of spinglass communities. "
                "Select the implementation meant for negative weights.",
                IGRAPH_EINVAL);
        }
    }
    if (coolfact < 0 || coolfact >= 1.0) {
        IGRAPH_ERROR("Cooling factor must be positive and strictly smaller than 1.", IGRAPH_EINVAL);
    }
    if (gamma < 0.0) {
        IGRAPH_ERROR("Gamma value must not be negative.", IGRAPH_EINVAL);
    }
    if ( !(starttemp == 0 && stoptemp == 0) ) {
        if (! (starttemp > 0 && stoptemp > 0)) {
            IGRAPH_ERROR("Starting and stopping temperatures must be both positive or both zero.",
                         IGRAPH_EINVAL);
        }
        if (starttemp <= stoptemp) {
            IGRAPH_ERROR("The starting temperature must be larger than the stopping temperature.",
                         IGRAPH_EINVAL);
        }
    }

    /* The spinglass algorithm does not handle the trivial cases of the
       null and singleton graphs, so we catch them here. */
    if (no_of_nodes < 2) {
        if (membership) {
            IGRAPH_CHECK(igraph_vector_int_resize(membership, no_of_nodes));
            igraph_vector_int_null(membership);
        }
        if (modularity) {
            IGRAPH_CHECK(igraph_modularity(graph, membership, nullptr, 1, igraph_is_directed(graph), modularity));
        }
        if (temperature) {
            *temperature = stoptemp;
        }
        if (csize) {
            /* 0 clusters for 0 nodes, 1 cluster for 1 node */
            IGRAPH_CHECK(igraph_vector_int_resize(csize, no_of_nodes));
            igraph_vector_int_fill(csize, 1);
        }
        return IGRAPH_SUCCESS;
    }

    /* Check whether we have a single component */
    igraph_bool_t conn;
    IGRAPH_CHECK(igraph_is_connected(graph, &conn, IGRAPH_WEAK));
    if (!conn) {
        IGRAPH_ERROR("Cannot work with unconnected graph.", IGRAPH_EINVAL);
    }

    network net;

    /* Transform the igraph_t */
    IGRAPH_CHECK(igraph_i_read_network_spinglass(graph, weights,
                                       &net, use_weights));

    prob = 2.0 * net.sum_weights / double(net.node_list.Size())
           / double(net.node_list.Size() - 1);

    PottsModel pm(&net, spins, update_rule);

    /* initialize the random number generator */
    RNG_BEGIN();

    if ((stoptemp == 0.0) && (starttemp == 0.0)) {
        zeroT = true;
    } else {
        zeroT = false;
    }
    if (!zeroT) {
        kT = pm.FindStartTemp(gamma, prob, starttemp);
    } else {
        kT = stoptemp;
    }
    /* assign random initial configuration */
    pm.assign_initial_conf(-1);
    runs = 0;
    changes = 1;

    while (changes > 0 && (kT / stoptemp > 1.0 || (zeroT && runs < 150))) {

        IGRAPH_ALLOW_INTERRUPTION();

        runs++;
        if (!zeroT) {
            kT *= coolfact;
            if (parupdate) {
                changes = pm.HeatBathParallelLookup(gamma, prob, kT, 50);
            } else {
                acc = pm.HeatBathLookup(gamma, prob, kT, 50);
                if (acc < (1.0 - 1.0 / double(spins)) * 0.01) {
                    changes = 0;
                } else {
                    changes = 1;
                }
            }
        } else {
            if (parupdate) {
                changes = pm.HeatBathParallelLookupZeroTemp(gamma, prob, 50);
            } else {
                acc = pm.HeatBathLookupZeroTemp(gamma, prob, 50);
                /* less than 1 percent acceptance ratio */
                if (acc < (1.0 - 1.0 / double(spins)) * 0.01) {
                    changes = 0;
                } else {
                    changes = 1;
                }
            }
        }
    } /* while loop */

    pm.WriteClusters(modularity, temperature, csize, membership, kT, gamma);

    RNG_END();

    return IGRAPH_SUCCESS;
}

/**
 * \function igraph_community_spinglass_single
 * \brief Community of a single node based on statistical mechanics.
 *
 * This function implements the community structure detection
 * algorithm proposed by Joerg Reichardt and Stefan Bornholdt. It is
 * described in their paper: Statistical Mechanics of
 * Community Detection, http://arxiv.org/abs/cond-mat/0603718 .
 *
 * </para><para>
 * This function calculates the community of a single vertex without
 * calculating all the communities in the graph.
 *
 * \param graph The input graph, it may be directed but the direction
 *    of the edges is not used in the algorithm.
 * \param weights Pointer to a vector with the weights of the edges.
 *    Alternatively \c NULL can be supplied to have the same weight
 *    for every edge.
 * \param vertex The vertex ID of the vertex of which ths community is
 *    calculated.
 * \param community Pointer to an initialized vector, the result, the
 *    IDs of the vertices in the community of the input vertex will be
 *    stored here. The vector will be resized as needed.
 * \param cohesion Pointer to a real variable, if not \c NULL the
 *     cohesion index of the community will be stored here.
 * \param adhesion Pointer to a real variable, if not \c NULL the
 *     adhesion index of the community will be stored here.
 * \param inner_links Pointer to an integer, if not \c NULL the
 *     number of edges within the community is stored here.
 * \param outer_links Pointer to an integer, if not \c NULL the
 *     number of edges between the community and the rest of the graph
 *     will be stored here.
 * \param spins The number of spins to use, this can be higher than
 *    the actual number of clusters in the network, in which case some
 *    clusters will contain zero vertices.
 * \param update_rule The type of the update rule. Possible values: \c
 *     IGRAPH_SPINCOMM_UPDATE_SIMPLE and \c
 *     IGRAPH_SPINCOMM_UPDATE_CONFIG. Basically this parameter defined
 *     the null model based on which the actual clustering is done. If
 *     this is \c IGRAPH_SPINCOMM_UPDATE_SIMPLE then the random graph
 *     (ie. G(n,p)), if it is \c IGRAPH_SPINCOMM_UPDATE then the
 *     configuration model is used. The configuration means that the
 *     baseline for the clustering is a random graph with the same
 *     degree distribution as the input graph.
 * \param gamma Real number. The gamma parameter of the
 *     algorithm. This defined the weight of the missing and existing
 *     links in the quality function for the clustering. The default
 *     value in the original code was 1.0, which is equal weight to
 *     missing and existing edges. Smaller values make the existing
 *     links contibute more to the energy function which is minimized
 *     in the algorithm. Bigger values make the missing links more
 *     important. (If my understanding is correct.)
 * \return Error code.
 *
 * \sa igraph_community_spinglass() for the traditional version of the
 * algorithm.
 *
 * Time complexity: TODO.
 */

igraph_error_t igraph_community_spinglass_single(const igraph_t *graph,
                                      const igraph_vector_t *weights,
                                      igraph_integer_t vertex,
                                      igraph_vector_int_t *community,
                                      igraph_real_t *cohesion,
                                      igraph_real_t *adhesion,
                                      igraph_integer_t *inner_links,
                                      igraph_integer_t *outer_links,
                                      igraph_integer_t spins,
                                      igraph_spincomm_update_t update_rule,
                                      igraph_real_t gamma) {
    IGRAPH_HANDLE_EXCEPTIONS(
        igraph_bool_t use_weights = false;
        char startnode[SPINGLASS_MAX_NAME_LEN];

        /* Check arguments */

        if (spins < 2) {
            IGRAPH_ERROR("Number of spins must be at least 2", IGRAPH_EINVAL);
        }
        if (update_rule != IGRAPH_SPINCOMM_UPDATE_SIMPLE &&
            update_rule != IGRAPH_SPINCOMM_UPDATE_CONFIG) {
            IGRAPH_ERROR("Invalid update rule", IGRAPH_EINVAL);
        }
        if (weights) {
            if (igraph_vector_size(weights) != igraph_ecount(graph)) {
                IGRAPH_ERROR("Invalid weight vector length", IGRAPH_EINVAL);
            }
            use_weights = 1;
        }
        if (gamma < 0.0) {
            IGRAPH_ERROR("Invalid gamme value", IGRAPH_EINVAL);
        }
        if (vertex < 0 || vertex > igraph_vcount(graph)) {
            IGRAPH_ERROR("Invalid vertex ID", IGRAPH_EINVAL);
        }

        /* Check whether we have a single component */
        igraph_bool_t conn;
        IGRAPH_CHECK(igraph_is_connected(graph, &conn, IGRAPH_WEAK));
        if (!conn) {
            IGRAPH_ERROR("Cannot work with unconnected graph", IGRAPH_EINVAL);
        }

        network net;

        /* Transform the igraph_t */
        IGRAPH_CHECK(igraph_i_read_network_spinglass(graph, weights,
                                           &net, use_weights));

        PottsModel pm(&net, spins, update_rule);

        /* initialize the random number generator */
        RNG_BEGIN();

        /* to be expected, if we want to find the community around a particular node*/
        /* the initial conf is needed, because otherwise,
           the degree of the nodes is not in the weight property, stupid!!! */
        pm.assign_initial_conf(-1);
        snprintf(startnode, sizeof(startnode) / sizeof(startnode[0]), "%" IGRAPH_PRId "", vertex + 1);
        pm.FindCommunityFromStart(gamma, startnode, community,
                                   cohesion, adhesion, inner_links, outer_links);

        RNG_END();
    );

    return IGRAPH_SUCCESS;
}

static igraph_error_t igraph_i_community_spinglass_negative(
        const igraph_t *graph,
        const igraph_vector_t *weights,
        igraph_real_t *modularity,
        igraph_real_t *temperature,
        igraph_vector_int_t *membership,
        igraph_vector_int_t *csize,
        igraph_integer_t spins,
        igraph_bool_t parupdate,
        igraph_real_t starttemp,
        igraph_real_t stoptemp,
        igraph_real_t coolfact,
        igraph_spincomm_update_t update_rule,
        igraph_real_t gamma,
        igraph_real_t gamma_minus) {

    igraph_integer_t no_of_nodes = igraph_vcount(graph);
    igraph_integer_t runs;
    igraph_bool_t use_weights = false;
    bool zeroT;
    double kT, acc;
    igraph_real_t d_n;
    igraph_real_t d_p;

    /* Check arguments */

    if (parupdate) {
        IGRAPH_ERROR("Parallel spin update not implemented with negative weights.",
                     IGRAPH_UNIMPLEMENTED);
    }

    if (spins < 2) {
        IGRAPH_ERROR("Number of spins must be at least 2.", IGRAPH_EINVAL);
    }
    if (update_rule != IGRAPH_SPINCOMM_UPDATE_SIMPLE &&
        update_rule != IGRAPH_SPINCOMM_UPDATE_CONFIG) {
        IGRAPH_ERROR("Invalid update rule for spinglass community detection.", IGRAPH_EINVAL);
    }
    if (weights) {
        if (igraph_vector_size(weights) != igraph_ecount(graph)) {
            IGRAPH_ERROR("Invalid weight vector length.", IGRAPH_EINVAL);
        }
        use_weights = true;
    }
    if (coolfact < 0 || coolfact >= 1.0) {
        IGRAPH_ERROR("Cooling factor must be positive and strictly smaller than 1.", IGRAPH_EINVAL);
    }
    if (gamma < 0.0) {
        IGRAPH_ERROR("Gamma value must not be negative.", IGRAPH_EINVAL);
    }
    if ( !(starttemp == 0 && stoptemp == 0) ) {
        if (! (starttemp > 0 && stoptemp > 0)) {
            IGRAPH_ERROR("Starting and stopping temperatures must be both positive or both zero.",
                         IGRAPH_EINVAL);
        }
        if (starttemp <= stoptemp) {
            IGRAPH_ERROR("The starting temperature must be larger than the stopping temperature.",
                         IGRAPH_EINVAL);
        }
    }

    /* The spinglass algorithm does not handle the trivial cases of the
       null and singleton graphs, so we catch them here. */
    if (no_of_nodes < 2) {
        if (membership) {
            IGRAPH_CHECK(igraph_vector_int_resize(membership, no_of_nodes));
            igraph_vector_int_null(membership);
        }
        if (modularity) {
            IGRAPH_CHECK(igraph_modularity(graph, membership, nullptr, 1, igraph_is_directed(graph), modularity));
        }
        if (temperature) {
            *temperature = stoptemp;
        }
        if (csize) {
            /* 0 clusters for 0 nodes, 1 cluster for 1 node */
            IGRAPH_CHECK(igraph_vector_int_resize(csize, no_of_nodes));
            igraph_vector_int_fill(csize, 1);
        }
        return IGRAPH_SUCCESS;
    }

    /* Check whether we have a single component */
    igraph_bool_t conn;
    IGRAPH_CHECK(igraph_is_connected(graph, &conn, IGRAPH_WEAK));
    if (!conn) {
        IGRAPH_ERROR("Cannot work with unconnected graph.", IGRAPH_EINVAL);
    }

    if (weights && igraph_vector_size(weights) > 0) {
        igraph_vector_minmax(weights, &d_n, &d_p);
    } else {
        d_n = d_p = 1;
    }

    if (d_n > 0) {
        d_n = 0;
    }
    if (d_p < 0) {
        d_p = 0;
    }
    d_n = -d_n;

    network net;

    /* Transform the igraph_t */
    IGRAPH_CHECK(igraph_i_read_network_spinglass(graph, weights,
                                       &net, use_weights));

    bool directed = igraph_is_directed(graph);

    PottsModelN pm(&net, spins, directed);

    /* initialize the random number generator */
    RNG_BEGIN();

    if ((stoptemp == 0.0) && (starttemp == 0.0)) {
        zeroT = true;
    } else {
        zeroT = false;
    }

    //Begin at a high enough temperature
    kT = pm.FindStartTemp(gamma, gamma_minus, starttemp);

    /* assign random initial configuration */
    pm.assign_initial_conf(true);

    runs = 0;
    while (kT / stoptemp > 1.0 || (zeroT && runs < 150)) {
        IGRAPH_ALLOW_INTERRUPTION();

        runs++;
        kT = kT * coolfact;
        acc = pm.HeatBathLookup(gamma, gamma_minus, kT, 50);
        if (acc < (1.0 - 1.0 / double(spins)) * 0.001) {
            break;
        }
    } /* while loop */

    /* These are needed, otherwise 'modularity' is not calculated */
    igraph_matrix_t adhesion, normalized_adhesion;
    igraph_real_t polarization;
    IGRAPH_MATRIX_INIT_FINALLY(&adhesion, 0, 0);
    IGRAPH_MATRIX_INIT_FINALLY(&normalized_adhesion, 0, 0);
    pm.WriteClusters(modularity, temperature, csize, membership,
                      &adhesion, &normalized_adhesion, &polarization,
                      kT, d_p, d_n);
    igraph_matrix_destroy(&normalized_adhesion);
    igraph_matrix_destroy(&adhesion);
    IGRAPH_FINALLY_CLEAN(2);

    RNG_END();

    return IGRAPH_SUCCESS;
}
