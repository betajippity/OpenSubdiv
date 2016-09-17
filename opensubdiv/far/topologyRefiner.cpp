//
//   Copyright 2014 DreamWorks Animation LLC.
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//
#include "../far/topologyRefiner.h"
#include "../far/error.h"
#include "../vtr/fvarLevel.h"
#include "../vtr/sparseSelector.h"
#include "../vtr/quadRefinement.h"
#include "../vtr/triRefinement.h"

#include <cassert>
#include <cstdio>


namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

//
//  Relatively trivial construction/destruction -- the base level (level[0]) needs
//  to be explicitly initialized after construction and refinement then applied
//
TopologyRefiner::TopologyRefiner(Sdc::SchemeType schemeType, Sdc::Options schemeOptions) :
    _subdivType(schemeType),
    _subdivOptions(schemeOptions),
    _isUniform(true),
    _hasHoles(false),
    _maxLevel(0),
    _uniformOptions(0),
    _adaptiveOptions(0),
    _totalVertices(0),
    _totalEdges(0),
    _totalFaces(0),
    _totalFaceVertices(0),
    _maxValence(0) {

    //  Need to revisit allocation scheme here -- want to use smart-ptrs for these
    //  but will probably have to settle for explicit new/delete...
    _levels.reserve(10);
    _levels.push_back(new Vtr::internal::Level);
    _farLevels.reserve(10);
    assembleFarLevels();
}

TopologyRefiner::~TopologyRefiner() {

    for (int i=0; i<(int)_levels.size(); ++i) {
        delete _levels[i];
    }

    for (int i=0; i<(int)_refinements.size(); ++i) {
        delete _refinements[i];
    }
}

void
TopologyRefiner::Unrefine() {

    if (_levels.size()) {
        for (int i=1; i<(int)_levels.size(); ++i) {
            delete _levels[i];
        }
        _levels.resize(1);
        initializeInventory();
    }
    for (int i=0; i<(int)_refinements.size(); ++i) {
        delete _refinements[i];
    }
    _refinements.clear();

    assembleFarLevels();
}


//
//  Intializing and updating the component inventory:
//
void
TopologyRefiner::initializeInventory() {

    if (_levels.size()) {
        assert(_levels.size() == 1);

        Vtr::internal::Level const & baseLevel = *_levels[0];

        _totalVertices     = baseLevel.getNumVertices();
        _totalEdges        = baseLevel.getNumEdges();
        _totalFaces        = baseLevel.getNumFaces();
        _totalFaceVertices = baseLevel.getNumFaceVerticesTotal();

        _maxValence = baseLevel.getMaxValence();
    } else {
        _totalVertices     = 0;
        _totalEdges        = 0;
        _totalFaces        = 0;
        _totalFaceVertices = 0;

        _maxValence = 0;
    }
}

void
TopologyRefiner::updateInventory(Vtr::internal::Level const & newLevel) {

    _totalVertices     += newLevel.getNumVertices();
    _totalEdges        += newLevel.getNumEdges();
    _totalFaces        += newLevel.getNumFaces();
    _totalFaceVertices += newLevel.getNumFaceVerticesTotal();

    _maxValence = std::max(_maxValence, newLevel.getMaxValence());
}

void
TopologyRefiner::appendLevel(Vtr::internal::Level & newLevel) {

    _levels.push_back(&newLevel);

    updateInventory(newLevel); 
}

void
TopologyRefiner::appendRefinement(Vtr::internal::Refinement & newRefinement) {

    _refinements.push_back(&newRefinement);
}

void
TopologyRefiner::assembleFarLevels() {

    _farLevels.resize(_levels.size());

    _farLevels[0]._refToParent = 0;
    _farLevels[0]._level       = _levels[0];
    _farLevels[0]._refToChild  = 0;

    int nRefinements = (int)_refinements.size();
    if (nRefinements) {
        _farLevels[0]._refToChild = _refinements[0];

        for (int i = 1; i < nRefinements; ++i) {
            _farLevels[i]._refToParent = _refinements[i - 1];
            _farLevels[i]._level       = _levels[i];
            _farLevels[i]._refToChild  = _refinements[i];;
        }

        _farLevels[nRefinements]._refToParent = _refinements[nRefinements - 1];
        _farLevels[nRefinements]._level       = _levels[nRefinements];
        _farLevels[nRefinements]._refToChild  = 0;
    }
}


