#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cctype>
#include <cmath>
#include <optional>

// Piece values for AI evaluation
const std::map<char, int> PIECE_VALUES = {
    {'P', 100}, {'N', 320}, {'B', 330}, {'R', 500}, {'Q', 900}, {'K', 20000},
    {'p', -100}, {'n', -320}, {'b', -330}, {'r', -500}, {'q', -900}, {'k', -20000}
};

// Position bonuses for pieces
const int PAWN_TABLE[8][8] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {5,  5, 10, 25, 25, 10,  5,  5},
    {0,  0,  0, 20, 20,  0,  0,  0},
    {5, -5,-10,  0,  0,-10, -5,  5},
    {5, 10, 10,-20,-20, 10, 10,  5},
    {0,  0,  0,  0,  0,  0,  0,  0}
};

const int KNIGHT_TABLE[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

const int BISHOP_TABLE[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5, 10, 10,  5,  0,-10},
    {-10,  5,  5, 10, 10,  5,  5,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};

const int KING_TABLE[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    { 20, 20,  0,  0,  0,  0, 20, 20},
    { 20, 30, 10,  0,  0, 10, 30, 20}
};

struct Position {
    int row, col;
    Position(int r = -1, int c = -1) : row(r), col(c) {}
    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
    bool isValid() const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }
};

struct Move {
    int fromRow, fromCol, toRow, toCol;
    int score;
    Move(int fr = 0, int fc = 0, int tr = 0, int tc = 0, int s = 0)
        : fromRow(fr), fromCol(fc), toRow(tr), toCol(tc), score(s) {}
};

class Chess {
private:
    std::vector<std::vector<char>> board;
    std::string currentPlayer;
    std::vector<std::string> moveHistory;
    std::map<std::string, std::vector<char>> capturedPieces;
    std::string winner;
    int difficultyAI1;
    int difficultyAI2;
    
    Position whiteKingPos;
    Position blackKingPos;
    bool whiteCanCastleKingside;
    bool whiteCanCastleQueenside;
    bool blackCanCastleKingside;
    bool blackCanCastleQueenside;
    Position enPassantTarget;
    
    int whiteWins;
    int blackWins;
    int draws;
    int totalGames;
    
