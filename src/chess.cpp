#include <array>
#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

using namespace std;

/**
 * Play interactive chess on the command line.
 *
 * This program was created for training purposes and is full of experiments –
 * read it at your own risk.
 *
 * Future ideas:
 * + highlight last move
 * + highlight check
 * + highlight available moves for a selected piece
 * */

enum Piece {
  white_king,
  white_queen,
  white_rook,
  white_bishop,
  white_knight,
  white_pawn,
  black_king,
  black_queen,
  black_rook,
  black_bishop,
  black_knight,
  black_pawn,
};

typedef optional<Piece> Board[8][8];

void init_board(Board board) {
  // empty squares
  for (ushort rank = 2; rank < 6; ++rank) {
    for (ushort file = 0; file < 8; ++file) {
      board[file][rank] = nullopt;
    }
  }

  // pawns
  for (ushort file = 0; file < 8; ++file) {
    board[file][1] = white_pawn;
    board[file][6] = black_pawn;
  }

  // white pieces
  board[0][0] = white_rook;
  board[1][0] = white_knight;
  board[2][0] = white_bishop;
  board[3][0] = white_queen;
  board[4][0] = white_king;
  board[5][0] = white_bishop;
  board[6][0] = white_knight;
  board[7][0] = white_rook;

  // black pieces
  board[0][7] = black_rook;
  board[1][7] = black_knight;
  board[2][7] = black_bishop;
  board[3][7] = black_queen;
  board[4][7] = black_king;
  board[5][7] = black_bishop;
  board[6][7] = black_knight;
  board[7][7] = black_rook;
}

// Disallowing lower case piece letters for now.
// Ambiguous case: "bc6" could be a bishop or a pawn capture
static const string PIECE_CHARS = "NBRQK";

Piece get_piece(const char piece_character, const bool as_white) {
  switch (piece_character) {
  case 'K':
    return as_white ? Piece::white_king : Piece::black_king;
  case 'Q':
    return as_white ? Piece::white_queen : Piece::black_queen;
  case 'R':
    return as_white ? Piece::white_rook : Piece::black_rook;
  case 'B':
    return as_white ? Piece::white_bishop : Piece::black_bishop;
  case 'N':
    return as_white ? Piece::white_knight : Piece::black_knight;
  case 'P':
    return as_white ? Piece::white_pawn : Piece::black_pawn;
  default:
    string piece_str;
    piece_str = piece_character;
    throw string("Invalid piece '" + piece_str + "'");
  }
}

struct Square {
  ushort file;
  ushort rank;
};

static const string FILE_CHARS = "abcdefgh";
static const string RANK_CHARS = "12345678";

inline Square get_square(const char file, const char rank) {
  if (FILE_CHARS.find(file) == string::npos ||
      RANK_CHARS.find(rank) == string::npos) {
    throw string("Not a valid square: '") + file + rank + "'";
  }
  return Square{static_cast<ushort>((file - 'a')),
                static_cast<ushort>((rank - '1'))};
}

inline string to_string(Square square) {
  return {static_cast<char>((square.file + 'a')),
          static_cast<char>((square.rank + '1'))};
}

struct Move {
  string algebraic;
  Square from;
  Square to;
  bool check;
  bool castle_long;
  bool castle_short;
  // bool en_passant;  // TODO
  Piece piece;
  optional<Piece> capture;
  optional<Piece> promotion;
};