//
//  Accessors to the topology information:
//
int
TopologyRefiner::GetNumFVarValuesTotal(int channel) const {
    int sum = 0;
    for (int i = 0; i < (int)_levels.size(); ++i) {
        sum += _levels[i]->getNumFVarValues(channel);
    }
    return sum;
}


//
//  Main refinement method -- allocating and initializing levels and refinements:
//
void
TopologyRefiner::RefineUniform(UniformOptions options) {

    if (_levels[0]->getNumVertices() == 0) {
        Error(FAR_RUNTIME_ERROR,
            "Failure in TopologyRefiner::RefineUniform() -- base level is uninitialized.");
        return;
    }
    if (_refinements.size()) {
        Error(FAR_RUNTIME_ERROR,
            "Failure in TopologyRefiner::RefineUniform() -- previous refinements already applied.");
        return;
    }

    //
    //  Allocate the stack of levels and the refinements between them:
    //
    _uniformOptions = options;

    _isUniform = true;
    _maxLevel = options.refinementLevel;

    Sdc::Split splitType = Sdc::SchemeTypeTraits::GetTopologicalSplitType(_subdivType);

    //
    //  Initialize refinement options for Vtr -- adjusting full-topology for the last level:
    //
    Vtr::internal::Refinement::Options refineOptions;
    refineOptions._sparse         = false;
    refineOptions._faceVertsFirst = options.orderVerticesFromFacesFirst;

    for (int i = 1; i <= (int)options.refinementLevel; ++i) {
        refineOptions._minimalTopology =
            options.fullTopologyInLastLevel ? false : (i == (int)options.refinementLevel);

        Vtr::internal::Level& parentLevel = getLevel(i-1);
        Vtr::internal::Level& childLevel  = *(new Vtr::internal::Level);

        Vtr::internal::Refinement* refinement = 0;
        if (splitType == Sdc::SPLIT_TO_QUADS) {
            refinement = new Vtr::internal::QuadRefinement(parentLevel, childLevel, _subdivOptions);
        } else {
            refinement = new Vtr::internal::TriRefinement(parentLevel, childLevel, _subdivOptions);
        }
        refinement->refine(refineOptions);

        appendLevel(childLevel);
        appendRefinement(*refinement);
    }
    assembleFarLevels();
}

class TopologyRefiner::FeatureMask {
public:
    typedef unsigned int int_type;

    FeatureMask() { *((int_type*)this) = 0; }

    bool isEmpty() const { return *((int_type*)this) == 0; }

    int_type xordFeatures  : 1;
    int_type sharpFeatures : 1;
    int_type fvarFeatures  : 1;
};