    std::mt19937 rng;

public:
    Chess() : currentPlayer("white"),
              difficultyAI1(2),
              difficultyAI2(2),
              whiteKingPos(7, 4),
              blackKingPos(0, 4),
              whiteCanCastleKingside(true),
              whiteCanCastleQueenside(true),
              blackCanCastleKingside(true),
              blackCanCastleQueenside(true),
              enPassantTarget(-1, -1),
              whiteWins(0),
              blackWins(0),
              draws(0),
              totalGames(0),
              rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
        initBoard();
        capturedPieces["white"] = {};
        capturedPieces["black"] = {};
    }
    
    void initBoard() {
        board = std::vector<std::vector<char>>(8, std::vector<char>(8, ' '));
        
        // Black pieces (top)
        board[0] = {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'};
        board[1] = {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'};
        
        // White pieces (bottom)
        board[6] = {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'};
        board[7] = {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'};
    }
    
    void displayBoard() {
        // Clear screen
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
        
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "   CHESS - " << currentPlayer << "'S TURN\n";
        std::cout << std::string(50, '=') << "\n";
        
        // Simple ASCII chess pieces (no Unicode)
        std::map<char, std::string> pieceSymbols = {
            {'K', "K"}, {'Q', "Q"}, {'R', "R"}, {'B', "B"}, {'N', "N"}, {'P', "P"},
            {'k', "k"}, {'q', "q"}, {'r', "r"}, {'b', "b"}, {'n', "n"}, {'p', "p"},
            {' ', "."}
        };
        
        std::cout << "\n    a  b  c  d  e  f  g  h\n";
        std::cout << "  +" << std::string(24, '-') << "+\n";
        
        for (int i = 0; i < 8; i++) {
            std::cout << (8 - i) << " |";
            for (int j = 0; j < 8; j++) {
                // Use background coloring for board squares
                if ((i + j) % 2 == 0) {
                    std::cout << " " << pieceSymbols[board[i][j]] << " ";
                } else {
                    std::cout << " " << pieceSymbols[board[i][j]] << " ";
                }
            }
            std::cout << "| " << (8 - i) << "\n";
        }
        
        std::cout << "  +" << std::string(24, '-') << "+\n";
        std::cout << "    a  b  c  d  e  f  g  h\n\n";
        
        // Show captured pieces
        if (!capturedPieces["white"].empty() || !capturedPieces["black"].empty()) {
            std::cout << "Captured pieces:\n";
            if (!capturedPieces["black"].empty()) {
                std::cout << "  White captured: ";
                for (char p : capturedPieces["black"]) {
                    std::cout << pieceSymbols[p] << " ";
                }
                std::cout << "\n";
            }
            if (!capturedPieces["white"].empty()) {
                std::cout << "  Black captured: ";
                for (char p : capturedPieces["white"]) {
                    std::cout << pieceSymbols[p] << " ";
                }
                std::cout << "\n";
            }
            std::cout << "\n";
        }
        
        // Show last move
        if (!moveHistory.empty()) {
            std::cout << "Last move: " << moveHistory.back() << "\n\n";
        }
    }
    
    bool isValidPosition(int row, int col) const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }
    
    bool isWhitePiece(char piece) const {
        return std::isupper(piece);
    }
    
    bool isBlackPiece(char piece) const {
        return std::islower(piece);
    }
    
    std::string getPieceColor(char piece) const {
        if (piece == ' ') return "";
        return isWhitePiece(piece) ? "white" : "black";
    }
    
    std::vector<Position> getPawnMoves(int row, int col, char piece) {
        std::vector<Position> moves;
        int direction = isWhitePiece(piece) ? -1 : 1;
        int startRow = isWhitePiece(piece) ? 6 : 1;
        
        // Forward move
        int newRow = row + direction;
        if (isValidPosition(newRow, col) && board[newRow][col] == ' ') {
            moves.push_back(Position(newRow, col));
            
            // Double move from starting position
            if (row == startRow) {
                int newRow2 = row + 2 * direction;
                if (board[newRow2][col] == ' ') {
                    moves.push_back(Position(newRow2, col));
                }
            }
        }
        
        // Captures
        for (int dc : {-1, 1}) {
            newRow = row + direction;
            int newCol = col + dc;
            if (isValidPosition(newRow, newCol)) {
                char target = board[newRow][newCol];
                if (target != ' ' && getPieceColor(target) != getPieceColor(piece)) {
                    moves.push_back(Position(newRow, newCol));
                }
                // En passant
                else if (enPassantTarget == Position(newRow, newCol)) {
                    moves.push_back(Position(newRow, newCol));
                }
            }
        }
        
        return moves;
    }
    
    // Fixed: Add separate function for pawn attack squares (for isSquareAttacked)
    std::vector<Position> getPawnAttackSquares(int row, int col, char piece) {
        std::vector<Position> attacks;
        int direction = isWhitePiece(piece) ? -1 : 1;
        
        for (int dc : {-1, 1}) {
            int newRow = row + direction;
            int newCol = col + dc;
            if (isValidPosition(newRow, newCol)) {
                attacks.push_back(Position(newRow, newCol));
            }
        }
        
        return attacks;
    }
    
    std::vector<Position> getKnightMoves(int row, int col, char piece) {
        std::vector<Position> moves;
        std::vector<std::pair<int, int>> knightMoves = {
            {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
            {1, -2}, {1, 2}, {2, -1}, {2, 1}
        };
        
        for (auto [dr, dc] : knightMoves) {
            int newRow = row + dr;
            int newCol = col + dc;
            if (isValidPosition(newRow, newCol)) {
                char target = board[newRow][newCol];
                if (target == ' ' || getPieceColor(target) != getPieceColor(piece)) {
                    moves.push_back(Position(newRow, newCol));
                }
            }
        }
        
        return moves;
    }
    
    std::vector<Position> getSlidingMoves(int row, int col, char piece, 
                                         const std::vector<std::pair<int, int>>& directions) {
        std::vector<Position> moves;
        
        for (auto [dr, dc] : directions) {
            int newRow = row + dr;
            int newCol = col + dc;
            while (isValidPosition(newRow, newCol)) {
                char target = board[newRow][newCol];
                if (target == ' ') {
                    moves.push_back(Position(newRow, newCol));
                } else if (getPieceColor(target) != getPieceColor(piece)) {
                    moves.push_back(Position(newRow, newCol));
                    break;
                } else {
                    break;
                }
                newRow += dr;
                newCol += dc;
            }
        }
        
        return moves;
    }
    
    std::vector<Position> getBishopMoves(int row, int col, char piece) {
        std::vector<std::pair<int, int>> directions = {
            {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
        };
        return getSlidingMoves(row, col, piece, directions);
    }
    
    std::vector<Position> getRookMoves(int row, int col, char piece) {
        std::vector<std::pair<int, int>> directions = {
            {-1, 0}, {1, 0}, {0, -1}, {0, 1}
        };
        return getSlidingMoves(row, col, piece, directions);
    }
    
    std::vector<Position> getQueenMoves(int row, int col, char piece) {
        std::vector<std::pair<int, int>> directions = {
            {-1, -1}, {-1, 0}, {-1, 1},
            {0, -1},           {0, 1},
            {1, -1},  {1, 0},  {1, 1}
        };
        return getSlidingMoves(row, col, piece, directions);
    }
    
    // Fixed: Separate king moves for actual moves vs attack squares
    std::vector<Position> getKingMoves(int row, int col, char piece, bool includeCastling = true) {
        std::vector<Position> moves;
        std::vector<std::pair<int, int>> directions = {
            {-1, -1}, {-1, 0}, {-1, 1},
            {0, -1},           {0, 1},
            {1, -1},  {1, 0},  {1, 1}
        };
        
        for (auto [dr, dc] : directions) {
            int newRow = row + dr;
            int newCol = col + dc;
            if (isValidPosition(newRow, newCol)) {
                char target = board[newRow][newCol];
                if (target == ' ' || getPieceColor(target) != getPieceColor(piece)) {
                    moves.push_back(Position(newRow, newCol));
                }
            }
        }
        
        // Castling (only include if requested)
        if (includeCastling) {
            if (piece == 'K' && row == 7 && col == 4) {
                if (whiteCanCastleKingside && board[7][5] == ' ' && board[7][6] == ' ' && 
                    board[7][7] == 'R') {
                    moves.push_back(Position(7, 6));
                }
                if (whiteCanCastleQueenside && board[7][1] == ' ' && 
                    board[7][2] == ' ' && board[7][3] == ' ' && board[7][0] == 'R') {
                    moves.push_back(Position(7, 2));
                }
            } else if (piece == 'k' && row == 0 && col == 4) {
                if (blackCanCastleKingside && board[0][5] == ' ' && board[0][6] == ' ' && 
                    board[0][7] == 'r') {
                    moves.push_back(Position(0, 6));
                }
                if (blackCanCastleQueenside && board[0][1] == ' ' && 
                    board[0][2] == ' ' && board[0][3] == ' ' && board[0][0] == 'r') {
                    moves.push_back(Position(0, 2));
                }
            }
        }
        
        return moves;
    }
    
    std::vector<Position> getPieceMoves(int row, int col) {
        char piece = board[row][col];
        if (piece == ' ') return {};
        
        char pieceType = std::toupper(piece);
        
        switch (pieceType) {
            case 'P': return getPawnMoves(row, col, piece);
            case 'N': return getKnightMoves(row, col, piece);
            case 'B': return getBishopMoves(row, col, piece);
            case 'R': return getRookMoves(row, col, piece);
            case 'Q': return getQueenMoves(row, col, piece);
            case 'K': return getKingMoves(row, col, piece);
            default: return {};
        }
    }
    
    // Fixed: Properly check if a square is attacked
    bool isSquareAttacked(int row, int col, const std::string& byColor) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                char piece = board[i][j];
                if (piece != ' ' && getPieceColor(piece) == byColor) {
                    char pieceType = std::toupper(piece);
                    std::vector<Position> moves;
                    
                    // Special handling for pawns (attack differently than they move)
                    if (pieceType == 'P') {
                        moves = getPawnAttackSquares(i, j, piece);
                    } else if (pieceType == 'K') {
                        // King attacks without castling
                        moves = getKingMoves(i, j, piece, false);
                    } else {
                        moves = getPieceMoves(i, j);
                    }
                    
                    for (const auto& pos : moves) {
                        if (pos.row == row && pos.col == col) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
    
    bool isInCheck(const std::string& color) {
        Position kingPos = (color == "white") ? whiteKingPos : blackKingPos;
        std::string opponentColor = (color == "white") ? "black" : "white";
        return isSquareAttacked(kingPos.row, kingPos.col, opponentColor);
    }
    
    // Fixed: Better state management in makeMove
    bool makeMove(int fromRow, int fromCol, int toRow, int toCol) {
        char piece = board[fromRow][fromCol];
        
        if (piece == ' ') return false;
        if (getPieceColor(piece) != currentPlayer) return false;
        
        auto validMoves = getPieceMoves(fromRow, fromCol);
        bool isValid = false;
        for (const auto& pos : validMoves) {
            if (pos.row == toRow && pos.col == toCol) {
                isValid = true;
                break;
            }
        }
        if (!isValid) return false;
        
        // Save state
        auto savedBoard = board;
        auto savedEnPassant = enPassantTarget;
        auto savedWhiteKingPos = whiteKingPos;
        auto savedBlackKingPos = blackKingPos;
        
        // Handle captures
        char captured = board[toRow][toCol];
        if (captured != ' ') {
            capturedPieces[currentPlayer].push_back(captured);
        }
        
        // Handle en passant capture
        if (std::toupper(piece) == 'P' && enPassantTarget == Position(toRow, toCol) && captured == ' ') {
            if (isWhitePiece(piece)) {
                captured = board[toRow + 1][toCol];
                board[toRow + 1][toCol] = ' ';
            } else {
                captured = board[toRow - 1][toCol];
                board[toRow - 1][toCol] = ' ';
            }
            capturedPieces[currentPlayer].push_back(captured);
        }
        
        // Move piece
        board[toRow][toCol] = piece;
        board[fromRow][fromCol] = ' ';
        
        // Update king position
        if (piece == 'K') {
            whiteKingPos = Position(toRow, toCol);
            // Handle castling
            if (fromCol == 4 && toCol == 6) {  // Kingside
                board[7][5] = 'R';
                board[7][7] = ' ';
            } else if (fromCol == 4 && toCol == 2) {  // Queenside
                board[7][3] = 'R';
                board[7][0] = ' ';
            }
            whiteCanCastleKingside = false;
            whiteCanCastleQueenside = false;
        } else if (piece == 'k') {
            blackKingPos = Position(toRow, toCol);
            // Handle castling
            if (fromCol == 4 && toCol == 6) {  // Kingside
                board[0][5] = 'r';
                board[0][7] = ' ';
            } else if (fromCol == 4 && toCol == 2) {  // Queenside
                board[0][3] = 'r';
                board[0][0] = ' ';
            }
            blackCanCastleKingside = false;
            blackCanCastleQueenside = false;
        }
        
        // Update castling rights
        if (piece == 'R' && fromRow == 7) {
            if (fromCol == 0) whiteCanCastleQueenside = false;
            else if (fromCol == 7) whiteCanCastleKingside = false;
        } else if (piece == 'r' && fromRow == 0) {
            if (fromCol == 0) blackCanCastleQueenside = false;
            else if (fromCol == 7) blackCanCastleKingside = false;
        }
        
        // Set en passant target
        enPassantTarget = Position(-1, -1);
        if (std::toupper(piece) == 'P' && std::abs(toRow - fromRow) == 2) {
            enPassantTarget = Position((fromRow + toRow) / 2, fromCol);
        }
        
        // Check if move puts own king in check
        if (isInCheck(currentPlayer)) {
            // Undo move
            board = savedBoard;
            enPassantTarget = savedEnPassant;
            whiteKingPos = savedWhiteKingPos;
            blackKingPos = savedBlackKingPos;
            if (captured != ' ') {
                capturedPieces[currentPlayer].pop_back();
            }
            return false;
        }
        
        // Handle pawn promotion
        if (piece == 'P' && toRow == 0) {
            board[toRow][toCol] = 'Q';
        } else if (piece == 'p' && toRow == 7) {
            board[toRow][toCol] = 'q';
        }
        
        // Record move
        std::string fromPos = std::string(1, 'a' + fromCol) + std::to_string(8 - fromRow);
        std::string toPos = std::string(1, 'a' + toCol) + std::to_string(8 - toRow);
        std::string moveNotation = std::string(1, std::toupper(piece)) + fromPos + "-" + toPos;
        moveHistory.push_back(moveNotation);
        
        return true;
    }
    
    // Fixed: Better handling of move validation
    std::vector<Move> getAllValidMoves(const std::string& color) {
        std::vector<Move> moves;
        
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                char piece = board[i][j];
                if (piece != ' ' && getPieceColor(piece) == color) {
                    auto pieceMoves = getPieceMoves(i, j);
                    for (const auto& pos : pieceMoves) {
                        // Test if move is legal by actually making it
                        auto savedBoard = board;
                        auto savedWhiteKingPos = whiteKingPos;
                        auto savedBlackKingPos = blackKingPos;
                        auto savedEnPassant = enPassantTarget;
                        
                        // Handle en passant capture in simulation
                        char captured = board[pos.row][pos.col];
                        if (std::toupper(piece) == 'P' && enPassantTarget == Position(pos.row, pos.col) && captured == ' ') {
                            if (isWhitePiece(piece)) {
                                board[pos.row + 1][pos.col] = ' ';
                            } else {
                                board[pos.row - 1][pos.col] = ' ';
                            }
                        }
                        
                        // Temporarily make move
                        board[pos.row][pos.col] = piece;
                        board[i][j] = ' ';
                        
                        if (piece == 'K') {
                            whiteKingPos = Position(pos.row, pos.col);
                        } else if (piece == 'k') {
                            blackKingPos = Position(pos.row, pos.col);
                        }
                        
                        // Check if king is in check
                        if (!isInCheck(color)) {
                            moves.push_back(Move(i, j, pos.row, pos.col, 0));
                        }
                        
                        // Restore board
                        board = savedBoard;
                        whiteKingPos = savedWhiteKingPos;
                        blackKingPos = savedBlackKingPos;
                        enPassantTarget = savedEnPassant;
                    }
                }
            }
        }
        
        return moves;
    }
    
    bool isCheckmate(const std::string& color) {
        if (!isInCheck(color)) return false;
        return getAllValidMoves(color).empty();
    }
    
    bool isStalemate(const std::string& color) {
        if (isInCheck(color)) return false;
        return getAllValidMoves(color).empty();
    }
    
    int evaluateBoard() {
        int score = 0;
        
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                char piece = board[i][j];
                if (piece == ' ') continue;
                
                // Material value
                score += PIECE_VALUES.at(piece);
                
                // Positional bonuses
                char pieceType = std::toupper(piece);
                int row = isBlackPiece(piece) ? i : 7 - i;
                
                int bonus = 0;
                if (pieceType == 'P') {
                    bonus = PAWN_TABLE[row][j];
                } else if (pieceType == 'N') {
                    bonus = KNIGHT_TABLE[row][j];
                } else if (pieceType == 'B') {
                    bonus = BISHOP_TABLE[row][j];
                } else if (pieceType == 'K') {
                    bonus = KING_TABLE[row][j];
                }
                
                score += isWhitePiece(piece) ? bonus : -bonus;
            }
        }
        
        return score;
    }
    
    std::optional<Move> getAIMove(int difficulty) {
        auto validMoves = getAllValidMoves(currentPlayer);
        
        if (validMoves.empty()) {
            return std::nullopt;
        }
        
        if (difficulty == 1) {  // Easy - Random move
            std::uniform_int_distribution<int> dist(0, validMoves.size() - 1);
            return validMoves[dist(rng)];
        }
        
        // Evaluate all moves
        std::vector<Move> moveScores;
        
        for (auto& move : validMoves) {
            // Make move
            auto savedBoard = board;
            auto savedWhiteKingPos = whiteKingPos;
            auto savedBlackKingPos = blackKingPos;
            char piece = board[move.fromRow][move.fromCol];
            char captured = board[move.toRow][move.toCol];
            
            board[move.toRow][move.toCol] = piece;
            board[move.fromRow][move.fromCol] = ' ';
            if (piece == 'K') {
                whiteKingPos = Position(move.toRow, move.toCol);
            } else if (piece == 'k') {
                blackKingPos = Position(move.toRow, move.toCol);
            }
            
            // Evaluate position
            int score = evaluateBoard();
            if (currentPlayer == "black") {
                score = -score;
            }
            
            // Bonus for captures
            if (captured != ' ') {
                score += std::abs(PIECE_VALUES.at(captured)) / 10;
            }
            
            // Bonus for checks
            std::string opponent = (currentPlayer == "white") ? "black" : "white";
            if (isInCheck(opponent)) {
                score += 50;
            }
            
            move.score = score;
            moveScores.push_back(move);
            
            // Restore board
            board = savedBoard;
            whiteKingPos = savedWhiteKingPos;
            blackKingPos = savedBlackKingPos;
        }
        
        // Sort by score
        std::sort(moveScores.begin(), moveScores.end(), 
                  [](const Move& a, const Move& b) {
                      return a.score > b.score;
                  });
        
        Move selected;
        
        if (difficulty == 2) {  // Medium
            // 60% best move, 40% from top 5
            std::uniform_real_distribution<double> prob(0.0, 1.0);
            if (prob(rng) < 0.6) {
                selected = moveScores[0];
            } else {
                int range = std::min(5, static_cast<int>(moveScores.size()));
                std::uniform_int_distribution<int> dist(0, range - 1);
                selected = moveScores[dist(rng)];
            }
        } else {  // Hard (difficulty 3)
            // 90% best move, 10% from top 3
            std::uniform_real_distribution<double> prob(0.0, 1.0);
            if (prob(rng) < 0.9) {
                selected = moveScores[0];
            } else {
                int range = std::min(3, static_cast<int>(moveScores.size()));
                std::uniform_int_distribution<int> dist(0, range - 1);
                selected = moveScores[dist(rng)];
            }
        }
        
        return selected;
    }
    
    bool playAITurn(const std::string& aiName, int difficulty) {
        std::cout << aiName << " is thinking";
        std::cout.flush();
        
        int delay = (difficulty == 1) ? 300 : ((difficulty == 2) ? 500 : 700);
        
        for (int i = 0; i < 3; i++) {
            std::cout << ".";
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(delay / 3));
        }
        std::cout << "\n";
        
        auto moveOpt = getAIMove(difficulty);
        
        if (moveOpt.has_value()) {
            Move move = moveOpt.value();
            char piece = board[move.fromRow][move.fromCol];
            
            if (makeMove(move.fromRow, move.fromCol, move.toRow, move.toCol)) {
                std::string fromPos = std::string(1, 'a' + move.fromCol) + 
                                     std::to_string(8 - move.fromRow);
                std::string toPos = std::string(1, 'a' + move.toCol) + 
                                   std::to_string(8 - move.toRow);
                std::cout << aiName << " plays: " << static_cast<char>(std::toupper(piece)) 
                         << " " << fromPos << " to " << toPos << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                return true;
            }
        }
        
        return false;
    }
    
    std::string getDifficultyName(int diff) const {
        if (diff == 1) return "Easy";
        if (diff == 2) return "Medium";
        return "Hard";
    }
    
    void resetGame() {
        initBoard();
        currentPlayer = "white";
        moveHistory.clear();
        capturedPieces["white"].clear();
        capturedPieces["black"].clear();
        winner = "";
        whiteKingPos = Position(7, 4);
        blackKingPos = Position(0, 4);
        whiteCanCastleKingside = true;
        whiteCanCastleQueenside = true;
        blackCanCastleKingside = true;
        blackCanCastleQueenside = true;
        enPassantTarget = Position(-1, -1);
    }
    
    void playAIvsAI() {
        resetGame();
        
        std::cout << "\n=== AI vs AI Chess Match ===\n";
        std::cout << "White AI: " << getDifficultyName(difficultyAI1) << "\n";
        std::cout << "Black AI: " << getDifficultyName(difficultyAI2) << "\n";
        std::cout << "Starting in 2 seconds...\n\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        int maxMoves = 200;
        int moveCount = 0;
        
        while (moveCount < maxMoves) {
            displayBoard();
            
            // Check game over conditions
            if (isCheckmate(currentPlayer)) {
                winner = (currentPlayer == "white") ? "black" : "white";
                break;
            }
            
            if (isStalemate(currentPlayer)) {
                winner = "draw";
                break;
            }
            
            // Get AI difficulty
            std::string aiName = (currentPlayer == "white") ? 
                "White AI (" + getDifficultyName(difficultyAI1) + ")" :
                "Black AI (" + getDifficultyName(difficultyAI2) + ")";
            int aiDiff = (currentPlayer == "white") ? difficultyAI1 : difficultyAI2;
            
            // Play AI turn
            if (!playAITurn(aiName, aiDiff)) {
                winner = "draw";
                break;
            }
            
            // Switch player
            currentPlayer = (currentPlayer == "white") ? "black" : "white";
            moveCount++;
        }
        
        if (moveCount >= maxMoves) {
            winner = "draw";
        }
        
        // Display final position
        displayBoard();
        
        // Show result
        std::cout << "\n" << std::string(50, '=') << "\n";
        if (winner == "white") {
            std::cout << "   CHECKMATE! White AI Wins!\n";
            whiteWins++;
        } else if (winner == "black") {
            std::cout << "   CHECKMATE! Black AI Wins!\n";
            blackWins++;
        } else {
            std::cout << "   DRAW!\n";
            draws++;
        }
        std::cout << std::string(50, '=') << "\n\n";
        
        totalGames++;
        showStatistics();
    }
    
    void showStatistics() {
        std::cout << "\n=== Game Statistics ===\n";
        std::cout << "White AI Wins: " << whiteWins << "\n";
        std::cout << "Black AI Wins: " << blackWins << "\n";
        std::cout << "Draws: " << draws << "\n";
        std::cout << "Total Games: " << totalGames << "\n";
        
        if (totalGames > 0) {
            std::cout << std::fixed << std::setprecision(1);
            std::cout << "\nWhite Win Rate: " << (whiteWins * 100.0 / totalGames) << "%\n";
            std::cout << "Black Win Rate: " << (blackWins * 100.0 / totalGames) << "%\n";
            std::cout << "Draw Rate: " << (draws * 100.0 / totalGames) << "%\n";
        }
        std::cout << std::string(23, '=') << "\n\n";
    }
    
    void showMenu() {
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "              CHESS AI vs AI\n";
        std::cout << std::string(50, '=') << "\n";
        std::cout << "1. Watch AI vs AI\n";
        std::cout << "2. Set White AI Difficulty (Current: " 
                  << getDifficultyName(difficultyAI1) << ")\n";
        std::cout << "3. Set Black AI Difficulty (Current: " 
                  << getDifficultyName(difficultyAI2) << ")\n";
        std::cout << "4. Show Statistics\n";
        std::cout << "5. Reset Statistics\n";
        std::cout << "6. Exit\n";
        std::cout << std::string(50, '=') << "\n";
        std::cout << "Enter your choice: ";
    }
    
    void run() {
        int choice;
        bool running = true;
        
        while (running) {
            showMenu();
            std::cin >> choice;
            
            switch (choice) {
                case 1:
                    playAIvsAI();
                    std::cout << "\nPress Enter to continue...";
                    std::cin.ignore();
                    std::cin.get();
                    break;
                case 2: {
                    int diff;
                    std::cout << "Enter difficulty (1=Easy, 2=Medium, 3=Hard): ";
                    std::cin >> diff;
                    difficultyAI1 = std::max(1, std::min(3, diff));
                    std::cout << "White AI difficulty set to " 
                              << getDifficultyName(difficultyAI1) << "\n";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    break;
                }
                case 3: {
                    int diff;
                    std::cout << "Enter difficulty (1=Easy, 2=Medium, 3=Hard): ";
                    std::cin >> diff;
                    difficultyAI2 = std::max(1, std::min(3, diff));
                    std::cout << "Black AI difficulty set to " 
                              << getDifficultyName(difficultyAI2) << "\n";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    break;
                }
                case 4:
                    showStatistics();
                    std::cout << "\nPress Enter to continue...";
                    std::cin.ignore();
                    std::cin.get();
                    break;
                case 5:
                    whiteWins = 0;
                    blackWins = 0;
                    draws = 0;
                    totalGames = 0;
                    std::cout << "Statistics reset!\n";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    break;
                case 6:
                    running = false;
                    std::cout << "\nThanks for watching!\n";
                    break;
                default:
                    std::cout << "Invalid choice. Please try again.\n";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
};

int main() {
    Chess game;
    game.run();
    return 0;
}
