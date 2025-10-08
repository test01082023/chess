import random
import time
import os
from typing import List, Tuple, Optional
from copy import deepcopy

# Piece values for AI evaluation
PIECE_VALUES = {
    'P': 100, 'N': 320, 'B': 330, 'R': 500, 'Q': 900, 'K': 20000,
    'p': -100, 'n': -320, 'b': -330, 'r': -500, 'q': -900, 'k': -20000
}

# Position bonuses for pieces (encourages good positioning)
PAWN_TABLE = [
    [0,  0,  0,  0,  0,  0,  0,  0],
    [50, 50, 50, 50, 50, 50, 50, 50],
    [10, 10, 20, 30, 30, 20, 10, 10],
    [5,  5, 10, 25, 25, 10,  5,  5],
    [0,  0,  0, 20, 20,  0,  0,  0],
    [5, -5,-10,  0,  0,-10, -5,  5],
    [5, 10, 10,-20,-20, 10, 10,  5],
    [0,  0,  0,  0,  0,  0,  0,  0]
]

KNIGHT_TABLE = [
    [-50,-40,-30,-30,-30,-30,-40,-50],
    [-40,-20,  0,  0,  0,  0,-20,-40],
    [-30,  0, 10, 15, 15, 10,  0,-30],
    [-30,  5, 15, 20, 20, 15,  5,-30],
    [-30,  0, 15, 20, 20, 15,  0,-30],
    [-30,  5, 10, 15, 15, 10,  5,-30],
    [-40,-20,  0,  5,  5,  0,-20,-40],
    [-50,-40,-30,-30,-30,-30,-40,-50]
]

BISHOP_TABLE = [
    [-20,-10,-10,-10,-10,-10,-10,-20],
    [-10,  0,  0,  0,  0,  0,  0,-10],
    [-10,  0,  5, 10, 10,  5,  0,-10],
    [-10,  5,  5, 10, 10,  5,  5,-10],
    [-10,  0, 10, 10, 10, 10,  0,-10],
    [-10, 10, 10, 10, 10, 10, 10,-10],
    [-10,  5,  0,  0,  0,  0,  5,-10],
    [-20,-10,-10,-10,-10,-10,-10,-20]
]

KING_TABLE = [
    [-30,-40,-40,-50,-50,-40,-40,-30],
    [-30,-40,-40,-50,-50,-40,-40,-30],
    [-30,-40,-40,-50,-50,-40,-40,-30],
    [-30,-40,-40,-50,-50,-40,-40,-30],
    [-20,-30,-30,-40,-40,-30,-30,-20],
    [-10,-20,-20,-20,-20,-20,-20,-10],
    [ 20, 20,  0,  0,  0,  0, 20, 20],
    [ 20, 30, 10,  0,  0, 10, 30, 20]
]


