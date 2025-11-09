#include "Chess.hpp"
using namespace Chess;
extern Networking::NetworkManager netMgr;

Board::Board() {
    resetBoard();
}

void Board::makeMove(Position from, Position to, std::vector<std::unique_ptr<Piece>>& takenPieces) {
    auto& piece = grid[from.x][from.y];
    auto& dest = grid[to.x][to.y];
	if (piece->getColor() != currentTurn) 
		return;

    if (gameState == GameState::PAUSED)
        return;

    std::vector<Move> legalMoves = piece->getLegalMoves(from);
	std::cout << "Legal moves for piece at (" << from.x << ", " << from.y << "):\n";
    for (const Move& move : legalMoves) {
        std::cout << "  To (" << move.to.x << ", " << move.to.y << ")\n";
    }
    if (legalMoves.size() == 0) {
		return;
	}
	Move usedMove;
	bool isLegal = false;
    for(const Move& move : legalMoves) {
        if (move.to == to) {
			usedMove = move;
			isLegal = true;
            break;
        }
	}
    if (!isLegal) return;
    // Simulate the move
    auto capturedPiece = std::move(dest);
    dest = std::move(piece);
    piece = std::make_unique<EmptyPiece>(this);
    bool stillInCheck = isChecked(currentTurn);
    // Revert the move
    piece = std::move(dest);
    dest = std::move(capturedPiece);
    if (stillInCheck) {
        std::cout << "Move would leave king in check, illegal move.\n";
        return; // Move is illegal as it leaves king in check
	}
    std::cout << "Move made from (" << from.x << ", " << from.y << ") to ("
		<< to.x << ", " << to.y << ")\n";
    if(usedMove.type == MoveType::CAPTURE)
		takenPieces.push_back(std::move(dest));
    dest = std::move(piece);
    // Leave an empty piece at the original square
    piece = std::make_unique<EmptyPiece>(this);
	currentTurn = (currentTurn == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;
	lastMove = usedMove;
	std::cout << "taken pieces count: " << takenPieces.size() << "\n";
	updateGameState();
}

bool Board::isSquareAttacked(Position pos, PieceColor attackerColor) const {
    // Iterate over all squares on the board
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            Piece* piece = getPiece(x,y);
            // Check if the piece belongs to the attacking color
            if (piece->getColor() == attackerColor) {
                Position from{x, y};
                std::vector<Move> moves = piece->getLegalMoves(from);
                // Check if any of the legal moves target the specified position
                for (const Move& move : moves) {
                    if (move.to == pos) {
                        return true; // The square is attacked
                    }
                }
            }
        }
    }
    return false; // No attacking pieces found
}

Position Board::kingPosition(PieceColor kingColor) const {
    for(int x = 0; x < 8; ++x) {
        for(int y = 0; y < 8; ++y) {
            Piece* piece = getPiece(x,y);
            if (piece->getType() == PieceType::KING && piece->getColor() == kingColor) {
                return Position{x,y};
            }
        }
	}
}
bool Board::isChecked(PieceColor kingColor) {
    bool isCheck = isSquareAttacked(kingPosition(kingColor), kingColor == PieceColor::WHITE ? PieceColor::BLACK : PieceColor::WHITE);
    return isCheck;
}

void Board::updateGameState() {
    bool inCheck = isChecked(currentTurn);
    bool hasLegalMoves = false;
    // Check for any legal moves for the current player
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            Piece* piece = getPiece(x, y);
            if (piece->getColor() == currentTurn) {
                Position from{x, y};
                std::vector<Move> moves = piece->getLegalMoves(from);
                for (const Move& move : moves) {
                    // Simulate the move
                    auto capturedPiece = std::move(grid[move.to.x][move.to.y]);
                    grid[move.to.x][move.to.y] = std::move(grid[from.x][from.y]);
                    grid[from.x][from.y] = std::make_unique<EmptyPiece>(this);
                    bool stillInCheck = isChecked(currentTurn);
                    // Revert the move
                    grid[from.x][from.y] = std::move(grid[move.to.x][move.to.y]);
                    grid[move.to.x][move.to.y] = std::move(capturedPiece);
                    if (!stillInCheck) {
                        hasLegalMoves = true;
                        break;
                    }
                }
            }
            if (hasLegalMoves) break;
        }
        if (hasLegalMoves) break;
    }
    if (inCheck && !hasLegalMoves) {
        gameState = GameState::CHECKMATE;
        gameOver();
    } else if (!inCheck && !hasLegalMoves) {
        gameState = GameState::STALEMATE;
        gameOver();
    } else if (inCheck) {
        gameState = GameState::CHECK;
    } else {
        gameState = GameState::ONGOING;
	}
    if(netMgr.isConnected()) {
		netMgr.sendData<GameInfo>({ lastMove, gameState, currentTurn });
	}
}

