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

enum Color : bool {
  black = false,
  white = true,
};

enum Piece : uint8_t {
  black_pawn = 0 << 1,
  black_knight = 1 << 1,
  black_bishop = 2 << 1,
  black_rook = 3 << 1,
  black_queen = 4 << 1,
  black_king = 5 << 1,
  white_pawn = white | black_pawn,
  white_knight = white | black_knight,
  white_bishop = white | black_bishop,
  white_rook = white | black_rook,
  white_queen = white | black_queen,
  white_king = white | black_king,
};

const Color get_color(Piece piece) {
  return static_cast<Color>(piece & static_cast<uint8_t>(white));
}

typedef optional<Piece> Board[8][8];

void init_board(Board board) {
  // empty squares
  for (uint8_t rank = 2; rank < 6; ++rank) {
    for (uint8_t file = 0; file < 8; ++file) {
      board[file][rank] = nullopt;
    }
  }

  // pawns
  for (uint8_t file = 0; file < 8; ++file) {
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

Piece get_piece(const char piece_character, const Color color) {
  switch (piece_character) {
    case 'K':
      return color ? Piece::white_king : Piece::black_king;
    case 'Q':
      return color ? Piece::white_queen : Piece::black_queen;
    case 'R':
      return color ? Piece::white_rook : Piece::black_rook;
    case 'B':
      return color ? Piece::white_bishop : Piece::black_bishop;
    case 'N':
      return color ? Piece::white_knight : Piece::black_knight;
    case 'P':
      return color ? Piece::white_pawn : Piece::black_pawn;
    default:
      string piece_str;
      piece_str = piece_character;
      throw string("Invalid piece '" + piece_str + "'");
  }
}

struct Square {
  uint8_t file;
  uint8_t rank;
};

const Square get_square(const char file, const char rank) {
  return Square{
      static_cast<uint8_t>((file - 'a')), static_cast<uint8_t>((rank - '1'))};
}

const string to_string(Square square) {
  return {
      static_cast<char>((square.file + 'a')),
      static_cast<char>((square.rank + '1'))};
}

const bool exists(Square square) {
  return 0 <= square.file && square.file <= 7 && 0 <= square.rank
         && square.rank <= 7;
}

const optional<Piece> find_piece(const Board board, const Square square) {
  return board[square.file][square.rank];
}

const bool has_piece(
    const Board board, const Square square, const Piece piece
) {
  return board[square.file][square.rank] == piece;
}

template <size_t N>
const vector<Square> find_line_moving_pieces(
    const Board board,
    const Square target_square,
    const Piece piece,
    const array<tuple<int8_t, int8_t>, N> directions
) {
  vector<Square> found;
  for (const auto [d_file, d_rank] : directions) {
    for (uint8_t offset = 1;; ++offset) {
      Square square = {
          static_cast<uint8_t>(target_square.file + offset * d_file),
          static_cast<uint8_t>(target_square.rank + offset * d_rank)};
      if (!exists(square)) {
        break;
      }
      const optional<Piece> found_piece = find_piece(board, square);
      if (found_piece) {
        if (found_piece == piece) {
          found.push_back(square);
        }
        break;
      }
    }
  }
  return found;
}

const vector<Square> find_bishops(
    const Board board, const Square target_square, const Piece piece
) {
  return find_line_moving_pieces<4>(
      board, target_square, piece, {{{-1, -1}, {+1, -1}, {-1, +1}, {+1, +1}}}
  );
}

const vector<Square> find_rooks(
    const Board board, const Square target_square, const Piece piece
) {
  return find_line_moving_pieces<4>(
      board, target_square, piece, {{{0, -1}, {0, +1}, {-1, 0}, {+1, 0}}}
  );
}

const vector<Square> find_queens(
    const Board board, const Square target_square, const Piece piece
) {
  return find_line_moving_pieces<8>(
      board,
      target_square,
      piece,
      {{{-1, -1},
        {+1, -1},
        {-1, +1},
        {+1, +1},
        {0, -1},
        {0, +1},
        {-1, 0},
        {+1, 0}}}
  );
}

const vector<Square> find_direct_moving_pieces(
    const Board board,
    const Square target_square,
    const Piece piece,
    const array<tuple<int8_t, int8_t>, 8> moves
) {
  vector<Square> found;
  for (const auto [d_file, d_rank] : moves) {
    Square square = {
        static_cast<uint8_t>(target_square.file + d_file),
        static_cast<uint8_t>(target_square.rank + d_rank)};
    if (exists(square) && find_piece(board, square) == piece) {
      found.push_back(square);
    }
  }
  return found;
}

const vector<Square> find_kings(
    const Board board, const Square target_square, const Piece piece
) {
  return find_direct_moving_pieces(
      board,
      target_square,
      piece,
      {{{-1, -1},
        {+1, -1},
        {-1, +1},
        {+1, +1},
        {0, -1},
        {0, +1},
        {-1, 0},
        {+1, 0}}}
  );
}

const vector<Square> find_knights(
    const Board board, const Square target_square, const Piece piece
) {
  return find_direct_moving_pieces(
      board,
      target_square,
      piece,
      {{{+1, +2},
        {+1, -2},
        {-1, +2},
        {-1, -2},
        {+2, +1},
        {+2, -1},
        {-2, +1},
        {-2, -1}}}
  );
}

const vector<Square> find_pieces(
    const Board board, const Square target_square, const Piece piece
) {
  switch (piece & ~white) {
    case black_knight:
      return find_knights(board, target_square, piece);
    case black_bishop:
      return find_bishops(board, target_square, piece);
    case black_rook:
      return find_rooks(board, target_square, piece);
    case black_queen:
      return find_queens(board, target_square, piece);
    case black_king:
      return find_kings(board, target_square, piece);
    default:
      throw string("Something went wrong!");
  }
}

struct Move {
  string algebraic;
  Square from;
  Square to;
  bool check;
  bool castle_long;
  bool castle_int8_t;
  // bool en_passant;  // TODO
  Piece piece;
  optional<Piece> capture;
  optional<Piece> promotion;
};

// TODO int8_tened pawn captures ("exd", "ed")
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

Move decode_move(const Board board, const string move, const Color color) {
  Move mv;
  mv.algebraic = move;
  mv.check = false;
  mv.castle_long = false;
  mv.castle_int8_t = false;

  const int8_t forwards = color ? 1 : -1;

  if (move == "0-0" || move == "O-O") {
    // TODO check castling rights
    mv.castle_int8_t = true;

  } else if (move == "0-0-0" || move == "O-O-O") {
    // TODO check castling rights
    mv.castle_long = true;

  } else if (regex_match(move, PAWN_MOVE_PATTERN)) {  // "e4"
    mv.piece = get_piece('P', color);
    mv.to = get_square(move[0], move[1]);
    mv.from.file = mv.to.file;

    if (board[mv.from.file][mv.to.rank - forwards] == mv.piece) {
      mv.from.rank = mv.to.rank - forwards;
    } else if ((color && mv.to.rank == 3 || !color && mv.to.rank == 4) &&
               !board[mv.from.file][mv.to.rank - forwards] &&
               board[mv.from.file][mv.to.rank - 2 * forwards] == mv.piece) {
      mv.from.rank = mv.to.rank - 2 * forwards;
    } else {
      throw string(
          "There is no eligible Pawn on "
          + to_string(Square{
              mv.from.file, static_cast<uint8_t>(mv.to.rank - forwards)})
          + " or "
          + to_string(Square{
              mv.from.file, static_cast<uint8_t>(mv.to.rank - 2 * forwards)})
          + "."
      );
    }

  } else if (regex_match(move, PAWN_CAPTURE_PATTERN)) {  // "dxe4"
    mv.piece = get_piece('P', color);
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
    if (color == get_color(mv.capture.value())) {
      throw string("Can't capture your own piece.");
    }

  } else if (regex_match(move, PAWN_PROMOTION_PATTERN)) {  // "e8Q"
    mv.piece = get_piece('P', color);
    mv.to = get_square(move[0], move[1]);
    mv.promotion = get_piece(move[2], color);
    // TODO

  } else if (regex_match(move, PAWN_CAPTURE_PROMOTION_PATTERN)) {  // "dxe8Q"
    mv.piece = get_piece('P', color);
    mv.to = get_square(move[2], move[3]);
    mv.promotion = get_piece(move[4], color);
    // TODO

  } else if (regex_match(move, PIECE_MOVE_PATTERN)) {  // "Qe4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[1], move[2]);
    const vector<Square> candidates = find_pieces(board, mv.to, mv.piece);
    switch (candidates.size()) {
      case 1:
        mv.from = candidates[0];
        break;
      case 0:
        throw string("No candidate pieces available.");
      default:
        throw string("Ambiguous move: multiple pieces available.");
    }
    mv.capture = find_piece(board, mv.to);
    if (mv.capture) {
      throw string("Target square is occupied")
          + (color == get_color(mv.capture.value()) ? " by your own piece."
                                                    : ", add 'x' to capture.");
    }

  } else if (regex_match(move, FILE_PIECE_MOVE_PATTERN)) {  // "Qde4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[1], move[2]);
    const vector<Square> candidates = find_pieces(board, mv.to, mv.piece);
    switch (candidates.size()) {
      case 1:
        mv.from = candidates[0];
        break;
      case 0:
        throw string("No candidate pieces available.");
      default:
        throw string("Ambiguous move: multiple pieces available.");
    }
    mv.capture = find_piece(board, mv.to);
    if (!mv.capture) {
      throw string("Nothing to capture on " + to_string(mv.to));
    } else if (color == get_color(mv.capture.value())) {
      throw string("Can't capture your own piece.");
    }

  } else if (regex_match(move, RANK_PIECE_MOVE_PATTERN)) {  // "Q3e4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[2], move[3]);
    // TODO

  } else if (regex_match(move, SQUARE_PIECE_MOVE_PATTERN)) {  // "Qd3e4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[3], move[4]);
    // TODO

  } else if (regex_match(move, PIECE_CAPTURE_PATTERN)) {  // "Qxe4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[2], move[3]);
    // TODO

  } else if (regex_match(move, FILE_PIECE_CAPTURE_PATTERN)) {  // "Qdxe4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[3], move[4]);
    // TODO

  } else if (regex_match(move, RANK_PIECE_CAPTURE_PATTERN)) {  // "Q3xe4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[3], move[4]);
    // TODO

  } else if (regex_match(move, SQUARE_PIECE_CAPTURE_PATTERN)) {  // "Qd3xe4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[4], move[5]);
    // TODO

  } else {
    throw string(
        "'" + move
        + "' is not a known move format.\n"
          "Note: Do not add any special characters like + # = "
          "to indicate check/mate/promotion."
    );
  }
  return mv;
}

void apply_move(Board board, const Move move) {
  if (move.castle_long) {
    // TODO
  } else if (move.castle_int8_t) {
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
    {white_king, "♔"},
    {white_queen, "♕"},
    {white_rook, "♖"},
    {white_bishop, "♗"},
    {white_knight, "♘"},
    {white_pawn, "♙"},
    {black_king, "♚"},
    {black_queen, "♛"},
    {black_rook, "♜"},
    {black_bishop, "♝"},
    {black_knight, "♞"},
    {black_pawn, "♟︎"},
};
map<Piece, Piece> inverted_pieces{
    {white_king, black_king},
    {white_queen, black_queen},
    {white_rook, black_rook},
    {white_bishop, black_bishop},
    {white_knight, black_knight},
    {white_pawn, black_pawn},
    {black_king, white_king},
    {black_queen, white_queen},
    {black_rook, white_rook},
    {black_bishop, white_bishop},
    {black_knight, white_knight},
    {black_pawn, white_pawn},
};

const uint8_t forward8[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const uint8_t reverse8[8] = {7, 6, 5, 4, 3, 2, 1, 0};

const uint8_t BOARD_HEADER_HEIGHT = 2;
const uint8_t BOARD_CONTENT_HEIGHT = 8;
const uint8_t BOARD_FOOTER_HEIGHT = 1;
const uint8_t BOARD_HEIGHT =
    BOARD_HEADER_HEIGHT + BOARD_CONTENT_HEIGHT + BOARD_FOOTER_HEIGHT;

array<string, BOARD_HEIGHT> board_to_lines(
    const Board board, const Color color = white
) {
  array<string, BOARD_HEIGHT> lines;
  lines[0] = color ? "       WHITE        " : "       BLACK        ";
  lines[1] = color ? "  a b c d e f g h   " : "  h g f e d c b a   ";
  lines[10] = color ? "  a b c d e f g h   " : "  h g f e d c b a   ";

  Color square_color = black;
  for (const uint8_t rank : color ? reverse8 : forward8) {
    const uint8_t line = (color ? 7 - rank : rank) + BOARD_HEADER_HEIGHT;
    lines[line] = to_string(rank + 1) + " ";
    for (const uint8_t file : color ? forward8 : reverse8) {
      string piece_character;
      if (board[file][rank] != nullopt) {
        Piece piece = board[file][rank].value();
        if (square_color == black) {
          piece = inverted_pieces[piece];
        }
        piece_character = pieces[piece];
      } else {
        piece_character = " ";
      }
      if (square_color == black) {
        lines[line] += piece_character + " ";
      } else {
        lines[line] += ANSI_INVERT + piece_character + " " + ANSI_RESET;
      }
      square_color = static_cast<Color>(!square_color);
    }
    square_color = static_cast<Color>(!square_color);
    lines[line] += " " + to_string(rank + 1);
  }

  return lines;
}

string concat(string a, string b) { return a + b; }

template <size_t size>
array<string, size> concat_lines(
    const array<string, size> a, const array<string, size> b
) {
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
  cout << join_lines(concat_lines(
      concat_lines<BOARD_HEIGHT>(board_to_lines(board), gap),
      board_to_lines(board, black)
  ));
}

int main() {
  Board board;
  init_board(board);

  vector<string> game = {"d4", "d5", "e3", "c6"};
  Color color = white;
  uint move_no = 0;
  string move;
  Move decoded_move;

  bool exit = false;
  while (true) {
    if (color) {
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
        cout << (color ? "White> " : "Black> ");
        cin >> move;
        if (move == "exit" || move == "quit" || move == "q" || move == "") {
          exit = true;
        } else {
          decoded_move = decode_move(board, move, color);
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
    color = static_cast<Color>(!color);
  }

  cout << "\nBye." << endl;
  return 0;
}
