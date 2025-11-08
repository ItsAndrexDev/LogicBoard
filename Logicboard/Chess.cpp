#include "Chess.hpp"
using namespace Chess;

Board::Board() {
    resetBoard();
}

void Board::makeMove(Position from, Position to) {
    auto& piece = grid[from.x][from.y];
    auto& dest = grid[to.x][to.y];

    // Capturing replaces the unique_ptr, old one is automatically deleted
    dest = std::move(piece);

    // Leave an empty piece at the original square
    piece = std::make_unique<EmptyPiece>(this);
}

void Board::resetBoard() {
    // Fill everything with empty pieces first
    for (int x = 0; x < 8; ++x)
        for (int y = 0; y < 8; ++y)
            grid[x][y] = std::make_unique<EmptyPiece>(this);

    // Place pawns
    for (int x = 0; x < 8; ++x) {
        grid[x][1] = std::make_unique<Pawn>(PieceColor::BLACK, this);
        grid[x][6] = std::make_unique<Pawn>(PieceColor::WHITE, this);
    }

    // Place rooks
    grid[0][0] = std::make_unique<Rook>(PieceColor::BLACK, this);
    grid[7][0] = std::make_unique<Rook>(PieceColor::BLACK, this);
    grid[0][7] = std::make_unique<Rook>(PieceColor::WHITE, this);
    grid[7][7] = std::make_unique<Rook>(PieceColor::WHITE, this);

    // Knights
    grid[1][0] = std::make_unique<Knight>(PieceColor::BLACK, this);
    grid[6][0] = std::make_unique<Knight>(PieceColor::BLACK, this);
    grid[1][7] = std::make_unique<Knight>(PieceColor::WHITE, this);
    grid[6][7] = std::make_unique<Knight>(PieceColor::WHITE, this);

    // Bishops
    grid[2][0] = std::make_unique<Bishop>(PieceColor::BLACK, this);
    grid[5][0] = std::make_unique<Bishop>(PieceColor::BLACK, this);
    grid[2][7] = std::make_unique<Bishop>(PieceColor::WHITE, this);
    grid[5][7] = std::make_unique<Bishop>(PieceColor::WHITE, this);

    // Queens
    grid[3][0] = std::make_unique<Queen>(PieceColor::BLACK, this);
    grid[3][7] = std::make_unique<Queen>(PieceColor::WHITE, this);

    // Kings
    grid[4][0] = std::make_unique<King>(PieceColor::BLACK, this);
    grid[4][7] = std::make_unique<King>(PieceColor::WHITE, this);
}

// ---------------- Pawn ----------------
std::vector<Move> Pawn::getLegalMoves(const Position& from) const {
    std::vector<Move> moves;
    int dir = (color == PieceColor::WHITE) ? -1 : 1;

    Position oneAhead(from.x + dir, from.y);
    if (OwningBoard->isInside(oneAhead.x, oneAhead.y) &&
        OwningBoard->getPiece(oneAhead.x, oneAhead.y)->getType() == PieceType::EMPTY)
        moves.emplace_back(from, oneAhead);

    // Captures
    for (int dy : {-1, 1}) {
        Position cap(from.x + dir, from.y + dy);
        if (OwningBoard->isInside(cap.x, cap.y)) {
            Piece* target = OwningBoard->getPiece(cap.x, cap.y);
            if (target->getColor() != color && target->getColor() != PieceColor::NONE)
                moves.emplace_back(from, cap, MoveType::CAPTURE);
        }
    }
    return moves;
}

// ---------------- Knight ----------------
std::vector<Move> Knight::getLegalMoves(const Position& from) const {
    std::vector<Move> moves;
    std::vector<std::pair<int, int>> offsets = {
        {2, 1}, {1, 2}, {-1, 2}, {-2, 1},
        {-2, -1}, {-1, -2}, {1, -2}, {2, -1}
    };

    for (auto [dx, dy] : offsets) {
        Position to(from.x + dx, from.y + dy);
        if (OwningBoard->isInside(to.x, to.y)) {
            Piece* target = OwningBoard->getPiece(to.x, to.y);
            if (target->getColor() != color)
                moves.emplace_back(from, to, target->getColor() == PieceColor::NONE ? MoveType::NORMAL : MoveType::CAPTURE);
        }
    }
    return moves;
}

// ---------------- Bishop ----------------
std::vector<Move> Bishop::getLegalMoves(const Position& from) const {
    std::vector<Move> moves;
    std::vector<std::pair<int, int>> dirs = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };

    for (auto [dx, dy] : dirs) {
        int x = from.x + dx, y = from.y + dy;
        while (OwningBoard->isInside(x, y)) {
            Piece* target = OwningBoard->getPiece(x, y);
            if (target->getType() == PieceType::EMPTY)
                moves.emplace_back(from, Position(x, y));
            else {
                if (target->getColor() != color)
                    moves.emplace_back(from, Position(x, y), MoveType::CAPTURE);
                break;
            }
            x += dx; y += dy;
        }
    }
    return moves;
}

// ---------------- Rook ----------------
std::vector<Move> Rook::getLegalMoves(const Position& from) const {
    std::vector<Move> moves;
    std::vector<std::pair<int, int>> dirs = { {1,0}, {-1,0}, {0,1}, {0,-1} };

    for (auto [dx, dy] : dirs) {
        int x = from.x + dx, y = from.y + dy;
        while (OwningBoard->isInside(x, y)) {
            Piece* target = OwningBoard->getPiece(x, y);
            if (target->getType() == PieceType::EMPTY)
                moves.emplace_back(from, Position(x, y));
            else {
                if (target->getColor() != color)
                    moves.emplace_back(from, Position(x, y), MoveType::CAPTURE);
                break;
            }
            x += dx; y += dy;
        }
    }
    return moves;
}

// ---------------- Queen ----------------
std::vector<Move> Queen::getLegalMoves(const Position& from) const {
    std::vector<Move> moves;
    Rook rook(color, OwningBoard);
    Bishop bishop(color, OwningBoard);

    auto rMoves = rook.getLegalMoves(from);
    auto bMoves = bishop.getLegalMoves(from);
    moves.insert(moves.end(), rMoves.begin(), rMoves.end());
    moves.insert(moves.end(), bMoves.begin(), bMoves.end());
    return moves;
}

// ---------------- King ----------------
std::vector<Move> King::getLegalMoves(const Position& from) const {
    std::vector<Move> moves;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx != 0 || dy != 0) {
                Position to(from.x + dx, from.y + dy);
                if (OwningBoard->isInside(to.x, to.y)) {
                    Piece* target = OwningBoard->getPiece(to.x, to.y);
                    if (target->getColor() != color)
                        moves.emplace_back(from, to, target->getColor() == PieceColor::NONE ? MoveType::NORMAL : MoveType::CAPTURE);
                }
            }
    return moves;
}
