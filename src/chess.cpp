#include <array>
#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <regex>
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

enum Piece : ushort {
  black_pawn = 0 << 1,
  black_knight = 1 << 1,
  black_bishop = 2 << 1,
  black_rook = 3 << 1,
  black_queen = 4 << 1,
  black_king = 5 << 1,
  white_pawn = 1 | black_pawn,
  white_knight = 1 | black_knight,
  white_bishop = 1 | black_bishop,
  white_rook = 1 | black_rook,
  white_queen = 1 | black_queen,
  white_king = 1 | black_king,
};

const bool is_white(Piece piece) { return piece & 1; }

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

const Square get_square(const char file, const char rank) {
  return Square{static_cast<ushort>((file - 'a')),
                static_cast<ushort>((rank - '1'))};
}

const string to_string(Square square) {
  return {static_cast<char>((square.file + 'a')),
          static_cast<char>((square.rank + '1'))};
}

const optional<Piece> find_piece(const Board board, const Square square) {
  return board[square.file][square.rank];
}

const bool has_piece(const Board board, const Square square,
                     const Piece piece) {
  return board[square.file][square.rank] == piece;
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

// TODO shortened pawn captures ("exd", "ed")
const regex PAWN_MOVE_PATTERN{"[a-h][1-8]"};
const regex PAWN_CAPTURE_PATTERN{"[a-h]x[a-h][1-8]"};
const regex PAWN_PROMOTION_PATTERN{"[a-h][1-8][NBRQ]"};
const regex PAWN_CAPTURE_PROMOTION_PATTERN{"[a-h]x[a-h][1-8][NBRQ]"};

const regex PIECE_MOVE_PATTERN{"[NBRQK][a-h][1-8]"};
const regex FILE_PIECE_MOVE_PATTERN{"[NBRQK][a-h][a-h][1-8]"};
const regex RANK_PIECE_MOVE_PATTERN{"[NBRQK][1-8][a-h][1-8]"};
const regex SQUARE_PIECE_MOVE_PATTERN{"[NBRQK][a-h][1-8][a-h][1-8]"};

const regex PIECE_CAPTURE_PATTERN{"[NBRQK]x[a-h][1-8]"};
const regex FILE_PIECE_CAPTURE_PATTERN{"[NBRQK][a-h]x[a-h][1-8]"};
const regex RANK_PIECE_CAPTURE_PATTERN{"[NBRQK][1-8]x[a-h][1-8]"};
const regex SQUARE_PIECE_CAPTURE_PATTERN{"[NBRQK][a-h][1-8]x[a-h][1-8]"};

Move decode_move(const Board board, const string move, const bool as_white) {
  Move mv;
  mv.algebraic = move;
  mv.check = false;
  mv.castle_long = false;
  mv.castle_short = false;

  const short forwards = as_white ? 1 : -1;

  if (move == "0-0" || move == "O-O") {
    // TODO check castling rights
    mv.castle_short = true;

  } else if (move == "0-0-0" || move == "O-O-O") {
    // TODO check castling rights
    mv.castle_long = true;

  } else if (regex_match(move, PAWN_MOVE_PATTERN)) { // "e4"
    mv.piece = get_piece('P', as_white);
    mv.to = get_square(move[0], move[1]);
    mv.from.file = mv.to.file;

    if (board[mv.from.file][mv.to.rank - forwards] == mv.piece) {
      mv.from.rank = mv.to.rank - forwards;
    } else if ((as_white && mv.to.rank == 3 || !as_white && mv.to.rank == 4) &&
               !board[mv.from.file][mv.to.rank - forwards] &&
               board[mv.from.file][mv.to.rank - 2 * forwards] == mv.piece) {
      mv.from.rank = mv.to.rank - 2 * forwards;
    } else {
      throw string(
          "There is no eligible Pawn on " +
          to_string(Square{mv.from.file,
                           static_cast<ushort>(mv.to.rank - forwards)}) +
          " or " +
          to_string(Square{mv.from.file,
                           static_cast<ushort>(mv.to.rank - 2 * forwards)}) +
          ".");
    }

  } else if (regex_match(move, PAWN_CAPTURE_PATTERN)) { // "dxe4"
    mv.piece = get_piece('P', as_white);
    mv.to = get_square(move[2], move[3]);
    mv.from = get_square(move[0], move[3] - forwards);
    if (abs(mv.from.file - mv.to.file) != 1) {
      throw string("Pawn must move one square diagonally when capturing.");
    }
    if (!has_piece(board, mv.from, mv.piece)) {
      throw string("No eligible Pawn on " + to_string(mv.from) + ".");
    }
    mv.capture = find_piece(board, mv.to);
    // TODO en passant
    if (!mv.capture) {
      throw string("There is nothing to capture on " + to_string(mv.to) + ".");
    }
    if (as_white == is_white(mv.capture.value())) {
      throw string("Can't capture your own piece.");
    }

  } else if (regex_match(move, PAWN_PROMOTION_PATTERN)) { // "e8Q"
    mv.piece = get_piece('P', as_white);
    mv.to = get_square(move[0], move[1]);
    mv.promotion = get_piece(move[2], as_white);
    // TODO

  } else if (regex_match(move, PAWN_CAPTURE_PROMOTION_PATTERN)) { // "dxe8Q"
    mv.piece = get_piece('P', as_white);
    mv.to = get_square(move[2], move[3]);
    mv.promotion = get_piece(move[4], as_white);
    // TODO

  } else if (regex_match(move, PIECE_MOVE_PATTERN)) { // "Qe4"
    mv.piece = get_piece(move[0], as_white);
    mv.to = get_square(move[1], move[2]);
    // TODO

  } else if (regex_match(move, FILE_PIECE_MOVE_PATTERN)) { // "Qde4"
    mv.piece = get_piece(move[0], as_white);
    mv.to = get_square(move[2], move[3]);
    // TODO

  } else if (regex_match(move, RANK_PIECE_MOVE_PATTERN)) { // "Q3e4"
    mv.piece = get_piece(move[0], as_white);
    mv.to = get_square(move[2], move[3]);
    // TODO

  } else if (regex_match(move, SQUARE_PIECE_MOVE_PATTERN)) { // "Qd3e4"
    mv.piece = get_piece(move[0], as_white);
    mv.to = get_square(move[3], move[4]);
    // TODO

  } else if (regex_match(move, PIECE_CAPTURE_PATTERN)) { // "Qxe4"
    mv.piece = get_piece(move[0], as_white);
    mv.to = get_square(move[2], move[3]);
    // TODO

  } else if (regex_match(move, FILE_PIECE_CAPTURE_PATTERN)) { // "Qdxe4"
    mv.piece = get_piece(move[0], as_white);
    mv.to = get_square(move[3], move[4]);
    // TODO

  } else if (regex_match(move, RANK_PIECE_CAPTURE_PATTERN)) { // "Q3xe4"
    mv.piece = get_piece(move[0], as_white);
    mv.to = get_square(move[3], move[4]);
    // TODO

  } else if (regex_match(move, SQUARE_PIECE_CAPTURE_PATTERN)) { // "Qd3xe4"
    mv.piece = get_piece(move[0], as_white);
    mv.to = get_square(move[4], move[5]);
    // TODO

  } else {
    throw string("'" + move +
                 "' is not a known move format.\n"
                 "Note: Do not add any special characters like + # = "
                 "to indicate check/mate/promotion.");
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
