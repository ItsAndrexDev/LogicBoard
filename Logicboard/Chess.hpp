#pragma once
#include <vector>
#include <array>
#include <memory>
#include "Rendering/Renderer.hpp"

namespace Chess {

    enum class PieceType { EMPTY = 0, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
    enum class PieceColor { NONE = 0, WHITE, BLACK };
    enum class MoveType { NORMAL = 0, CAPTURE, CASTLE, PROMOTION, EN_PASSANT };
	enum class GameState { ONGOING = 0, PAUSED, CHECK, CHECKMATE, STALEMATE };
	class Board; // Forward declaration

    struct Position {
        int x;
        int y;
		Position() : x(-1), y(-1) {}
        Position(int x, int y) : x(x), y(y) {}
        bool operator==(const Position& other) const { return x == other.x && y == other.y; }
    };

    struct Move {
        Position from;
        Position to;
        MoveType type;
        PieceType promotion;

        Move() : from(), to(), type(MoveType::NORMAL), promotion(PieceType::EMPTY) {}
        Move(Position f, Position t, MoveType mt = MoveType::NORMAL, PieceType promo = PieceType::EMPTY)
            : from(f), to(t), type(mt), promotion(promo) {
        }
    };


    // ------------------- Base Piece -------------------
    class Piece {
    protected:
        PieceColor color;
        Board* OwningBoard;

    public:
        Piece(PieceColor c, Board* b) : color(c), OwningBoard(b) {}
        virtual ~Piece() = default;

        virtual std::vector<Move> getLegalMoves(const Position& from) const = 0;
        PieceColor getColor() const { return color; }
        virtual PieceType getType() const = 0;
		Renderer::VertexObject vertexObject;
        bool isVisible = true;
    };

    // ------------------- Derived Pieces -------------------
    class EmptyPiece : public Piece {
    public:
        EmptyPiece(Board* b = nullptr) : Piece(PieceColor::NONE, b) {}
        std::vector<Move> getLegalMoves(const Position&) const override { return {}; }
        PieceType getType() const override { return PieceType::EMPTY; }
    };

    class Pawn : public Piece {
    public:
        Pawn(PieceColor c, Board* b) : Piece(c, b) {}
        std::vector<Move> getLegalMoves(const Position& from) const override;
        PieceType getType() const override { return PieceType::PAWN; }
    };

    class Knight : public Piece {
    public:
        Knight(PieceColor c, Board* b) : Piece(c, b) {}
        std::vector<Move> getLegalMoves(const Position& from) const override;
        PieceType getType() const override { return PieceType::KNIGHT; }
    };

    class Bishop : public Piece {
    public:
        Bishop(PieceColor c, Board* b) : Piece(c, b) {}
        std::vector<Move> getLegalMoves(const Position& from) const override;
        PieceType getType() const override { return PieceType::BISHOP; }
    };

    class Rook : public Piece {
    public:
        Rook(PieceColor c, Board* b) : Piece(c, b) {}
        std::vector<Move> getLegalMoves(const Position& from) const override;
        PieceType getType() const override { return PieceType::ROOK; }
    };

    class Queen : public Piece {
    public:
        Queen(PieceColor c, Board* b) : Piece(c, b) {}
        std::vector<Move> getLegalMoves(const Position& from) const override;
        PieceType getType() const override { return PieceType::QUEEN; }
    };

    class King : public Piece {
    public:
        King(PieceColor c, Board* b) : Piece(c, b) {}
        std::vector<Move> getLegalMoves(const Position& from) const override;
        PieceType getType() const override { return PieceType::KING; }
    };

    // ------------------- Board -------------------
    class Board {
    public:
        std::array<std::array<std::unique_ptr<Piece>, 8>, 8> grid;

        Board();
        ~Board() = default;


        bool isInside(int x, int y) const { return x >= 0 && x < 8 && y >= 0 && y < 8; }
        Piece* getPiece(int x, int y) const { return grid[x][y].get(); }
        void makeMove(Position from, Position to, std::vector<std::unique_ptr<Piece>>& takenPieces);
        bool isSquareAttacked(Position pos, PieceColor attackerColor) const;
		Position kingPosition(PieceColor kingColor) const;
		bool isChecked(PieceColor kingColor);
        void resetBoard();
		void updateGameState();
        void gameOver();

		PieceColor currentTurn = PieceColor::WHITE;
		GameState gameState = GameState::PAUSED;
    };

} // namespace Chess