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
  white = true,
  black = false,
};

enum Piece : uint8_t {
  pawn,
  knight,
  bishop,
  rook,
  queen,
  king,
};

struct ColorPiece {
  Color color;
  Piece piece;
};

const ColorPiece WHITE_PAWN = {white, pawn};
const ColorPiece WHITE_KNIGHT = {white, knight};
const ColorPiece WHITE_BISHOP = {white, bishop};
const ColorPiece WHITE_ROOK = {white, rook};
const ColorPiece WHITE_QUEEN = {white, queen};
const ColorPiece WHITE_KING = {white, king};
const ColorPiece BLACK_PAWN = {black, pawn};
const ColorPiece BLACK_KNIGHT = {black, knight};
const ColorPiece BLACK_BISHOP = {black, bishop};
const ColorPiece BLACK_ROOK = {black, rook};
const ColorPiece BLACK_QUEEN = {black, queen};
const ColorPiece BLACK_KING = {black, king};

bool operator==(const ColorPiece &a, const ColorPiece &b) {
  return a.color == b.color && a.piece == b.piece;
}

typedef array<array<optional<ColorPiece>, 8>, 8> Board;

Board create_board() {
  Board board;
  // empty squares
  for (uint8_t rank = 2; rank < 6; ++rank) {
    for (uint8_t file = 0; file < 8; ++file) {
      board[file][rank] = nullopt;
    }
  }

  // pawns
  for (uint8_t file = 0; file < 8; ++file) {
    board[file][1] = WHITE_PAWN;
    board[file][6] = BLACK_PAWN;
  }

  // white pieces
  board[0][0] = WHITE_ROOK;
  board[1][0] = WHITE_KNIGHT;
  board[2][0] = WHITE_BISHOP;
  board[3][0] = WHITE_QUEEN;
  board[4][0] = WHITE_KING;
  board[5][0] = WHITE_BISHOP;
  board[6][0] = WHITE_KNIGHT;
  board[7][0] = WHITE_ROOK;

  // black pieces
  board[0][7] = BLACK_ROOK;
  board[1][7] = BLACK_KNIGHT;
  board[2][7] = BLACK_BISHOP;
  board[3][7] = BLACK_QUEEN;
  board[4][7] = BLACK_KING;
  board[5][7] = BLACK_BISHOP;
  board[6][7] = BLACK_KNIGHT;
  board[7][7] = BLACK_ROOK;

  return board;
}

// Disallowing lower case piece letters for now.
// Ambiguous case: "bc6" could be a bishop or a pawn capture
static const string PIECE_CHARS = "NBRQK";