Move decode_move(const Board board, const string move, const bool as_white) {
  Move mv;
  mv.algebraic = move;
  mv.check = false;
  mv.castle_long = false;
  mv.castle_short = false;

  const short forwards = as_white ? 1 : -1;

  // Without check+
  string move_ = move;

  if (move_[move_.length() - 1] == '+') {
    mv.check = true;
    move_ = move_.substr(0, move_.length() - 1);
  }

  switch (move_.length()) {
    // TODO shortened pawn captures ("exd", "ed")
  case 2:
    // Pawn move, e.g. "e4";
    mv.piece = get_piece('P', as_white);
    mv.to = get_square(move_[0], move_[1]);
    mv.from.file = mv.to.file;

    if (board[mv.from.file][mv.to.rank - forwards] == mv.piece) {
      mv.from.rank = mv.to.rank - forwards;
    } else if ((as_white && mv.to.rank == 3 || !as_white && mv.to.rank == 4) &&
               !board[mv.from.file][mv.to.rank - forwards].has_value() &&
               board[mv.from.file][mv.to.rank - 2 * forwards] == mv.piece) {
      mv.from.rank = mv.to.rank - 2 * forwards;
    } else {
      throw string(
          "Invalid move: No eligible Pawn on " +
          to_string(Square{mv.from.file,
                           static_cast<ushort>(mv.to.rank - forwards)}) +
          " or " +
          to_string(Square{mv.from.file,
                           static_cast<ushort>(mv.to.rank - 2 * forwards)}));
    }
    break;
  case 3:
    if (move_ == "0-0" || move_ == "O-O") {
      // TODO check castling rights
      mv.castle_short = true;
    } else if (PIECE_CHARS.find(move_[0]) != string::npos) {
      // Basic piece move, e.g. "Nf6"
      mv.piece = get_piece(move_[0], as_white);
      mv.to = get_square(move_[1], move_[2]);
      // TODO check available piece
    } else if (PIECE_CHARS.find(move_[2]) != string::npos) {
      // Promotion
      mv.to = get_square(move_[0], move_[1]);
      mv.promotion = get_piece(move_[2], as_white);
      // TODO check available piece
    }
    break;
  case 4:
    if (move_[1] == 'x') {
      // Basic capture
      if (PIECE_CHARS.find(move_[0]) != string::npos) {
        mv.piece = get_piece(move_[0], as_white);
      } else if (FILE_CHARS.find(move_[0]) != string::npos) {
        mv.piece = get_piece('P', as_white);
        ushort file = move_[0] - 'a';
      } else {
        throw string("Invalid move '" + move_ + "'");
      }
    } else {
      // Piece move with qualified starting rank or file
      mv.piece = get_piece(move_[0], as_white);
      if (PIECE_CHARS.find(mv.piece) == string::npos) {
        throw string("Invalid move '" + move_ + "'");
      }
      if (FILE_CHARS.find(move_[1] != string::npos)) {
        mv.from.file = move_[1];
      } else if (RANK_CHARS.find(move_[1] != string::npos)) {
        ushort rank = move_[1] - '1';
      } else {
        throw string("Invalid move '" + move_ + "'");
      }
    }
    break;
  case 5:
    if (move_ == "0-0-0" || move_ == "O-O-O") {
      // TODO check castling rights
      mv.castle_long = true;
    } else if (move_[1] == 'x') {
      // Pawn capture with promotion ("dxe8Q")
      // TODO
    } else if (move_[2] == 'x') {
      // Piece capture with qualified starting rank or file ("Qhxe1")
      // TODO
    } else {
      // Piece move with qualified starting rank and file ("Qh4e1")
      // TODO
    }
    break;
  case 6:
    // Piece capture with qualified starting rank and file ""Qh4xe1""
    // TODO
    break;
  default:
    throw string("Invalid move '" + move_ + "'");
    break;
  }
  return mv;
}

void apply_move(Board board, const Move move) {
  if (move.castle_long) {
    // TODO
  } else if (move.castle_short) {
    // TODO
  }

  board[move.from.file][move.from.rank] = nullopt;
  board[move.to.file][move.to.rank] = move.promotion.value_or(move.piece);

  // TODO capture en passant
}

// const string ANSI_RED = "\033[31m";
const string ANSI_INVERT = "\033[0;0;7m";
const string ANSI_RESET = "\033[0m";