void
TopologyRefiner::RefineAdaptive(AdaptiveOptions options) {

    if (_levels[0]->getNumVertices() == 0) {
        Error(FAR_RUNTIME_ERROR,
            "Failure in TopologyRefiner::RefineAdaptive() -- base level is uninitialized.");
        return;
    }
    if (_refinements.size()) {
        Error(FAR_RUNTIME_ERROR,
            "Failure in TopologyRefiner::RefineAdaptive() -- previous refinements already applied.");
        return;
    }
    if (_subdivType != Sdc::SCHEME_CATMARK) {
        Error(FAR_RUNTIME_ERROR,
            "Failure in TopologyRefiner::RefineAdaptive() -- currently only supported for Catmark scheme.");
        return;
    }

    //
    //  Initialize member and local variables from the adaptive options:
    //
    _isUniform = false;
    _adaptiveOptions = options;

    //int xordLevel  = options.xordIsolationLevel;
    int xordLevel  = options.isolationLevel;
    int sharpLevel = options.isolationLevel;

    int potentialMaxLevel = std::max(xordLevel, sharpLevel);

    FeatureMask adaptiveFeatureMask;

    //adaptiveFeatureMask.fvarFeatures = options.includeFVarChannels;
    adaptiveFeatureMask.fvarFeatures = false;
    if (adaptiveFeatureMask.fvarFeatures) {
        //  Ignore face-varying channels if none present are non-linear:
        adaptiveFeatureMask.fvarFeatures = false;
        for (int channel = 0; channel < _levels[0]->getNumFVarChannels(); ++channel) {
            adaptiveFeatureMask.fvarFeatures |= !_levels[0]->getFVarLevel(channel).isLinear();
        }
    }

    //
    //  Initialize refinement options for Vtr -- full topology is always generated in
    //  the last level as expected usage is for patch retrieval:
    //
    Vtr::internal::Refinement::Options refineOptions;

    refineOptions._sparse          = true;
    refineOptions._minimalTopology = false;
    refineOptions._faceVertsFirst  = options.orderVerticesFromFacesFirst;

    Sdc::Split splitType = Sdc::SchemeTypeTraits::GetTopologicalSplitType(_subdivType);

    for (int i = 1; i <= potentialMaxLevel; ++i) {

        Vtr::internal::Level& parentLevel     = getLevel(i-1);
        Vtr::internal::Level& childLevel      = *(new Vtr::internal::Level);

        Vtr::internal::Refinement* refinement = 0;
        if (splitType == Sdc::SPLIT_TO_QUADS) {
            refinement = new Vtr::internal::QuadRefinement(parentLevel, childLevel, _subdivOptions);
        } else {
            refinement = new Vtr::internal::TriRefinement(parentLevel, childLevel, _subdivOptions);
        }

        //
        //  Initialize a Selector to mark a sparse set of components for refinement.  If
        //  no features were selected, discard the new refinement and child level, and stop
        //  refinining further.  Otherwise, refine and append the new refinement and child.
        //
        Vtr::internal::SparseSelector selector(*refinement);

        adaptiveFeatureMask.xordFeatures  = (i <= xordLevel);
        adaptiveFeatureMask.sharpFeatures = (i <= sharpLevel);

        selectFeatureAdaptiveComponents(selector, adaptiveFeatureMask);
        if (selector.isSelectionEmpty()) {
            delete refinement;
            delete &childLevel;
            break;
        } else {
            refinement->refine(refineOptions);

            appendLevel(childLevel);
            appendRefinement(*refinement);
        }
    }
    _maxLevel = (unsigned int) _refinements.size();

    assembleFarLevels();
}