ColorPiece get_piece(const char piece_character, const Color color) {
  switch (piece_character) {
    case 'K':
      return {color, king};
    case 'Q':
      return {color, queen};
    case 'R':
      return {color, rook};
    case 'B':
      return {color, bishop};
    case 'N':
      return {color, knight};
    case 'P':
      return {color, pawn};
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

Square get_square(const char file, const char rank) {
  return Square{
      static_cast<uint8_t>((file - 'a')), static_cast<uint8_t>((rank - '1'))};
}

string to_string(const Square &square) {
  return {
      static_cast<char>((square.file + 'a')),
      static_cast<char>((square.rank + '1'))};
}

bool exists(const Square &square) {
  return 0 <= square.file && square.file <= 7 && 0 <= square.rank
         && square.rank <= 7;
}

optional<ColorPiece> find_piece(const Board &board, const Square &square) {
  return board[square.file][square.rank];
}

bool has_piece(
    const Board &board, const Square &square, const ColorPiece &piece
) {
  return board[square.file][square.rank] == piece;
}

template <size_t N>
vector<Square> find_line_moving_pieces(
    const Board &board,
    const Square &target_square,
    const ColorPiece &piece,
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
      const optional<ColorPiece> found_piece = find_piece(board, square);
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

vector<Square> find_bishops(
    const Board &board, const Square &target_square, const ColorPiece &piece
) {
  return find_line_moving_pieces<4>(
      board, target_square, piece, {{{-1, -1}, {+1, -1}, {-1, +1}, {+1, +1}}}
  );
}

vector<Square> find_rooks(
    const Board &board, const Square &target_square, const ColorPiece &piece
) {
  return find_line_moving_pieces<4>(
      board, target_square, piece, {{{0, -1}, {0, +1}, {-1, 0}, {+1, 0}}}
  );
}

vector<Square> find_queens(
    const Board &board, const Square &target_square, const ColorPiece &piece
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

vector<Square> find_direct_moving_pieces(
    const Board &board,
    const Square &target_square,
    const ColorPiece &piece,
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

vector<Square> find_kings(
    const Board &board, const Square &target_square, const ColorPiece &piece
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

vector<Square> find_knights(
    const Board &board, const Square &target_square, const ColorPiece &piece
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

vector<Square> find_pieces(
    const Board &board, const Square &target_square, const ColorPiece &piece
) {
  switch (piece.piece) {
    case knight:
      return find_knights(board, target_square, piece);
    case bishop:
      return find_bishops(board, target_square, piece);
    case rook:
      return find_rooks(board, target_square, piece);
    case queen:
      return find_queens(board, target_square, piece);
    case king:
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
  ColorPiece piece;
  optional<ColorPiece> capture;
  optional<ColorPiece> promotion;
};

// TODO int8_tened pawn captures ("exd", "ed")
const regex PAWN_MOVE_PATTERN{"^[a-h][1-8]$"};
const regex PAWN_CAPTURE_PATTERN{"^[a-h]x[a-h][1-8]$"};
const regex PAWN_PROMOTION_PATTERN{"^[a-h][1-8][NBRQ]$"};
const regex PAWN_CAPTURE_PROMOTION_PATTERN{"^[a-h]x[a-h][1-8][NBRQ]$"};

const regex PIECE_MOVE_PATTERN{"^[NBRQK][a-h][1-8]$"};
const regex FILE_PIECE_MOVE_PATTERN{"^[NBRQK][a-h][a-h][1-8]$"};
const regex RANK_PIECE_MOVE_PATTERN{"^[NBRQK][1-8][a-h][1-8]$"};
const regex SQUARE_PIECE_MOVE_PATTERN{"^[NBRQK][a-h][1-8][a-h][1-8]$"};

const regex PIECE_CAPTURE_PATTERN{"^[NBRQK]x[a-h][1-8]$"};
const regex FILE_PIECE_CAPTURE_PATTERN{"^[NBRQK][a-h]x[a-h][1-8]$"};
const regex RANK_PIECE_CAPTURE_PATTERN{"^[NBRQK][1-8]x[a-h][1-8]$"};
const regex SQUARE_PIECE_CAPTURE_PATTERN{"^[NBRQK][a-h][1-8]x[a-h][1-8]$"};

Move decode_move(const Board &board, const string &move, const Color color) {
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
    if (color == mv.capture->color) {
      throw string("Can't capture your own piece.");
    }

  } else if (regex_match(move, PAWN_PROMOTION_PATTERN)) {  // "e8Q"
    mv.piece = get_piece('P', color);
    mv.to = get_square(move[0], move[1]);
    mv.promotion = get_piece(move[2], color);
    throw string("NOT IMPLEMENTED");  // TODO

  } else if (regex_match(move, PAWN_CAPTURE_PROMOTION_PATTERN)) {  // "dxe8Q"
    mv.piece = get_piece('P', color);
    mv.to = get_square(move[2], move[3]);
    mv.promotion = get_piece(move[4], color);
    throw string("NOT IMPLEMENTED");  // TODO

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
          + (color == mv.capture.value().color ? " by your own piece."
                                               : ", add 'x' to capture.");
    }

  } else if (regex_match(move, PIECE_CAPTURE_PATTERN)) {  // "Qxe4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[2], move[3]);
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
    } else if (color == mv.capture->color) {
      throw string("Can't capture your own piece.");
    }

  } else if (regex_match(move, FILE_PIECE_MOVE_PATTERN)) {  // "Qde4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[2], move[3]);
    throw string("NOT IMPLEMENTED");  // TODO

  } else if (regex_match(move, FILE_PIECE_CAPTURE_PATTERN)) {  // "Qdxe4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[3], move[4]);
    throw string("NOT IMPLEMENTED");  // TODO

  } else if (regex_match(move, RANK_PIECE_MOVE_PATTERN)) {  // "Q3e4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[2], move[3]);
    throw string("NOT IMPLEMENTED");  // TODO

  } else if (regex_match(move, RANK_PIECE_CAPTURE_PATTERN)) {  // "Q3xe4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[3], move[4]);
    throw string("NOT IMPLEMENTED");  // TODO

  } else if (regex_match(move, SQUARE_PIECE_MOVE_PATTERN)) {  // "Qd3e4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[3], move[4]);
    throw string("NOT IMPLEMENTED");  // TODO

  } else if (regex_match(move, SQUARE_PIECE_CAPTURE_PATTERN)) {  // "Qd3xe4"
    mv.piece = get_piece(move[0], color);
    mv.to = get_square(move[4], move[5]);
    throw string("NOT IMPLEMENTED");  // TODO

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

void apply_move(Board &board, const Move &move) {
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

const map<Piece, array<string, 2>> UTF8_PIECES{
    {king, {"♚", "♔"}},
    {queen, {"♛", "♕"}},
    {rook, {"♜", "♖"}},
    {bishop, {"♝", "♗"}},
    {knight, {"♞", "♘"}},
    {pawn, {"♟︎", "♙"}},
};

const uint8_t FORWARD_8[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const uint8_t REVERSE_8[8] = {7, 6, 5, 4, 3, 2, 1, 0};

const uint8_t BOARD_HEADER_HEIGHT = 2;
const uint8_t BOARD_CONTENT_HEIGHT = 8;
const uint8_t BOARD_FOOTER_HEIGHT = 1;
const uint8_t BOARD_HEIGHT =
    BOARD_HEADER_HEIGHT + BOARD_CONTENT_HEIGHT + BOARD_FOOTER_HEIGHT;

array<string, BOARD_HEIGHT> board_to_lines(
    const Board &board, const Color color = white
) {
  array<string, BOARD_HEIGHT> lines;
  lines[0] = color ? "       WHITE        " : "       BLACK        ";
  lines[1] = color ? "  a b c d e f g h   " : "  h g f e d c b a   ";
  lines[10] = color ? "  a b c d e f g h   " : "  h g f e d c b a   ";

  Color square_color = black;
  for (const uint8_t rank : color ? REVERSE_8 : FORWARD_8) {
    const uint8_t line = (color ? 7 - rank : rank) + BOARD_HEADER_HEIGHT;
    lines[line] = to_string(rank + 1) + " ";
    for (const uint8_t file : color ? FORWARD_8 : REVERSE_8) {
      optional<ColorPiece> piece = board[file][rank];
      string piece_character;
      if (piece) {
        if (square_color == black) {
          piece->color = static_cast<Color>(!piece->color);
        }
        piece_character = UTF8_PIECES.at(piece->piece)[piece->color];
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

template <size_t size>
array<string, size> concat_lines(
    const array<string, size> &a, const array<string, size> &b
) {
  array<string, size> result = {};
  for (int i = 0; i < size; ++i) {
    result[i] = a[i] + b[i];
  }
  return result;
}

template <size_t size> string join_lines(const array<string, size> &lines) {
  string result = "";
  for (string line : lines) {
    result += line + "\n";
  }
  return result;
}

void print_board(const Board &board) {
  array<string, BOARD_HEIGHT> gap;
  fill(gap.begin(), gap.end(), "   ");
  cout << join_lines(concat_lines(
      concat_lines<BOARD_HEIGHT>(board_to_lines(board), gap),
      board_to_lines(board, black)
  ));
}

int main() {
  Board board = create_board();
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
