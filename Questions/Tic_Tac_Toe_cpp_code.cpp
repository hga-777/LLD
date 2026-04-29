#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <optional>

// ─────────────────────────────────────────────
// Piece — what a player places on the board
// ─────────────────────────────────────────────
class Piece {
    char symbol_;
public:
    explicit Piece(char symbol) : symbol_(symbol) {}
    char getSymbol() const { return symbol_; }
    bool operator==(const Piece& o) const { return symbol_ == o.symbol_; }
};

// ─────────────────────────────────────────────
// Cell — one square on the board
// ─────────────────────────────────────────────
class Cell {
    int row_, col_;
    std::optional<Piece> piece_; // empty = unoccupied
public:
    Cell(int row, int col) : row_(row), col_(col) {}

    bool isOccupied() const { return piece_.has_value(); }

    void placePiece(const Piece& p) {
        if (isOccupied())
            throw std::runtime_error("Cell already occupied!");
        piece_ = p;
    }

    const std::optional<Piece>& getPiece() const { return piece_; }
    int getRow() const { return row_; }
    int getCol() const { return col_; }
};

// ─────────────────────────────────────────────
// Board — N×N grid of cells
// ─────────────────────────────────────────────
class Board {
    int size_;
    std::vector<std::vector<Cell>> grid_;

public:
    explicit Board(int size) : size_(size) {
        for (int r = 0; r < size_; ++r) {
            grid_.emplace_back();
            for (int c = 0; c < size_; ++c)
                grid_[r].emplace_back(r, c);
        }
    }

    int getSize() const { return size_; }

    Cell& getCell(int row, int col) {
        if (row < 0 || row >= size_ || col < 0 || col >= size_)
            throw std::out_of_range("Cell out of bounds");
        return grid_[row][col];
    }

    // Returns true if every cell is occupied (draw check)
    bool isFull() const {
        for (auto& row : grid_)
            for (auto& cell : row)
                if (!cell.isOccupied()) return false;
        return true;
    }

    void print() const {
        std::cout << "\n  ";
        for (int c = 0; c < size_; ++c)
            std::cout << c << " ";
        std::cout << "\n";

        for (int r = 0; r < size_; ++r) {
            std::cout << r << " ";
            for (int c = 0; c < size_; ++c) {
                auto& p = grid_[r][c].getPiece();
                std::cout << (p.has_value() ? p->getSymbol() : '.') << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
};

// ─────────────────────────────────────────────
// IPlayer — interface so AI can be added later
// Strategy pattern: human and AI are interchangeable
// ─────────────────────────────────────────────
class IPlayer {
public:
    virtual std::string getName()         const = 0;
    virtual Piece       getPiece()        const = 0;
    virtual std::pair<int,int> getMove(const Board& board) = 0;
    virtual ~IPlayer() = default;
};

// ─────────────────────────────────────────────
// HumanPlayer — reads move from stdin
// ─────────────────────────────────────────────
class HumanPlayer : public IPlayer {
    std::string name_;
    Piece       piece_;
public:
    HumanPlayer(std::string name, char symbol)
        : name_(std::move(name)), piece_(symbol) {}

    std::string getName()  const override { return name_; }
    Piece       getPiece() const override { return piece_; }

    std::pair<int,int> getMove(const Board& board) override {
        int row, col;
        std::cout << name_ << " [" << piece_.getSymbol() << "] "
                  << "Enter row col: ";
        std::cin >> row >> col;
        return {row, col};
    }
};

// ─────────────────────────────────────────────
// WinChecker — pure logic, no state
// Separate class so it's testable independently
// ─────────────────────────────────────────────
class WinChecker {
public:
    // Returns the winning piece if found, nullopt otherwise
    std::optional<Piece> check(const Board& board, int lastRow, int lastCol) {
        int N = board.getSize();

        auto sameAs = [&](int r, int c, const Piece& p) -> bool {
            const auto& cell = const_cast<Board&>(board).getCell(r, c);
            return cell.isOccupied() && cell.getPiece().value() == p;
        };

        const auto& lastPiece =
            const_cast<Board&>(board).getCell(lastRow, lastCol).getPiece();
        if (!lastPiece.has_value()) return std::nullopt;
        const Piece& p = lastPiece.value();

        // Check row
        bool win = true;
        for (int c = 0; c < N && win; ++c) win = sameAs(lastRow, c, p);
        if (win) return p;

        // Check col
        win = true;
        for (int r = 0; r < N && win; ++r) win = sameAs(r, lastCol, p);
        if (win) return p;

        // Check main diagonal (only if on it)
        if (lastRow == lastCol) {
            win = true;
            for (int i = 0; i < N && win; ++i) win = sameAs(i, i, p);
            if (win) return p;
        }

        // Check anti-diagonal (only if on it)
        if (lastRow + lastCol == N - 1) {
            win = true;
            for (int i = 0; i < N && win; ++i) win = sameAs(i, N-1-i, p);
            if (win) return p;
        }

        return std::nullopt;
    }
};

// ─────────────────────────────────────────────
// Game — orchestrates everything
// Knows players and board, delegates to WinChecker
// ─────────────────────────────────────────────
class Game {
    Board                                   board_;
    std::vector<std::shared_ptr<IPlayer>>   players_;
    WinChecker                              winChecker_;
    int                                     currentIdx_ = 0;

public:
    Game(int boardSize, std::vector<std::shared_ptr<IPlayer>> players)
        : board_(boardSize), players_(std::move(players)) {
        if (players_.size() < 2)
            throw std::invalid_argument("Need at least 2 players");
    }

    void run() {
        std::cout << "=== Tic-Tac-Toe ===\n";
        board_.print();

        while (true) {
            auto& player = players_[currentIdx_];

            // Get valid move — retry on bad input
            int row, col;
            while (true) {
                try {
                    auto [r, c] = player->getMove(board_);
                    row = r; col = c;
                    board_.getCell(row, col).placePiece(player->getPiece());
                    break;
                } catch (const std::exception& e) {
                    std::cout << "Invalid move: " << e.what()
                              << " — try again\n";
                }
            }

            board_.print();

            // Check win
            auto winner = winChecker_.check(board_, row, col);
            if (winner.has_value()) {
                std::cout << "🎉 " << player->getName() << " wins!\n";
                return;
            }

            // Check draw
            if (board_.isFull()) {
                std::cout << "🤝 It's a draw!\n";
                return;
            }

            // Next player — works for 2+ players
            currentIdx_ = (currentIdx_ + 1) % players_.size();
        }
    }
};

// ─────────────────────────────────────────────
// Driver
// ─────────────────────────────────────────────
int main() {
    int N;
    std::cout << "Board size (e.g. 3): ";
    std::cin >> N;

    std::vector<std::shared_ptr<IPlayer>> players;
    players.push_back(std::make_shared<HumanPlayer>("Alice", 'X'));
    players.push_back(std::make_shared<HumanPlayer>("Bob",   'O'));

    Game game(N, std::move(players));
    game.run();
}