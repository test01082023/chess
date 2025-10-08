#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cctype>
#include <limits>
#include <initializer_list>
#include <utility>
#include <iomanip>

using namespace std;

// Piece values for AI evaluation
struct PieceValues {
    static int get(char piece) {
        static map<char, int> values;
        if (values.empty()) {
            values['P'] = 100; values['N'] = 320; values['B'] = 330;
            values['R'] = 500; values['Q'] = 900; values['K'] = 20000;
            values['p'] = -100; values['n'] = -320; values['b'] = -330;
            values['r'] = -500; values['q'] = -900; values['k'] = -20000;
        }
        return values[piece];
    }
};

// Position bonus tables
int PAWN_TABLE[8][8] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {5,  5, 10, 25, 25, 10,  5,  5},
    {0,  0,  0, 20, 20,  0,  0,  0},
    {5, -5,-10,  0,  0,-10, -5,  5},
    {5, 10, 10,-20,-20, 10, 10,  5},
    {0,  0,  0,  0,  0,  0,  0,  0}
};

int KNIGHT_TABLE[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

int BISHOP_TABLE[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5, 10, 10,  5,  0,-10},
    {-10,  5,  5, 10, 10,  5,  5,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};

int KING_TABLE[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    { 20, 20,  0,  0,  0,  0, 20, 20},
    { 20, 30, 10,  0,  0, 10, 30, 20}
};

// Utility structures
struct Position {
    int row, col;
    bool valid;
    
    Position() : row(0), col(0), valid(false) {}
    Position(int r, int c) : row(r), col(c), valid(true) {}
    
    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
    
    bool has_value() const { return valid; }
};

struct Move {
    int from_row, from_col, to_row, to_col;
    bool valid;
    
    Move() : from_row(0), from_col(0), to_row(0), to_col(0), valid(false) {}
    Move(int fr, int fc, int tr, int tc) 
        : from_row(fr), from_col(fc), to_row(tr), to_col(tc), valid(true) {}
    
    bool has_value() const { return valid; }
};

struct MoveScore {
    int from_row, from_col, to_row, to_col;
    int score;
    MoveScore(int fr, int fc, int tr, int tc, int s)
        : from_row(fr), from_col(fc), to_row(tr), to_col(tc), score(s) {}
};

// Comparison function for sorting moves by score
bool compare_move_scores(const MoveScore& a, const MoveScore& b) {
    return a.score > b.score;
}

// Helper function to repeat a string
string repeat_string(const string& str, int times) {
    string result;
    for (int i = 0; i < times; i++) {
        result += str;
    }
    return result;
}

class Chess {
private:
    vector<vector<char> > board;
    string current_player;
    vector<string> move_history;
    map<string, vector<char> > captured_pieces;
    string winner;
    int difficulty_ai1;
    int difficulty_ai2;
    
    Position white_king_pos;
    Position black_king_pos;
    bool white_can_castle_kingside;
    bool white_can_castle_queenside;
    bool black_can_castle_kingside;
    bool black_can_castle_queenside;
    Position en_passant_target;
    
    int white_wins;
    int black_wins;
    int draws;
    int total_games;
    
    mt19937 gen;

public:
    Chess() {
        random_device rd;
        gen.seed(rd());
        
        board = init_board();
        current_player = "white";
        winner = "";
        difficulty_ai1 = 2;
        difficulty_ai2 = 2;
        
        white_king_pos = Position(7, 4);
        black_king_pos = Position(0, 4);
        white_can_castle_kingside = true;
        white_can_castle_queenside = true;
        black_can_castle_kingside = true;
        black_can_castle_queenside = true;
        
        white_wins = 0;
        black_wins = 0;
        draws = 0;
        total_games = 0;
        
        captured_pieces["white"] = vector<char>();
        captured_pieces["black"] = vector<char>();
    }
    
    vector<vector<char> > init_board() {
        vector<vector<char> > b(8, vector<char>(8, ' '));
        
        // Black pieces
        char black_back[] = {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'};
        char black_pawns[] = {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'};
        for (int i = 0; i < 8; i++) {
            b[0][i] = black_back[i];
            b[1][i] = black_pawns[i];
        }
        
        // White pieces
        char white_pawns[] = {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'};
        char white_back[] = {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'};
        for (int i = 0; i < 8; i++) {
            b[6][i] = white_pawns[i];
            b[7][i] = white_back[i];
        }
        
        return b;
    }
    
    void clear_screen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }
    
    string get_piece_symbol(char piece) {
        // Unicode chess pieces
        static map<char, string> symbols;
        if (symbols.empty()) {
            // White pieces
            symbols['K'] = "♔";
            symbols['Q'] = "♕";
            symbols['R'] = "♖";
            symbols['B'] = "♗";
            symbols['N'] = "♘";
            symbols['P'] = "♙";
            // Black pieces
            symbols['k'] = "♚";
            symbols['q'] = "♛";
            symbols['r'] = "♜";
            symbols['b'] = "♝";
            symbols['n'] = "♞";
            symbols['p'] = "♟";
            // Empty square
            symbols[' '] = "·";
        }
        return symbols[piece];
    }
    
    void display_board() {
        clear_screen();
        
        cout << "\n" << string(50, '=') << endl;
        cout << "   CHESS - " << current_player << "'s Turn" << endl;
        cout << string(50, '=') << endl;
        
        cout << "\n    a  b  c  d  e  f  g  h" << endl;
        cout << "  ┌" << repeat_string("─", 24) << "┐" << endl;
        
        for (int i = 0; i < 8; i++) {
            cout << (8 - i) << " │";
            for (int j = 0; j < 8; j++) {
                char piece = board[i][j];
                cout << " " << get_piece_symbol(piece) << " ";
            }
            cout << "│ " << (8 - i) << endl;
        }
        
        cout << "  └" << repeat_string("─", 24) << "┘" << endl;
        cout << "    a  b  c  d  e  f  g  h\n" << endl;
        
        // Show captured pieces
        if (!captured_pieces["white"].empty() || !captured_pieces["black"].empty()) {
            cout << "Captured pieces:" << endl;
            if (!captured_pieces["black"].empty()) {
                cout << "  White captured: ";
                for (size_t i = 0; i < captured_pieces["black"].size(); i++) {
                    cout << get_piece_symbol(captured_pieces["black"][i]) << " ";
                }
                cout << endl;
            }
            if (!captured_pieces["white"].empty()) {
                cout << "  Black captured: ";
                for (size_t i = 0; i < captured_pieces["white"].size(); i++) {
                    cout << get_piece_symbol(captured_pieces["white"][i]) << " ";
                }
                cout << endl;
            }
            cout << endl;
        }
        
        // Show last move
        if (!move_history.empty()) {
            cout << "Last move: " << move_history.back() << "\n" << endl;
        }
    }
    
    bool is_valid_position(int row, int col) const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }
    
    bool is_white_piece(char piece) const {
        return piece != ' ' && isupper(static_cast<unsigned char>(piece));
    }
    
    bool is_black_piece(char piece) const {
        return piece != ' ' && islower(static_cast<unsigned char>(piece));
    }
    
    string get_piece_color(char piece) const {
        if (piece == ' ') return "";
        return is_white_piece(piece) ? "white" : "black";
    }
    
    vector<Position> get_pawn_moves(int row, int col, char piece) {
        vector<Position> moves;
        int direction = is_white_piece(piece) ? -1 : 1;
        int start_row = is_white_piece(piece) ? 6 : 1;
        
        // Forward move
        int new_row = row + direction;
        if (is_valid_position(new_row, col) && board[new_row][col] == ' ') {
            moves.push_back(Position(new_row, col));
            
            // Double move from starting position
            if (row == start_row) {
                int new_row2 = row + 2 * direction;
                if (board[new_row2][col] == ' ') {
                    moves.push_back(Position(new_row2, col));
                }
            }
        }
        
        // Captures
        int deltas[] = {-1, 1};
        for (int d = 0; d < 2; d++) {
            int dc = deltas[d];
            new_row = row + direction;
            int new_col = col + dc;
            if (is_valid_position(new_row, new_col)) {
                char target = board[new_row][new_col];
                if (target != ' ' && get_piece_color(target) != get_piece_color(piece)) {
                    moves.push_back(Position(new_row, new_col));
                }
                // En passant
                else if (en_passant_target.has_value() && 
                         en_passant_target.row == new_row && 
                         en_passant_target.col == new_col) {
                    moves.push_back(Position(new_row, new_col));
                }
            }
        }
        
        return moves;
    }
    
    vector<Position> get_knight_moves(int row, int col, char piece) {
        vector<Position> moves;
        int knight_deltas[][2] = {
            {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
            {1, -2}, {1, 2}, {2, -1}, {2, 1}
        };
        
        for (int i = 0; i < 8; i++) {
            int dr = knight_deltas[i][0];
            int dc = knight_deltas[i][1];
            int new_row = row + dr;
            int new_col = col + dc;
            if (is_valid_position(new_row, new_col)) {
                char target = board[new_row][new_col];
                if (target == ' ' || get_piece_color(target) != get_piece_color(piece)) {
                    moves.push_back(Position(new_row, new_col));
                }
            }
        }
        
        return moves;
    }
    
    vector<Position> get_sliding_moves(int row, int col, char piece, 
                                       const int directions[][2], int num_dirs) {
        vector<Position> moves;
        
        for (int i = 0; i < num_dirs; i++) {
            int dr = directions[i][0];
            int dc = directions[i][1];
            int new_row = row + dr;
            int new_col = col + dc;
            while (is_valid_position(new_row, new_col)) {
                char target = board[new_row][new_col];
                if (target == ' ') {
                    moves.push_back(Position(new_row, new_col));
                } else if (get_piece_color(target) != get_piece_color(piece)) {
                    moves.push_back(Position(new_row, new_col));
                    break;
                } else {
                    break;
                }
                new_row += dr;
                new_col += dc;
            }
        }
        
        return moves;
    }
    
    vector<Position> get_bishop_moves(int row, int col, char piece) {
        int directions[][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
        return get_sliding_moves(row, col, piece, directions, 4);
    }
    
    vector<Position> get_rook_moves(int row, int col, char piece) {
        int directions[][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        return get_sliding_moves(row, col, piece, directions, 4);
    }
    
    vector<Position> get_queen_moves(int row, int col, char piece) {
        int directions[][2] = {
            {-1, -1}, {-1, 0}, {-1, 1},
            {0, -1},           {0, 1},
            {1, -1},  {1, 0},  {1, 1}
        };
        return get_sliding_moves(row, col, piece, directions, 8);
    }
    
    vector<Position> get_king_moves(int row, int col, char piece) {
        vector<Position> moves;
        int directions[][2] = {
            {-1, -1}, {-1, 0}, {-1, 1},
            {0, -1},           {0, 1},
            {1, -1},  {1, 0},  {1, 1}
        };
        
        for (int i = 0; i < 8; i++) {
            int dr = directions[i][0];
            int dc = directions[i][1];
            int new_row = row + dr;
            int new_col = col + dc;
            if (is_valid_position(new_row, new_col)) {
                char target = board[new_row][new_col];
                if (target == ' ' || get_piece_color(target) != get_piece_color(piece)) {
                    moves.push_back(Position(new_row, new_col));
                }
            }
        }
        
        // Castling
        if (piece == 'K' && row == 7 && col == 4) {
            if (white_can_castle_kingside && board[7][5] == ' ' && board[7][6] == ' ') {
                moves.push_back(Position(7, 6));
            }
            if (white_can_castle_queenside && board[7][1] == ' ' && 
                board[7][2] == ' ' && board[7][3] == ' ') {
                moves.push_back(Position(7, 2));
            }
        } else if (piece == 'k' && row == 0 && col == 4) {
            if (black_can_castle_kingside && board[0][5] == ' ' && board[0][6] == ' ') {
                moves.push_back(Position(0, 6));
            }
            if (black_can_castle_queenside && board[0][1] == ' ' && 
                board[0][2] == ' ' && board[0][3] == ' ') {
                moves.push_back(Position(0, 2));
            }
        }
        
        return moves;
    }
    
    vector<Position> get_piece_moves(int row, int col) {
        char piece = board[row][col];
        if (piece == ' ') return vector<Position>();
        
        char piece_type = toupper(static_cast<unsigned char>(piece));
        
        switch (piece_type) {
            case 'P': return get_pawn_moves(row, col, piece);
            case 'N': return get_knight_moves(row, col, piece);
            case 'B': return get_bishop_moves(row, col, piece);
            case 'R': return get_rook_moves(row, col, piece);
            case 'Q': return get_queen_moves(row, col, piece);
            case 'K': return get_king_moves(row, col, piece);
        }
        
        return vector<Position>();
    }
    
    bool is_square_attacked(int row, int col, const string& by_color) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                char piece = board[i][j];
                if (piece != ' ' && get_piece_color(piece) == by_color) {
                    vector<Position> moves = get_piece_moves(i, j);
                    for (size_t k = 0; k < moves.size(); k++) {
                        if (moves[k].row == row && moves[k].col == col) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
    
    bool is_in_check(const string& color) {
        Position king_pos = (color == "white") ? white_king_pos : black_king_pos;
        string opponent_color = (color == "white") ? "black" : "white";
        return is_square_attacked(king_pos.row, king_pos.col, opponent_color);
    }
    
    // Helper to save game state
    struct GameState {
        vector<vector<char> > board;
        Position white_king;
        Position black_king;
        bool w_castle_k, w_castle_q, b_castle_k, b_castle_q;
        Position en_passant;
    };
    
    GameState save_state() {
        GameState state;
        state.board = board;
        state.white_king = white_king_pos;
        state.black_king = black_king_pos;
        state.w_castle_k = white_can_castle_kingside;
        state.w_castle_q = white_can_castle_queenside;
        state.b_castle_k = black_can_castle_kingside;
        state.b_castle_q = black_can_castle_queenside;
        state.en_passant = en_passant_target;
        return state;
    }
    
    void restore_state(const GameState& state) {
        board = state.board;
        white_king_pos = state.white_king;
        black_king_pos = state.black_king;
        white_can_castle_kingside = state.w_castle_k;
        white_can_castle_queenside = state.w_castle_q;
        black_can_castle_kingside = state.b_castle_k;
        black_can_castle_queenside = state.b_castle_q;
        en_passant_target = state.en_passant;
    }
    
    bool make_move(int from_row, int from_col, int to_row, int to_col) {
        char piece = board[from_row][from_col];
        
        if (piece == ' ') return false;
        if (get_piece_color(piece) != current_player) return false;
        
        vector<Position> valid_moves = get_piece_moves(from_row, from_col);
        bool found = false;
        for (size_t i = 0; i < valid_moves.size(); i++) {
            if (valid_moves[i].row == to_row && valid_moves[i].col == to_col) {
                found = true;
                break;
            }
        }
        if (!found) return false;
        
        // Save state
        GameState saved_state = save_state();
        
        // Handle captures
        char captured = board[to_row][to_col];
        
        // Handle en passant capture
        if (toupper(static_cast<unsigned char>(piece)) == 'P' && en_passant_target.has_value() && 
            en_passant_target.row == to_row && en_passant_target.col == to_col) {
            if (is_white_piece(piece)) {
                captured = board[to_row + 1][to_col];
                board[to_row + 1][to_col] = ' ';
            } else {
                captured = board[to_row - 1][to_col];
                board[to_row - 1][to_col] = ' ';
            }
        }
        
        if (captured != ' ') {
            captured_pieces[current_player].push_back(captured);
        }
        
        // Move piece
        board[to_row][to_col] = piece;
        board[from_row][from_col] = ' ';
        
        // Update king position
        if (piece == 'K') {
            white_king_pos = Position(to_row, to_col);
            // Handle castling
            if (from_col == 4 && to_col == 6) {
                board[7][5] = 'R';
                board[7][7] = ' ';
            } else if (from_col == 4 && to_col == 2) {
                board[7][3] = 'R';
                board[7][0] = ' ';
            }
            white_can_castle_kingside = false;
            white_can_castle_queenside = false;
        } else if (piece == 'k') {
            black_king_pos = Position(to_row, to_col);
            // Handle castling
            if (from_col == 4 && to_col == 6) {
                board[0][5] = 'r';
                board[0][7] = ' ';
            } else if (from_col == 4 && to_col == 2) {
                board[0][3] = 'r';
                board[0][0] = ' ';
            }
            black_can_castle_kingside = false;
            black_can_castle_queenside = false;
        }
        
        // Update castling rights for rook moves
        if (piece == 'R' && from_row == 7) {
            if (from_col == 0) white_can_castle_queenside = false;
            else if (from_col == 7) white_can_castle_kingside = false;
        } else if (piece == 'r' && from_row == 0) {
            if (from_col == 0) black_can_castle_queenside = false;
            else if (from_col == 7) black_can_castle_kingside = false;
        }
        
        // Set en passant target
        en_passant_target = Position();
        if (toupper(static_cast<unsigned char>(piece)) == 'P' && abs(to_row - from_row) == 2) {
            en_passant_target = Position((from_row + to_row) / 2, from_col);
        }
        
        // Check if move puts own king in check
        if (is_in_check(current_player)) {
            // Undo move
            restore_state(saved_state);
            if (captured != ' ') {
                captured_pieces[current_player].pop_back();
            }
            return false;
        }
        
        // Handle pawn promotion
        if (piece == 'P' && to_row == 0) {
            board[to_row][to_col] = 'Q';
        } else if (piece == 'p' && to_row == 7) {
            board[to_row][to_col] = 'q';
        }
        
        // Record move
        string from_pos = string(1, char('a' + from_col)) + char('0' + (8 - from_row));
        string to_pos = string(1, char('a' + to_col)) + char('0' + (8 - to_row));
        string move_notation = string(1, char(toupper(static_cast<unsigned char>(piece)))) + from_pos + "-" + to_pos;
        move_history.push_back(move_notation);
        
        return true;
    }
    
    vector<Move> get_all_valid_moves(const string& color) {
        vector<Move> moves;
        
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                char piece = board[i][j];
                if (piece != ' ' && get_piece_color(piece) == color) {
                    vector<Position> piece_moves = get_piece_moves(i, j);
                    for (size_t k = 0; k < piece_moves.size(); k++) {
                        Position pos = piece_moves[k];
                        // Test if move is legal
                        GameState saved_state = save_state();
                        
                        // Temporarily make move
                        board[pos.row][pos.col] = piece;
                        board[i][j] = ' ';
                        if (piece == 'K') {
                            white_king_pos = Position(pos.row, pos.col);
                        } else if (piece == 'k') {
                            black_king_pos = Position(pos.row, pos.col);
                        }
                        
                        // Check if king is in check
                        if (!is_in_check(color)) {
                            moves.push_back(Move(i, j, pos.row, pos.col));
                        }
                        
                        // Restore board
                        restore_state(saved_state);
                    }
                }
            }
        }
        
        return moves;
    }
    
    bool is_checkmate(const string& color) {
        if (!is_in_check(color)) return false;
        return get_all_valid_moves(color).empty();
    }
    
    bool is_stalemate(const string& color) {
        if (is_in_check(color)) return false;
        return get_all_valid_moves(color).empty();
    }
    
    int evaluate_board() {
        int score = 0;
        
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                char piece = board[i][j];
                if (piece == ' ') continue;
                
                // Material value
                score += PieceValues::get(piece);
                
                // Positional bonuses
                char piece_type = toupper(static_cast<unsigned char>(piece));
                int row = is_black_piece(piece) ? i : 7 - i;
                
                int bonus = 0;
                if (piece_type == 'P') {
                    bonus = PAWN_TABLE[row][j];
                } else if (piece_type == 'N') {
                    bonus = KNIGHT_TABLE[row][j];
                } else if (piece_type == 'B') {
                    bonus = BISHOP_TABLE[row][j];
                } else if (piece_type == 'K') {
                    bonus = KING_TABLE[row][j];
                }
                
                score += is_white_piece(piece) ? bonus : -bonus;
            }
        }
        
        return score;
    }
    
    Move get_ai_move(int difficulty) {
        vector<Move> valid_moves = get_all_valid_moves(current_player);
        
        if (valid_moves.empty()) return Move();
        
        if (difficulty == 1) {
            // Easy - Random move
            uniform_int_distribution<int> dis(0, valid_moves.size() - 1);
            return valid_moves[dis(gen)];
        }
        
        // Evaluate all moves
        vector<MoveScore> move_scores;
        
        for (size_t m = 0; m < valid_moves.size(); m++) {
            Move move = valid_moves[m];
            // Make move
            GameState saved_state = save_state();
            char piece = board[move.from_row][move.from_col];
            char captured = board[move.to_row][move.to_col];
            
            board[move.to_row][move.to_col] = piece;
            board[move.from_row][move.from_col] = ' ';
            if (piece == 'K') {
                white_king_pos = Position(move.to_row, move.to_col);
            } else if (piece == 'k') {
                black_king_pos = Position(move.to_row, move.to_col);
            }
            
            // Evaluate position
            int score = evaluate_board();
            if (current_player == "black") {
                score = -score;
            }
            
            // Bonus for captures
            if (captured != ' ') {
                score += abs(PieceValues::get(captured)) / 10;
            }
            
            // Bonus for checks
            string opponent = (current_player == "white") ? "black" : "white";
            if (is_in_check(opponent)) {
                score += 50;
            }
            
            move_scores.push_back(MoveScore(move.from_row, move.from_col, 
                                           move.to_row, move.to_col, score));
            
            // Restore board
            restore_state(saved_state);
        }
        
        // Sort by score (descending) using comparison function
        sort(move_scores.begin(), move_scores.end(), compare_move_scores);
        
        MoveScore selected = move_scores[0];
        
        uniform_real_distribution<double> prob_dist(0.0, 1.0);
        
        if (difficulty == 2) {
            // Medium: 60% best move, 40% from top 5
            if (prob_dist(gen) < 0.6) {
                selected = move_scores[0];
            } else {
                int top_count = min(5, (int)move_scores.size());
                uniform_int_distribution<int> dis(0, top_count - 1);
                selected = move_scores[dis(gen)];
            }
        } else {
            // Hard: 90% best move, 10% from top 3
            if (prob_dist(gen) < 0.9) {
                selected = move_scores[0];
            } else {
                int top_count = min(3, (int)move_scores.size());
                uniform_int_distribution<int> dis(0, top_count - 1);
                selected = move_scores[dis(gen)];
            }
        }
        
        return Move(selected.from_row, selected.from_col, 
                   selected.to_row, selected.to_col);
    }
    
    bool play_ai_turn(const string& ai_name, int difficulty) {
        cout << ai_name << " is thinking" << flush;
        
        int delay_ms = (difficulty == 1) ? 300 : ((difficulty == 2) ? 500 : 700);
        
        for (int i = 0; i < 3; i++) {
            cout << "." << flush;
            this_thread::sleep_for(chrono::milliseconds(delay_ms / 3));
        }
        cout << endl;
        
        Move move = get_ai_move(difficulty);
        
        if (move.has_value()) {
            char piece = board[move.from_row][move.from_col];
            
            if (make_move(move.from_row, move.from_col, move.to_row, move.to_col)) {
                string from_pos = string(1, char('a' + move.from_col)) + char('0' + (8 - move.from_row));
                string to_pos = string(1, char('a' + move.to_col)) + char('0' + (8 - move.to_row));
                cout << ai_name << " plays: " << get_piece_symbol(piece) << " " << from_pos << " → " << to_pos << endl;
                this_thread::sleep_for(chrono::milliseconds(500));
                return true;
            }
        }
        
        return false;
    }
    
    string get_difficulty_name(int diff) const {
        if (diff == 1) return "Easy";
        if (diff == 2) return "Medium";
        return "Hard";
    }
    
    void reset_game() {
        board = init_board();
        current_player = "white";
        move_history.clear();
        captured_pieces["white"].clear();
        captured_pieces["black"].clear();
        winner = "";
        white_king_pos = Position(7, 4);
        black_king_pos = Position(0, 4);
        white_can_castle_kingside = true;
        white_can_castle_queenside = true;
        black_can_castle_kingside = true;
        black_can_castle_queenside = true;
        en_passant_target = Position();
    }
    
    void play_ai_vs_ai() {
        reset_game();
        
        cout << "\n=== AI vs AI Chess Match ===" << endl;
        cout << "White AI: " << get_difficulty_name(difficulty_ai1) << endl;
        cout << "Black AI: " << get_difficulty_name(difficulty_ai2) << endl;
        cout << "Starting in 2 seconds...\n" << endl;
        this_thread::sleep_for(chrono::seconds(2));
        
        const int max_moves = 200;
        int move_count = 0;
        
        while (move_count < max_moves) {
            display_board();
            
            // Check game over conditions
            if (is_checkmate(current_player)) {
                winner = (current_player == "white") ? "black" : "white";
                break;
            }
            
            if (is_stalemate(current_player)) {
                winner = "draw";
                break;
            }
            
            // Get AI difficulty
            string ai_name = (current_player == "white") ? 
                "White AI (" + get_difficulty_name(difficulty_ai1) + ")" :
                "Black AI (" + get_difficulty_name(difficulty_ai2) + ")";
            int ai_diff = (current_player == "white") ? difficulty_ai1 : difficulty_ai2;
            
            // Play AI turn
            if (!play_ai_turn(ai_name, ai_diff)) {
                winner = "draw";
                break;
            }
            
            // Switch player
            current_player = (current_player == "white") ? "black" : "white";
            move_count++;
        }
        
        if (move_count >= max_moves) {
            winner = "draw";
        }
        
        // Display final position
        display_board();
        
        // Show result
        cout << "\n" << string(50, '=') << endl;
        if (winner == "white") {
            cout << "   CHECKMATE! White AI Wins! ♔" << endl;
            white_wins++;
        } else if (winner == "black") {
            cout << "   CHECKMATE! Black AI Wins! ♚" << endl;
            black_wins++;
        } else {
            cout << "   DRAW! ½-½" << endl;
            draws++;
        }
        cout << string(50, '=') << "\n" << endl;
        
        total_games++;
        show_statistics();
    }
    
    void show_statistics() const {
        cout << "\n=== Game Statistics ===" << endl;
        cout << "White AI Wins: " << white_wins << " ♔" << endl;
        cout << "Black AI Wins: " << black_wins << " ♚" << endl;
        cout << "Draws: " << draws << endl;
        cout << "Total Games: " << total_games << endl;
        
        if (total_games > 0) {
            cout << fixed << setprecision(1);
            cout << "\nWhite Win Rate: " << (white_wins * 100.0 / total_games) << "%" << endl;
            cout << "Black Win Rate: " << (black_wins * 100.0 / total_games) << "%" << endl;
            cout << "Draw Rate: " << (draws * 100.0 / total_games) << "%" << endl;
        }
        cout << string(23, '=') << "\n" << endl;
    }
    
    void show_menu() const {
        cout << "\n" << string(50, '=') << endl;
        cout << "              ♔ CHESS AI vs AI ♚" << endl;
        cout << string(50, '=') << endl;
        cout << "1. Watch AI vs AI" << endl;
        cout << "2. Set White AI Difficulty (Current: " << get_difficulty_name(difficulty_ai1) << ")" << endl;
        cout << "3. Set Black AI Difficulty (Current: " << get_difficulty_name(difficulty_ai2) << ")" << endl;
        cout << "4. Show Statistics" << endl;
        cout << "5. Reset Statistics" << endl;
        cout << "6. Exit" << endl;
        cout << string(50, '=') << endl;
        cout << "Enter your choice: ";
    }
    
    void run() {
        bool running = true;
        
        while (running) {
            show_menu();
            
            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid choice. Please try again." << endl;
                this_thread::sleep_for(chrono::seconds(1));
                continue;
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            
            switch (choice) {
                case 1:
                    play_ai_vs_ai();
                    cout << "\nPress Enter to continue...";
                    cin.get();
                    break;
                    
                case 2: {
                    int diff;
                    cout << "Enter difficulty (1=Easy, 2=Medium, 3=Hard): ";
                    if (cin >> diff) {
                        difficulty_ai1 = max(1, min(3, diff));
                        cout << "White AI difficulty set to " << get_difficulty_name(difficulty_ai1) << endl;
                    } else {
                        cin.clear();
                        cout << "Invalid input." << endl;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    this_thread::sleep_for(chrono::seconds(1));
                    break;
                }
                    
                case 3: {
                    int diff;
                    cout << "Enter difficulty (1=Easy, 2=Medium, 3=Hard): ";
                    if (cin >> diff) {
                        difficulty_ai2 = max(1, min(3, diff));
                        cout << "Black AI difficulty set to " << get_difficulty_name(difficulty_ai2) << endl;
                    } else {
                        cin.clear();
                        cout << "Invalid input." << endl;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    this_thread::sleep_for(chrono::seconds(1));
                    break;
                }
                    
                case 4:
                    show_statistics();
                    cout << "\nPress Enter to continue...";
                    cin.get();
                    break;
                    
                case 5:
                    white_wins = 0;
                    black_wins = 0;
                    draws = 0;
                    total_games = 0;
                    cout << "Statistics reset!" << endl;
                    this_thread::sleep_for(chrono::seconds(1));
                    break;
                    
                case 6:
                    running = false;
                    cout << "\nThanks for watching! ♟" << endl;
                    break;
                    
                default:
                    cout << "Invalid choice. Please try again." << endl;
                    this_thread::sleep_for(chrono::seconds(1));
            }
        }
    }
};

int main() {
    Chess game;
    game.run();
    return 0;
}
