/*
 * Copyright (C) 2017 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "Grid.h"
#include "GridTrackSize.h"
#include "LayoutUnit.h"

namespace WebCore {

static const int infinity = -1;

enum SizingOperation { TrackSizing, IntrinsicSizeComputation };

enum TrackSizeComputationPhase {
    ResolveIntrinsicMinimums,
    ResolveContentBasedMinimums,
    ResolveMaxContentMinimums,
    ResolveIntrinsicMaximums,
    ResolveMaxContentMaximums,
    MaximizeTracks,
};

class GridTrackSizingAlgorithmStrategy;

class GridTrack {
public:
    GridTrack() { }

    const LayoutUnit& baseSize() const;
    void setBaseSize(LayoutUnit);

    const LayoutUnit& growthLimit() const;
    void setGrowthLimit(LayoutUnit);

    bool infiniteGrowthPotential() const { return growthLimitIsInfinite() || m_infinitelyGrowable; }
    const LayoutUnit& growthLimitIfNotInfinite() const;

    const LayoutUnit& plannedSize() const { return m_plannedSize; }
    void setPlannedSize(LayoutUnit plannedSize) { m_plannedSize = plannedSize; }

    const LayoutUnit& tempSize() const { return m_tempSize; }
    void setTempSize(const LayoutUnit&);
    void growTempSize(const LayoutUnit&);

    bool infinitelyGrowable() const { return m_infinitelyGrowable; }
    void setInfinitelyGrowable(bool infinitelyGrowable) { m_infinitelyGrowable = infinitelyGrowable; }

    void setGrowthLimitCap(std::optional<LayoutUnit>);
    std::optional<LayoutUnit> growthLimitCap() const { return m_growthLimitCap; }

private:
    bool growthLimitIsInfinite() const { return m_growthLimit == infinity; }
    bool isGrowthLimitBiggerThanBaseSize() const { return growthLimitIsInfinite() || m_growthLimit >= m_baseSize; }

    void ensureGrowthLimitIsBiggerThanBaseSize();

    LayoutUnit m_baseSize { 0 };
    LayoutUnit m_growthLimit { 0 };
    LayoutUnit m_plannedSize { 0 };
    LayoutUnit m_tempSize { 0 };
    std::optional<LayoutUnit> m_growthLimitCap;
    bool m_infinitelyGrowable { false };
};

class GridTrackSizingAlgorithm final {
    friend class GridTrackSizingAlgorithmStrategy;

public:
    GridTrackSizingAlgorithm(const RenderGrid* renderGrid, Grid& grid)
        : m_grid(grid)
        , m_renderGrid(renderGrid)
        , m_sizingState(ColumnSizingFirstIteration)
    {
    }

    void setup(GridTrackSizingDirection, unsigned numTracks, SizingOperation, std::optional<LayoutUnit> availableSpace, std::optional<LayoutUnit> freeSpace);
    void run();
    void reset();

    // Required by RenderGrid. Try to minimize the exposed surface.
    const Grid& grid() const { return m_grid; }
    GridTrackSize gridTrackSize(GridTrackSizingDirection, unsigned translatedIndex) const;

    LayoutUnit minContentSize() const { return m_minContentSize; };
    LayoutUnit maxContentSize() const { return m_maxContentSize; };

    Vector<GridTrack>& tracks(GridTrackSizingDirection direction) { return direction == ForColumns ? m_columns : m_rows; }
    const Vector<GridTrack>& tracks(GridTrackSizingDirection direction) const { return direction == ForColumns ? m_columns : m_rows; }

    std::optional<LayoutUnit> freeSpace(GridTrackSizingDirection direction) const { return direction == ForColumns ? m_freeSpaceColumns : m_freeSpaceRows; }
    void setFreeSpace(GridTrackSizingDirection, std::optional<LayoutUnit>);

    std::optional<LayoutUnit> availableSpace(GridTrackSizingDirection direction) const { return direction == ForColumns ? m_availableSpaceColumns : m_availableSpaceRows; }
    void setAvailableSpace(GridTrackSizingDirection, std::optional<LayoutUnit>);

#ifndef NDEBUG
    bool tracksAreWiderThanMinTrackBreadth() const;
#endif

private:
    std::optional<LayoutUnit> availableSpace() const { return availableSpace(m_direction); }
    const GridTrackSize& rawGridTrackSize(GridTrackSizingDirection, unsigned translatedIndex) const;
    LayoutUnit assumedRowsSizeForOrthogonalChild(const RenderBox&) const;
    LayoutUnit computeTrackBasedSize() const;

    // Helper methods for step 1. initializeTrackSizes().
    LayoutUnit initialBaseSize(const GridTrackSize&) const;
    LayoutUnit initialGrowthLimit(const GridTrackSize&, LayoutUnit baseSize) const;

    // Helper methods for step 2. resolveIntrinsicTrackSizes().
    void sizeTrackToFitNonSpanningItem(const GridSpan&, RenderBox& gridItem, GridTrack&);
    bool spanningItemCrossesFlexibleSizedTracks(const GridSpan&) const;
    typedef struct GridItemsSpanGroupRange GridItemsSpanGroupRange;
    template <TrackSizeComputationPhase phase> void increaseSizesToAccommodateSpanningItems(const GridItemsSpanGroupRange& gridItemsWithSpan);
    LayoutUnit itemSizeForTrackSizeComputationPhase(TrackSizeComputationPhase, RenderBox&) const;
    template <TrackSizeComputationPhase phase> void distributeSpaceToTracks(Vector<GridTrack*>& tracks, Vector<GridTrack*>* growBeyondGrowthLimitsTracks, LayoutUnit& availableLogicalSpace) const;
    LayoutUnit gridAreaBreadthForChild(const RenderBox&, GridTrackSizingDirection) const;

    void computeGridContainerIntrinsicSizes();

    // Helper methods for step 4. Strech flexible tracks.
    typedef HashSet<unsigned, DefaultHash<unsigned>::Hash, WTF::UnsignedWithZeroKeyHashTraits<unsigned>> TrackIndexSet;
    double computeFlexFactorUnitSize(const Vector<GridTrack>& tracks, double flexFactorSum, LayoutUnit& leftOverSpace, const Vector<unsigned, 8>& flexibleTracksIndexes, std::unique_ptr<TrackIndexSet> tracksToTreatAsInflexible = nullptr) const;
    void computeFlexSizedTracksGrowth(double flexFraction, Vector<LayoutUnit>& increments, LayoutUnit& totalGrowth) const;
    double findFrUnitSize(const GridSpan& tracksSpan, LayoutUnit leftOverSpace) const;

    // Track sizing algorithm steps. Note that the "Maximize Tracks" step is done
    // entirely inside the strategies, that's why we don't need an additional
    // method at thise level.
    void initializeTrackSizes();
    void resolveIntrinsicTrackSizes();
    void stretchFlexibleTracks(std::optional<LayoutUnit> freeSpace);
    void stretchAutoTracks();

    // State machine.
    void advanceNextState();
    bool isValidTransition() const;

    bool m_needsSetup { true };
    std::optional<LayoutUnit> m_availableSpaceRows;
    std::optional<LayoutUnit> m_availableSpaceColumns;

    std::optional<LayoutUnit> m_freeSpaceColumns;
    std::optional<LayoutUnit> m_freeSpaceRows;

    // We need to keep both alive in order to properly size grids with orthogonal
    // writing modes.
    Vector<GridTrack> m_columns;
    Vector<GridTrack> m_rows;
    Vector<unsigned> m_contentSizedTracksIndex;
    Vector<unsigned> m_flexibleSizedTracksIndex;
    Vector<unsigned> m_autoSizedTracksForStretchIndex;

    GridTrackSizingDirection m_direction;
    SizingOperation m_sizingOperation;

    Grid& m_grid;

    const RenderGrid* m_renderGrid;
    std::unique_ptr<GridTrackSizingAlgorithmStrategy> m_strategy;

    // The track sizing algorithm is used for both layout and intrinsic size
    // computation. We're normally just interested in intrinsic inline sizes
    // (a.k.a widths in most of the cases) for the computeIntrinsicLogicalWidths()
    // computations. That's why we don't need to keep around different values for
    // rows/columns.
    LayoutUnit m_minContentSize;
    LayoutUnit m_maxContentSize;

    enum SizingState {
        ColumnSizingFirstIteration,
        RowSizingFirstIteration,
        ColumnSizingSecondIteration,
        RowSizingSecondIteration
    };
    SizingState m_sizingState;

    // This is a RAII class used to ensure that the track sizing algorithm is
    // executed as it is suppossed to be, i.e., first resolve columns and then
    // rows. Only if required a second iteration is run following the same order,
    // first columns and then rows.
    class StateMachine {
    public:
        StateMachine(GridTrackSizingAlgorithm&);
        ~StateMachine();

    private:
        GridTrackSizingAlgorithm& m_algorithm;
    };
};

class GridTrackSizingAlgorithmStrategy {
public:
    LayoutUnit minContentForChild(RenderBox&) const;
    LayoutUnit maxContentForChild(RenderBox&) const;
    LayoutUnit minSizeForChild(RenderBox&) const;

    virtual ~GridTrackSizingAlgorithmStrategy() { }

    virtual void maximizeTracks(Vector<GridTrack>&, std::optional<LayoutUnit>& freeSpace) = 0;
    virtual double findUsedFlexFraction(Vector<unsigned>& flexibleSizedTracksIndex, GridTrackSizingDirection, std::optional<LayoutUnit> initialFreeSpace) const = 0;
    virtual bool recomputeUsedFlexFractionIfNeeded(double& flexFraction, LayoutUnit& totalGrowth) const = 0;

protected:
    GridTrackSizingAlgorithmStrategy(GridTrackSizingAlgorithm& algorithm)
        : m_algorithm(algorithm) { }

    virtual LayoutUnit minLogicalWidthForChild(RenderBox&, Length childMinSize, GridTrackSizingDirection) const = 0;
    virtual void layoutGridItemForMinSizeComputation(RenderBox&, bool overrideSizeHasChanged) const = 0;

    LayoutUnit logicalHeightForChild(RenderBox&) const;
    bool updateOverrideContainingBlockContentSizeForChild(RenderBox&, GridTrackSizingDirection) const;

    // GridTrackSizingAlgorithm accessors for subclasses.
    LayoutUnit computeTrackBasedSize() const { return m_algorithm.computeTrackBasedSize(); }
    GridTrackSizingDirection direction() const { return m_algorithm.m_direction; }
    double findFrUnitSize(const GridSpan& tracksSpan, LayoutUnit leftOverSpace) const { return m_algorithm.findFrUnitSize(tracksSpan, leftOverSpace); }
    void distributeSpaceToTracks(Vector<GridTrack*>& tracks, LayoutUnit& availableLogicalSpace) const { m_algorithm.distributeSpaceToTracks<MaximizeTracks>(tracks, nullptr, availableLogicalSpace); }
    const RenderGrid* renderGrid() const { return m_algorithm.m_renderGrid; }
    std::optional<LayoutUnit> availableSpace() const { return m_algorithm.availableSpace(); }

    GridTrackSizingAlgorithm& m_algorithm;
};

} // namespace WebCore
