#include "Game/ScoreSystem.h"

namespace
{
// Index N = points for clearing N lines in one lock (classic guideline
// values). Index 0 is unused.
constexpr int kLineClearPoints[] = {0, 100, 300, 500, 800};
} // namespace

void ScoreSystem::registerLineClear(int linesCleared)
{
    if (linesCleared <= 0) {
        return;
    }

    const int clampedLines = linesCleared < 4 ? linesCleared : 4; // a single lock clears at most 4 lines
    m_score += kLineClearPoints[clampedLines];
    m_totalLinesCleared += linesCleared;
}

void ScoreSystem::reset()
{
    m_score = 0;
    m_totalLinesCleared = 0;
}