map<Piece, string> pieces{
    {white_king, "♔"},   {white_queen, "♕"},  {white_rook, "♖"},
    {white_bishop, "♗"}, {white_knight, "♘"}, {white_pawn, "♙"},
    {black_king, "♚"},   {black_queen, "♛"},  {black_rook, "♜"},
    {black_bishop, "♝"}, {black_knight, "♞"}, {black_pawn, "♟︎"},
};
map<Piece, Piece> inverted_pieces{
    {white_king, black_king},     {white_queen, black_queen},
    {white_rook, black_rook},     {white_bishop, black_bishop},
    {white_knight, black_knight}, {white_pawn, black_pawn},
    {black_king, white_king},     {black_queen, white_queen},
    {black_rook, white_rook},     {black_bishop, white_bishop},
    {black_knight, white_knight}, {black_pawn, white_pawn},
};

const ushort forward8[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const ushort reverse8[8] = {7, 6, 5, 4, 3, 2, 1, 0};

const ushort BOARD_HEADER_HEIGHT = 2;
const ushort BOARD_CONTENT_HEIGHT = 8;
const ushort BOARD_FOOTER_HEIGHT = 1;
const ushort BOARD_HEIGHT =
    BOARD_HEADER_HEIGHT + BOARD_CONTENT_HEIGHT + BOARD_FOOTER_HEIGHT;

array<string, BOARD_HEIGHT> board_to_lines(const Board board,
                                           const bool as_white = true) {
  array<string, BOARD_HEIGHT> lines;
  lines[0] = as_white ? "       WHITE        " : "       BLACK        ";
  lines[1] = as_white ? "  a b c d e f g h   " : "  h g f e d c b a   ";
  lines[10] = as_white ? "  a b c d e f g h   " : "  h g f e d c b a   ";

  bool black_square = true;
  for (const ushort rank : as_white ? reverse8 : forward8) {
    const ushort line = (as_white ? 7 - rank : rank) + BOARD_HEADER_HEIGHT;
    lines[line] = to_string(rank + 1) + " ";
    for (const ushort file : as_white ? forward8 : reverse8) {
      string piece_character;
      if (board[file][rank] != nullopt) {
        Piece piece = board[file][rank].value();
        if (black_square) {
          piece = inverted_pieces[piece];
        }
        piece_character = pieces[piece];
      } else {
        piece_character = " ";
      }
      if (black_square) {
        lines[line] += piece_character + " ";
      } else {
        lines[line] += ANSI_INVERT + piece_character + " " + ANSI_RESET;
      }
      black_square = !black_square;
    }
    black_square = !black_square;
    lines[line] += " " + to_string(rank + 1);
  }

  return lines;
}

string concat(string a, string b) { return a + b; }

template <size_t size>
array<string, size> concat_lines(const array<string, size> a,
                                 const array<string, size> b) {
  array<string, size> result = {};
  for (int i = 0; i < size; ++i) {
    result[i] = a[i] + b[i];
  }
  return result;
}

template <size_t size> string join_lines(const array<string, size> lines) {
  string result = "";
  for (string line : lines) {
    result += line + "\n";
  }
  return result;
}

void print_board(const Board board) {
  array<string, BOARD_HEIGHT> gap;
  fill(gap.begin(), gap.end(), "   ");
  cout << join_lines(concat_lines<BOARD_HEIGHT>(
      concat_lines<BOARD_HEIGHT>(board_to_lines(board), gap),
      board_to_lines(board, false)));
}

int main() {
  Board board;
  init_board(board);

  vector<string> game = {"d4", "d5", "e3", "c6"};
  bool is_white = true;
  uint move_no = 0;
  string move;
  Move decoded_move;

  bool exit = false;
  while (true) {
    if (is_white) {
      move_no += 1;
      cout << "                { Move " << move_no << " }" << endl;
    }
    print_board(board);
    cout << endl;

    while (true) {
      try {
        if (cin.eof()) {
          exit = true;
          break;
        }
        cout << (is_white ? "White> " : "Black> ");
        cin >> move;
        if (move == "exit" || move == "quit" || move == "q" || move == "") {
          exit = true;
        } else {
          decoded_move = decode_move(board, move, is_white);
        }
        break;
      } catch (string err) {
        cout << err << endl;
      }
    }
    if (exit) {
      break;
    }

    cout << endl;
    apply_move(board, decoded_move);
    is_white = !is_white;
  }

  cout << "\nBye." << endl;
  return 0;
}