class Chess:
    def __init__(self):
        self.board = self.init_board()
        self.current_player = 'white'  # white goes first
        self.move_history = []
        self.captured_pieces = {'white': [], 'black': []}
        self.winner = None
        self.difficulty_ai1 = 2  # White AI
        self.difficulty_ai2 = 2  # Black AI
        
        # Game state
        self.white_king_pos = (7, 4)
        self.black_king_pos = (0, 4)
        self.white_can_castle_kingside = True
        self.white_can_castle_queenside = True
        self.black_can_castle_kingside = True
        self.black_can_castle_queenside = True
        self.en_passant_target = None
        
        # Statistics
        self.white_wins = 0
        self.black_wins = 0
        self.draws = 0
        self.total_games = 0
    
    def init_board(self) -> List[List[str]]:
        """Initialize the chess board with starting positions."""
        board = [[' ' for _ in range(8)] for _ in range(8)]
        
        # Black pieces (top)
        board[0] = ['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r']
        board[1] = ['p'] * 8
        
        # White pieces (bottom)
        board[6] = ['P'] * 8
        board[7] = ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R']
        
        return board
    
    def display_board(self):
        """Display the chess board."""
        os.system('cls' if os.name == 'nt' else 'clear')
        
        print("\n" + "=" * 50)
        print(f"   CHESS - {self.current_player.upper()}'s Turn")
        print("=" * 50)
        
        # Unicode chess pieces for better visualization
        piece_symbols = {
            'K': '♔', 'Q': '♕', 'R': '♖', 'B': '♗', 'N': '♘', 'P': '♙',
            'k': '♚', 'q': '♛', 'r': '♜', 'b': '♝', 'n': '♞', 'p': '♟',
            ' ': '·'
        }
        
        print("\n    a  b  c  d  e  f  g  h")
        print("  ┌" + "─" * 24 + "┐")
        
        for i in range(8):
            print(f"{8-i} │", end="")
            for j in range(8):
                piece = self.board[i][j]
                symbol = piece_symbols.get(piece, piece)
                print(f" {symbol} ", end="")
            print(f"│ {8-i}")
        
        print("  └" + "─" * 24 + "┘")
        print("    a  b  c  d  e  f  g  h\n")
        
        # Show captured pieces
        if self.captured_pieces['white'] or self.captured_pieces['black']:
            print("Captured pieces:")
            if self.captured_pieces['black']:
                black_caps = [piece_symbols.get(p, p) for p in self.captured_pieces['black']]
                print(f"  White captured: {' '.join(black_caps)}")
            if self.captured_pieces['white']:
                white_caps = [piece_symbols.get(p, p) for p in self.captured_pieces['white']]
                print(f"  Black captured: {' '.join(white_caps)}")
            print()
        
        # Show last move
        if self.move_history:
            print(f"Last move: {self.move_history[-1]}\n")
    
    def is_valid_position(self, row: int, col: int) -> bool:
        """Check if position is within board boundaries."""
        return 0 <= row < 8 and 0 <= col < 8
    
    def is_white_piece(self, piece: str) -> bool:
        """Check if piece belongs to white."""
        return piece.isupper()
    
    def is_black_piece(self, piece: str) -> bool:
        """Check if piece belongs to black."""
        return piece.islower()
    
    def get_piece_color(self, piece: str) -> Optional[str]:
        """Get the color of a piece."""
        if piece == ' ':
            return None
        return 'white' if self.is_white_piece(piece) else 'black'
    
    def get_pawn_moves(self, row: int, col: int, piece: str) -> List[Tuple[int, int]]:
        """Get all valid moves for a pawn."""
        moves = []
        direction = -1 if self.is_white_piece(piece) else 1
        start_row = 6 if self.is_white_piece(piece) else 1
        
        # Forward move
        new_row = row + direction
        if self.is_valid_position(new_row, col) and self.board[new_row][col] == ' ':
            moves.append((new_row, col))
            
            # Double move from starting position
            if row == start_row:
                new_row2 = row + 2 * direction
                if self.board[new_row2][col] == ' ':
                    moves.append((new_row2, col))
        
        # Captures
        for dc in [-1, 1]:
            new_row = row + direction
            new_col = col + dc
            if self.is_valid_position(new_row, new_col):
                target = self.board[new_row][new_col]
                if target != ' ' and self.get_piece_color(target) != self.get_piece_color(piece):
                    moves.append((new_row, new_col))
                # En passant
                elif self.en_passant_target == (new_row, new_col):
                    moves.append((new_row, new_col))
        
        return moves
    
    def get_knight_moves(self, row: int, col: int, piece: str) -> List[Tuple[int, int]]:
        """Get all valid moves for a knight."""
        moves = []
        knight_moves = [
            (-2, -1), (-2, 1), (-1, -2), (-1, 2),
            (1, -2), (1, 2), (2, -1), (2, 1)
        ]
        
        for dr, dc in knight_moves:
            new_row, new_col = row + dr, col + dc
            if self.is_valid_position(new_row, new_col):
                target = self.board[new_row][new_col]
                if target == ' ' or self.get_piece_color(target) != self.get_piece_color(piece):
                    moves.append((new_row, new_col))
        
        return moves
    
    def get_sliding_moves(self, row: int, col: int, piece: str, directions: List[Tuple[int, int]]) -> List[Tuple[int, int]]:
        """Get moves for sliding pieces (bishop, rook, queen)."""
        moves = []
        
        for dr, dc in directions:
            new_row, new_col = row + dr, col + dc
            while self.is_valid_position(new_row, new_col):
                target = self.board[new_row][new_col]
                if target == ' ':
                    moves.append((new_row, new_col))
                elif self.get_piece_color(target) != self.get_piece_color(piece):
                    moves.append((new_row, new_col))
                    break
                else:
                    break
                new_row += dr
                new_col += dc
        
        return moves
    
    def get_bishop_moves(self, row: int, col: int, piece: str) -> List[Tuple[int, int]]:
        """Get all valid moves for a bishop."""
        directions = [(-1, -1), (-1, 1), (1, -1), (1, 1)]
        return self.get_sliding_moves(row, col, piece, directions)
    
    def get_rook_moves(self, row: int, col: int, piece: str) -> List[Tuple[int, int]]:
        """Get all valid moves for a rook."""
        directions = [(-1, 0), (1, 0), (0, -1), (0, 1)]
        return self.get_sliding_moves(row, col, piece, directions)
    
    def get_queen_moves(self, row: int, col: int, piece: str) -> List[Tuple[int, int]]:
        """Get all valid moves for a queen."""
        directions = [
            (-1, -1), (-1, 0), (-1, 1),
            (0, -1),           (0, 1),
            (1, -1),  (1, 0),  (1, 1)
        ]
        return self.get_sliding_moves(row, col, piece, directions)
    
    def get_king_moves(self, row: int, col: int, piece: str) -> List[Tuple[int, int]]:
        """Get all valid moves for a king."""
        moves = []
        directions = [
            (-1, -1), (-1, 0), (-1, 1),
            (0, -1),           (0, 1),
            (1, -1),  (1, 0),  (1, 1)
        ]
        
        for dr, dc in directions:
            new_row, new_col = row + dr, col + dc
            if self.is_valid_position(new_row, new_col):
                target = self.board[new_row][new_col]
                if target == ' ' or self.get_piece_color(target) != self.get_piece_color(piece):
                    moves.append((new_row, new_col))
        
        # Castling (simplified - doesn't check if king passes through check)
        if piece == 'K' and row == 7 and col == 4:
            if self.white_can_castle_kingside and self.board[7][5] == ' ' and self.board[7][6] == ' ':
                moves.append((7, 6))
            if self.white_can_castle_queenside and self.board[7][1] == ' ' and self.board[7][2] == ' ' and self.board[7][3] == ' ':
                moves.append((7, 2))
        elif piece == 'k' and row == 0 and col == 4:
            if self.black_can_castle_kingside and self.board[0][5] == ' ' and self.board[0][6] == ' ':
                moves.append((0, 6))
            if self.black_can_castle_queenside and self.board[0][1] == ' ' and self.board[0][2] == ' ' and self.board[0][3] == ' ':
                moves.append((0, 2))
        
        return moves
    
    def get_piece_moves(self, row: int, col: int) -> List[Tuple[int, int]]:
        """Get all valid moves for a piece at given position."""
        piece = self.board[row][col]
        if piece == ' ':
            return []
        
        piece_type = piece.upper()
        
        if piece_type == 'P':
            return self.get_pawn_moves(row, col, piece)
        elif piece_type == 'N':
            return self.get_knight_moves(row, col, piece)
        elif piece_type == 'B':
            return self.get_bishop_moves(row, col, piece)
        elif piece_type == 'R':
            return self.get_rook_moves(row, col, piece)
        elif piece_type == 'Q':
            return self.get_queen_moves(row, col, piece)
        elif piece_type == 'K':
            return self.get_king_moves(row, col, piece)
        
        return []
    
    def is_square_attacked(self, row: int, col: int, by_color: str) -> bool:
        """Check if a square is attacked by a given color."""
        for i in range(8):
            for j in range(8):
                piece = self.board[i][j]
                if piece != ' ' and self.get_piece_color(piece) == by_color:
                    moves = self.get_piece_moves(i, j)
                    if (row, col) in moves:
                        return True
        return False
    
    def is_in_check(self, color: str) -> bool:
        """Check if the king of given color is in check."""
        king_pos = self.white_king_pos if color == 'white' else self.black_king_pos
        opponent_color = 'black' if color == 'white' else 'white'
        return self.is_square_attacked(king_pos[0], king_pos[1], opponent_color)
    
    def make_move(self, from_row: int, from_col: int, to_row: int, to_col: int) -> bool:
        """Make a move on the board."""
        piece = self.board[from_row][from_col]
        
        if piece == ' ':
            return False
        
        if self.get_piece_color(piece) != self.current_player:
            return False
        
        valid_moves = self.get_piece_moves(from_row, from_col)
        if (to_row, to_col) not in valid_moves:
            return False
        
        # Save state for undo if move puts own king in check
        saved_board = deepcopy(self.board)
        saved_en_passant = self.en_passant_target
        
        # Handle captures
        captured = self.board[to_row][to_col]
        if captured != ' ':
            self.captured_pieces[self.current_player].append(captured)
        
        # Handle en passant capture
        if piece.upper() == 'P' and self.en_passant_target == (to_row, to_col):
            if self.is_white_piece(piece):
                self.board[to_row + 1][to_col] = ' '
                captured = 'p'
            else:
                self.board[to_row - 1][to_col] = ' '
                captured = 'P'
            self.captured_pieces[self.current_player].append(captured)
        
        # Move piece
        self.board[to_row][to_col] = piece
        self.board[from_row][from_col] = ' '
        
        # Update king position
        if piece == 'K':
            self.white_king_pos = (to_row, to_col)
            # Handle castling
            if from_col == 4 and to_col == 6:  # Kingside
                self.board[7][5] = 'R'
                self.board[7][7] = ' '
            elif from_col == 4 and to_col == 2:  # Queenside
                self.board[7][3] = 'R'
                self.board[7][0] = ' '
            self.white_can_castle_kingside = False
            self.white_can_castle_queenside = False
        elif piece == 'k':
            self.black_king_pos = (to_row, to_col)
            # Handle castling
            if from_col == 4 and to_col == 6:  # Kingside
                self.board[0][5] = 'r'
                self.board[0][7] = ' '
            elif from_col == 4 and to_col == 2:  # Queenside
                self.board[0][3] = 'r'
                self.board[0][0] = ' '
            self.black_can_castle_kingside = False
            self.black_can_castle_queenside = False
        
        # Update castling rights
        if piece == 'R' and from_row == 7:
            if from_col == 0:
                self.white_can_castle_queenside = False
            elif from_col == 7:
                self.white_can_castle_kingside = False
        elif piece == 'r' and from_row == 0:
            if from_col == 0:
                self.black_can_castle_queenside = False
            elif from_col == 7:
                self.black_can_castle_kingside = False
        
        # Set en passant target
        self.en_passant_target = None
        if piece.upper() == 'P' and abs(to_row - from_row) == 2:
            self.en_passant_target = ((from_row + to_row) // 2, from_col)
        
        # Check if move puts own king in check
        if self.is_in_check(self.current_player):
            # Undo move
            self.board = saved_board
            self.en_passant_target = saved_en_passant
            if captured != ' ':
                self.captured_pieces[self.current_player].pop()
            if piece == 'K':
                self.white_king_pos = (from_row, from_col)
            elif piece == 'k':
                self.black_king_pos = (from_row, from_col)
            return False
        
        # Handle pawn promotion
        if piece == 'P' and to_row == 0:
            self.board[to_row][to_col] = 'Q'  # Auto-promote to queen
        elif piece == 'p' and to_row == 7:
            self.board[to_row][to_col] = 'q'  # Auto-promote to queen
        
        # Record move
        from_pos = chr(ord('a') + from_col) + str(8 - from_row)
        to_pos = chr(ord('a') + to_col) + str(8 - to_row)
        move_notation = f"{piece.upper()}{from_pos}-{to_pos}"
        self.move_history.append(move_notation)
        
        return True
    
    def get_all_valid_moves(self, color: str) -> List[Tuple[int, int, int, int]]:
        """Get all valid moves for a color."""
        moves = []
        
        for i in range(8):
            for j in range(8):
                piece = self.board[i][j]
                if piece != ' ' and self.get_piece_color(piece) == color:
                    piece_moves = self.get_piece_moves(i, j)
                    for to_row, to_col in piece_moves:
                        # Test if move is legal (doesn't put own king in check)
                        saved_board = deepcopy(self.board)
                        saved_king_pos = self.white_king_pos if color == 'white' else self.black_king_pos
                        saved_en_passant = self.en_passant_target
                        
                        # Temporarily make move
                        self.board[to_row][to_col] = piece
                        self.board[i][j] = ' '
                        if piece == 'K':
                            self.white_king_pos = (to_row, to_col)
                        elif piece == 'k':
                            self.black_king_pos = (to_row, to_col)
                        
                        # Check if king is in check
                        if not self.is_in_check(color):
                            moves.append((i, j, to_row, to_col))
                        
                        # Restore board
                        self.board = saved_board
                        if piece == 'K':
                            self.white_king_pos = saved_king_pos
                        elif piece == 'k':
                            self.black_king_pos = saved_king_pos
                        self.en_passant_target = saved_en_passant
        
        return moves
    
    def is_checkmate(self, color: str) -> bool:
        """Check if the given color is in checkmate."""
        if not self.is_in_check(color):
            return False
        return len(self.get_all_valid_moves(color)) == 0
    
    def is_stalemate(self, color: str) -> bool:
        """Check if the given color is in stalemate."""
        if self.is_in_check(color):
            return False
        return len(self.get_all_valid_moves(color)) == 0
    
    def evaluate_board(self) -> int:
        """Evaluate the board position. Positive favors white, negative favors black."""
        score = 0
        
        for i in range(8):
            for j in range(8):
                piece = self.board[i][j]
                if piece == ' ':
                    continue
                
                # Material value
                score += PIECE_VALUES[piece]
                
                # Positional bonuses
                piece_type = piece.upper()
                row = i if self.is_black_piece(piece) else 7 - i
                
                if piece_type == 'P':
                    bonus = PAWN_TABLE[row][j]
                    score += bonus if self.is_white_piece(piece) else -bonus
                elif piece_type == 'N':
                    bonus = KNIGHT_TABLE[row][j]
                    score += bonus if self.is_white_piece(piece) else -bonus
                elif piece_type == 'B':
                    bonus = BISHOP_TABLE[row][j]
                    score += bonus if self.is_white_piece(piece) else -bonus
                elif piece_type == 'K':
                    bonus = KING_TABLE[row][j]
                    score += bonus if self.is_white_piece(piece) else -bonus
        
        return score
    
    def get_ai_move(self, difficulty: int) -> Optional[Tuple[int, int, int, int]]:
        """Get AI move based on difficulty level."""
        valid_moves = self.get_all_valid_moves(self.current_player)
        
        if not valid_moves:
            return None
        
        if difficulty == 1:  # Easy - Random move
            return random.choice(valid_moves)
        
        # Evaluate all moves
        move_scores = []
        
        for from_row, from_col, to_row, to_col in valid_moves:
            # Make move
            saved_board = deepcopy(self.board)
            saved_king_pos = self.white_king_pos if self.current_player == 'white' else self.black_king_pos
            piece = self.board[from_row][from_col]
            captured = self.board[to_row][to_col]
            
            self.board[to_row][to_col] = piece
            self.board[from_row][from_col] = ' '
            if piece == 'K':
                self.white_king_pos = (to_row, to_col)
            elif piece == 'k':
                self.black_king_pos = (to_row, to_col)
            
            # Evaluate position
            score = self.evaluate_board()
            if self.current_player == 'black':
                score = -score
            
            # Bonus for captures
            if captured != ' ':
                score += abs(PIECE_VALUES[captured]) // 10
            
            # Bonus for checks
            opponent = 'black' if self.current_player == 'white' else 'white'
            if self.is_in_check(opponent):
                score += 50
            
            move_scores.append((from_row, from_col, to_row, to_col, score))
            
            # Restore board
            self.board = saved_board
            if piece == 'K':
                self.white_king_pos = saved_king_pos
            elif piece == 'k':
                self.black_king_pos = saved_king_pos
        
        # Sort by score
        move_scores.sort(key=lambda x: x[4], reverse=True)
        
        if difficulty == 2:  # Medium
            # 60% best move, 40% from top 5
            if random.random() < 0.6:
                selected = move_scores[0]
            else:
                top_moves = move_scores[:min(5, len(move_scores))]
                selected = random.choice(top_moves)
        else:  # Hard (difficulty 3)
            # 90% best move, 10% from top 3
            if random.random() < 0.9:
                selected = move_scores[0]
            else:
                top_moves = move_scores[:min(3, len(move_scores))]
                selected = random.choice(top_moves)
        
        return (selected[0], selected[1], selected[2], selected[3])
    
    def play_ai_turn(self, ai_name: str, difficulty: int):
        """Play AI turn."""
        print(f"{ai_name} is thinking", end="", flush=True)
        
        delay = 0.3 if difficulty == 1 else (0.5 if difficulty == 2 else 0.7)
        
        for _ in range(3):
            print(".", end="", flush=True)
            time.sleep(delay / 3)
        print()
        
        move = self.get_ai_move(difficulty)
        
        if move:
            from_row, from_col, to_row, to_col = move
            piece = self.board[from_row][from_col]
            
            if self.make_move(from_row, from_col, to_row, to_col):
                from_pos = chr(ord('a') + from_col) + str(8 - from_row)
                to_pos = chr(ord('a') + to_col) + str(8 - to_row)
                print(f"{ai_name} plays: {piece.upper()}{from_pos} → {to_pos}")
                time.sleep(0.5)
                return True
        
        return False
    
    def get_difficulty_name(self, diff: int) -> str:
        """Get difficulty name."""
        if diff == 1:
            return "Easy"
        elif diff == 2:
            return "Medium"
        return "Hard"
    
    def reset_game(self):
        """Reset the game to initial state."""
        self.board = self.init_board()
        self.current_player = 'white'
        self.move_history = []
        self.captured_pieces = {'white': [], 'black': []}
        self.winner = None
        self.white_king_pos = (7, 4)
        self.black_king_pos = (0, 4)
        self.white_can_castle_kingside = True
        self.white_can_castle_queenside = True
        self.black_can_castle_kingside = True
        self.black_can_castle_queenside = True
        self.en_passant_target = None
    
    def play_ai_vs_ai(self):
        """Play AI vs AI game."""
        self.reset_game()
        
        print("\n=== AI vs AI Chess Match ===")
        print(f"White AI: {self.get_difficulty_name(self.difficulty_ai1)}")
        print(f"Black AI: {self.get_difficulty_name(self.difficulty_ai2)}")
        print("Starting in 2 seconds...\n")
        time.sleep(2)
        
        max_moves = 200  # Prevent infinite games
        move_count = 0
        
        while move_count < max_moves:
            self.display_board()
            
            # Check game over conditions
            if self.is_checkmate(self.current_player):
                self.winner = 'black' if self.current_player == 'white' else 'white'
                break
            
            if self.is_stalemate(self.current_player):
                self.winner = 'draw'
                break
            
            # Get AI difficulty
            ai_name = f"White AI ({self.get_difficulty_name(self.difficulty_ai1)})" if self.current_player == 'white' else f"Black AI ({self.get_difficulty_name(self.difficulty_ai2)})"
            ai_diff = self.difficulty_ai1 if self.current_player == 'white' else self.difficulty_ai2
            
            # Play AI turn
            if not self.play_ai_turn(ai_name, ai_diff):
                self.winner = 'draw'
                break
            
            # Switch player
            self.current_player = 'black' if self.current_player == 'white' else 'white'
            move_count += 1
        
        if move_count >= max_moves:
            self.winner = 'draw'
        
        # Display final position
        self.display_board()
        
        # Show result
        print("\n" + "=" * 50)
        if self.winner == 'white':
            print("   CHECKMATE! White AI Wins!")
            self.white_wins += 1
        elif self.winner == 'black':
            print("   CHECKMATE! Black AI Wins!")
            self.black_wins += 1
        else:
            print("   DRAW!")
            self.draws += 1
        print("=" * 50 + "\n")
        
        self.total_games += 1
        self.show_statistics()
    
    def show_statistics(self):
        """Display game statistics."""
        print("\n=== Game Statistics ===")
        print(f"White AI Wins: {self.white_wins}")
        print(f"Black AI Wins: {self.black_wins}")
        print(f"Draws: {self.draws}")
        print(f"Total Games: {self.total_games}")
        
        if self.total_games > 0:
            print(f"\nWhite Win Rate: {self.white_wins / self.total_games * 100:.1f}%")
            print(f"Black Win Rate: {self.black_wins / self.total_games * 100:.1f}%")
            print(f"Draw Rate: {self.draws / self.total_games * 100:.1f}%")
        print("=" * 23 + "\n")
    
    def show_menu(self):
        """Display main menu."""
        print("\n" + "=" * 50)
        print("              CHESS AI vs AI")
        print("=" * 50)
        print("1. Watch AI vs AI")
        print(f"2. Set White AI Difficulty (Current: {self.get_difficulty_name(self.difficulty_ai1)})")
        print(f"3. Set Black AI Difficulty (Current: {self.get_difficulty_name(self.difficulty_ai2)})")
        print("4. Show Statistics")
        print("5. Reset Statistics")
        print("6. Exit")
        print("=" * 50)
        print("Enter your choice: ", end="")
    
    def run(self):
        """Main game loop."""
        running = True
        
        while running:
            self.show_menu()
            try:
                choice = int(input())
            except ValueError:
                print("Invalid choice. Please try again.")
                time.sleep(1)
                continue
            
            if choice == 1:
                self.play_ai_vs_ai()
                input("\nPress Enter to continue...")
            elif choice == 2:
                try:
                    diff = int(input("Enter difficulty (1=Easy, 2=Medium, 3=Hard): "))
                    self.difficulty_ai1 = max(1, min(3, diff))
                    print(f"White AI difficulty set to {self.get_difficulty_name(self.difficulty_ai1)}")
                    time.sleep(1)
                except ValueError:
                    print("Invalid input.")
                    time.sleep(1)
            elif choice == 3:
                try:
                    diff = int(input("Enter difficulty (1=Easy, 2=Medium, 3=Hard): "))
                    self.difficulty_ai2 = max(1, min(3, diff))
                    print(f"Black AI difficulty set to {self.get_difficulty_name(self.difficulty_ai2)}")
                    time.sleep(1)
                except ValueError:
                    print("Invalid input.")
                    time.sleep(1)
            elif choice == 4:
                self.show_statistics()
                input("\nPress Enter to continue...")
            elif choice == 5:
                self.white_wins = 0
                self.black_wins = 0
                self.draws = 0
                self.total_games = 0
                print("Statistics reset!")
                time.sleep(1)
            elif choice == 6:
                running = False
                print("\nThanks for watching!")
            else:
                print("Invalid choice. Please try again.")
                time.sleep(1)


if __name__ == "__main__":
    game = Chess()
    game.run()