void Board::gameOver() {
    
}


void Board::resetBoard() {
    currentTurn = PieceColor::WHITE;
    // Fill everything with empty pieces first
    for (int x = 0; x < 8; ++x)
        for (int y = 0; y < 8; ++y)
            grid[x][y] = std::make_unique<EmptyPiece>(this);

    // Place pawns
    for (int x = 0; x < 8; ++x) {
        grid[x][1] = std::make_unique<Pawn>(PieceColor::WHITE, this);
        grid[x][6] = std::make_unique<Pawn>(PieceColor::BLACK, this);
    }

    // Place rooks
    grid[0][0] = std::make_unique<Rook>(PieceColor::WHITE, this);
    grid[7][0] = std::make_unique<Rook>(PieceColor::WHITE, this);
    grid[0][7] = std::make_unique<Rook>(PieceColor::BLACK, this);
    grid[7][7] = std::make_unique<Rook>(PieceColor::BLACK, this);

    // Knights
    grid[1][0] = std::make_unique<Knight>(PieceColor::WHITE, this);
    grid[6][0] = std::make_unique<Knight>(PieceColor::WHITE, this);
    grid[1][7] = std::make_unique<Knight>(PieceColor::BLACK, this);
    grid[6][7] = std::make_unique<Knight>(PieceColor::BLACK, this);

    // Bishops
    grid[2][0] = std::make_unique<Bishop>(PieceColor::WHITE, this);
    grid[5][0] = std::make_unique<Bishop>(PieceColor::WHITE, this);
    grid[2][7] = std::make_unique<Bishop>(PieceColor::BLACK, this);
    grid[5][7] = std::make_unique<Bishop>(PieceColor::BLACK, this);

    // Queens
    grid[3][0] = std::make_unique<Queen>(PieceColor::WHITE, this);
    grid[3][7] = std::make_unique<Queen>(PieceColor::BLACK, this);

    // Kings
    grid[4][0] = std::make_unique<King>(PieceColor::WHITE, this);
    grid[4][7] = std::make_unique<King>(PieceColor::BLACK, this);
}

// ---------------- Pawn ----------------
std::vector<Move> Pawn::getLegalMoves(const Position& from) const {
    std::vector<Move> moves;

    int dir = (color == PieceColor::WHITE) ? 1 : -1;

    // --- 1. One-Square Forward Move ---
    Position oneAhead(from.x, from.y + dir); // Move changes the Y-coordinate (row)

    if (OwningBoard->isInside(oneAhead.x, oneAhead.y) &&
        OwningBoard->getPiece(oneAhead.x, oneAhead.y)->getType() == PieceType::EMPTY)
    {
        moves.emplace_back(from, oneAhead);


        bool isStartingRank = (color == PieceColor::WHITE && from.y == 1) ||
            (color == PieceColor::BLACK && from.y == 6);

        if (isStartingRank) {
            Position twoAhead(from.x, from.y + 2 * dir);
            // Check that the two-ahead square is inside AND empty
            if (OwningBoard->isInside(twoAhead.x, twoAhead.y) &&
                OwningBoard->getPiece(twoAhead.x, twoAhead.y)->getType() == PieceType::EMPTY)
            {
                moves.emplace_back(from, twoAhead);
            }
        }
    }

    // --- 3. Diagonal Captures ---
    for (int dx : {-1, 1}) { // Check diagonal left (dx=-1) and diagonal right (dx=1)
        Position cap(from.x + dx, from.y + dir); // Diagonal move changes X (column) and Y (row)

        if (OwningBoard->isInside(cap.x, cap.y)) {
            Piece* target = OwningBoard->getPiece(cap.x, cap.y);

            // Legal capture condition: The square is occupied AND the piece is the opponent's color
            if (target->getType() != PieceType::EMPTY && target->getColor() != color) {
                moves.emplace_back(from, cap, MoveType::CAPTURE);
            }
        }
    }

    // Note: En passant and promotion are complex and not included here.

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