//
//   Method for selecting components for sparse refinement based on the feature-adaptive needs
//   of patch generation.
//
//   It assumes we have a freshly initialized SparseSelector (i.e. nothing already selected)
//   and will select all relevant topological features for inclusion in the subsequent sparse
//   refinement.
//
//   This was originally written specific to the quad-centric Catmark scheme and was since
//   generalized to support Loop given the enhanced tagging of components based on the scheme.
//   Any further enhancements here, e.g. new approaches for dealing with infinitely sharp
//   creases, should be aware of the intended generality.  Ultimately it may not be worth
//   trying to keep this general and we will be better off specializing it for each scheme.
//   The fact that this method is intimately tied to patch generation also begs for it to
//   become part of a class that encompasses both the feature adaptive tagging and the
//   identification of the intended patch that result from it.
//
void
TopologyRefiner::selectFeatureAdaptiveComponents(Vtr::internal::SparseSelector& selector,
                                                 FeatureMask const & featureMask) {

    if (featureMask.isEmpty()) return;

    Vtr::internal::Level const& level = selector.getRefinement().parent();

    bool considerXOrdinaryFaces = featureMask.xordFeatures;

    int  regularFaceSize           =  selector.getRefinement().getRegularFaceSize();
    bool considerSingleCreasePatch = _adaptiveOptions.useSingleCreasePatch && (regularFaceSize == 4);

    int  numFVarChannels      = level.getNumFVarChannels();
    bool considerFVarChannels = featureMask.fvarFeatures;

    //
    //  Inspect each face and the properties tagged at all of its corners:
    //
    for (Vtr::Index face = 0; face < level.getNumFaces(); ++face) {

        if (level.isFaceHole(face)) {
            continue;
        }

        Vtr::ConstIndexArray faceVerts = level.getFaceVertices(face);

        //
        //  Testing irregular faces is only necessary at level 0, and potentially warrants
        //  separating out as the caller can detect these:
        //
        if (faceVerts.size() != regularFaceSize) {
            //
            //  We need to also ensure that all adjacent faces to this are selected, so we
            //  select every face incident every vertex of the face.  This is the only place
            //  where other faces are selected as a side effect and somewhat undermines the
            //  whole intent of the per-face traversal.
            //
            Vtr::ConstIndexArray fVerts = level.getFaceVertices(face);
            for (int i = 0; i < fVerts.size(); ++i) {
                ConstIndexArray fVertFaces = level.getVertexFaces(fVerts[i]);
                for (int j = 0; j < fVertFaces.size(); ++j) {
                    selector.selectFace(fVertFaces[j]);
                }
            }
            continue;
        }

        //
        //  Combine the tags for all vertices of the face and quickly accept/reject based on
        //  the presence/absence of properties where we can (further inspection is likely to
        //  be necessary in some cases, particularly when we start trying to be clever about
        //  minimizing refinement for inf-sharp creases, etc.):
        //
        Vtr::internal::Level::VTag compFaceVTag = level.getFaceCompositeVTag(faceVerts);
        if (compFaceVTag._incomplete) {
            continue;
        }

        bool selectFace = false;
        if (compFaceVTag._rule == Sdc::Crease::RULE_SMOOTH) {
            //  If smooth, only isolate xordinary vertices if specified:
            selectFace = compFaceVTag._xordinary && considerXOrdinaryFaces;
        } else if (compFaceVTag._nonManifold) {
            //  Warrants further inspection in future -- isolate for now
            //    - will want to defer inf-sharp treatment to below
            selectFace = true;
        } else if (compFaceVTag._rule & Sdc::Crease::RULE_DART) {
            //  Any occurrence of a Dart vertex requires isolation
            selectFace = true;
        } else if (! (compFaceVTag._rule & Sdc::Crease::RULE_SMOOTH)) {
            //  None of the vertices is Smooth, so we have all vertices either Crease or Corner.
            //  Though some may be regular patches, this currently warrants isolation as we only
            //  support regular patches with one corner or one boundary, i.e. with one or more
            //  smooth interior vertices.
            selectFace = true;
        } else if (compFaceVTag._semiSharp || compFaceVTag._semiSharpEdges) {
            //  Any semi-sharp feature at or around the vertex warrants isolation -- unless we
            //  optimize for the single-crease patch, i.e. only edge sharpness of a constant value
            //  along the entire regular patch boundary (quickly exclude the Corner case first):
            if (considerSingleCreasePatch && ! (compFaceVTag._rule & Sdc::Crease::RULE_CORNER)) {
                selectFace = ! level.isSingleCreasePatch(face);
            } else {
                selectFace = true;
            }
        } else if (! compFaceVTag._boundary) {
            //  At this point we are left with a mix of smooth and inf-sharp features.  If not
            //  on a boundary, the interior inf-sharp features need isolation -- unless we are
            //  again optimizing for the single-crease patch, infinitely sharp in this case.
            //
            //  Note this case of detecting a single-crease patch, while similar to the above,
            //  is kept separate for the inf-sharp case:  a separate and much more efficient
            //  test can be made for the inf-sharp case, and there are other opportunities here
            //  to optimize for regular patches at infinitely sharp corners.
            if (considerSingleCreasePatch && ! (compFaceVTag._rule & Sdc::Crease::RULE_CORNER)) {
                selectFace = ! level.isSingleCreasePatch(face);
            } else {
                selectFace = true;
            }
        } else if (compFaceVTag._xordinary) {
            if (considerXOrdinaryFaces) {
                selectFace = true;
            } else if (compFaceVTag._rule & Sdc::Crease::RULE_CORNER) {
                selectFace = true;
            }
        } else if (! (compFaceVTag._rule & Sdc::Crease::RULE_CORNER)) {
            //  We are now left with boundary faces -- if no Corner vertex, we have a mix of both
            //  regular Smooth and Crease vertices on a boundary face, which can only be a regular
            //  boundary patch, so don't isolate.
            selectFace = false;
        } else {
            //  The last case with at least one Corner vertex and one Smooth (interior) vertex --
            //  distinguish the regular corner case from others:
            if (! compFaceVTag._corner) {
                //  We may consider interior sharp corners as regular in future, but for now we
                //  only accept a topological corner for the regular corner case:
                selectFace = true;
            } else if (level.getDepth() > 0) {
                //  A true corner at a subdivided level -- adjacent verts must be Crease and the
                //  opposite Smooth so we must have a regular corner:
                selectFace = false;
            } else {
                //  Make sure the adjacent boundary vertices were not sharpened, or equivalently,
                //  that only one corner is sharp:
                unsigned int infSharpCount = level.getVertexTag(faceVerts[0])._infSharp;
                for (int i = 1; i < faceVerts.size(); ++i) {
                    infSharpCount += level.getVertexTag(faceVerts[i])._infSharp;
                }
                selectFace = (infSharpCount != 1);
            }
        }

        //
        //  If still not selected, inspect the face-varying channels (when present) for similar
        //  irregular features requiring isolation:
        //
        if (! selectFace && considerFVarChannels) {
            for (int channel = 0; ! selectFace && (channel < numFVarChannels); ++channel) {

                //  No mismatch in topology -> no need to further isolate...
                if (level.doesFaceFVarTopologyMatch(face, channel)) continue;

                //
                //  Get the corner tags for the FVar topology, combine, then make similar inferences
                //  from the combined tags as done above for the vertex topology:
                //
                Vtr::internal::Level::VTag fvarVTags[4];
                level.getFaceVTags(face, fvarVTags, channel);

                Vtr::internal::Level::VTag compFVarVTag = Vtr::internal::Level::VTag::BitwiseOr(fvarVTags);

                if (compFVarVTag._xordinary) {
                    //  An xordinary boundary value always requires isolation:
                    selectFace = true;
                } else {
                    if (! (compFVarVTag._rule & Sdc::Crease::RULE_SMOOTH)) {
                        //  No Smooth corners so too many boundaries/corners -- need to isolate:
                        selectFace = true;
                    } else if (! (compFVarVTag._rule & Sdc::Crease::RULE_CORNER)) {
                        //  A mix of Smooth and Crease corners -- must be regular so don't isolate:
                        selectFace = false;
                    } else {
                        //  Since FVar boundaries can be "sharpened" based on the linear interpolation
                        //  rules, we again have to inspect more closely (as we did with the original
                        //  face) to ensure we have a regular corner and not a sharpened crease:
                        unsigned int boundaryCount = fvarVTags[0]._boundary,
                                     infSharpCount = fvarVTags[0]._infSharp;
                        for (int i = 1; i < faceVerts.size(); ++i) {
                            boundaryCount += fvarVTags[i]._boundary;
                            infSharpCount += fvarVTags[i]._infSharp;
                        }
                        selectFace = (boundaryCount != 3) || (infSharpCount != 1);

                        //  There is a possibility of a false positive at level 0 --  Smooth interior
                        //  vertex with adjacent Corner and two opposite boundary Crease vertices
                        //  (the topological corner tag catches this above).  Verify that the corner
                        //  vertex is opposite the smooth vertex (and consider doing this above)...
                        //
                        if (! selectFace && (level.getDepth() == 0)) {
                            if (fvarVTags[0]._infSharp && fvarVTags[2]._boundary) selectFace = true;
                            if (fvarVTags[1]._infSharp && fvarVTags[3]._boundary) selectFace = true;
                            if (fvarVTags[2]._infSharp && fvarVTags[0]._boundary) selectFace = true;
                            if (fvarVTags[3]._infSharp && fvarVTags[1]._boundary) selectFace = true;
                        }
                    }
                }
            }
        }

        //  Finally, select the face for further refinement:
        if (selectFace) {
            selector.selectFace(face);
        }
    }
}

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
