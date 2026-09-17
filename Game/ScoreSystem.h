#pragma once

// Tracks score and total lines cleared for one player. Point values follow
// the classic Tetris guideline table; no level multiplier yet (this is
// trivial to add later if a difficulty curve turns out to matter).
class ScoreSystem
{
public:
    void registerLineClear(int linesCleared);

    int score() const { return m_score; }
    int totalLinesCleared() const { return m_totalLinesCleared; }

    void reset();

private:
    int m_score = 0;
    int m_totalLinesCleared = 0;
};
