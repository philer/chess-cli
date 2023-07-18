#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <string>

using namespace std;

enum Color { red, green, blue };
const Color COLORS[3] = {red, green, blue};

enum Count { one, two, three };
const Count COUNTS[3] = {one, two, three};

enum Fill { full, half, empty };
const Fill FILLS[3] = {full, half, Fill::empty};

enum Symbol { tilde, square, ellipsis };
const Symbol SYMBOLS[3] = {tilde, square, ellipsis};

struct Card {
  Color color;
  Count count;
  Fill fill;
  Symbol symbol;
};

typedef Card Deck[81];

void init_deck(Deck &deck) {
  for (int i = 0; i < 81; ++i) {
    // deck[i] = {color : COLORS[i % 3]};
    deck[i] = {};
    deck[i].color = COLORS[i % 3];
    deck[i].count = COUNTS[(i / 3) % 3];
    deck[i].fill = FILLS[(i / 9) % 3];
    deck[i].symbol = SYMBOLS[(i / 27) % 3];
  }
}

string card_to_string(const Card &card) {
  char str[5] = "....";
  str[0] = "rgb"[card.color];
  str[1] = "123"[card.count];
  str[2] = "fhe"[card.fill];
  str[3] = "tse"[card.symbol];
  // switch(card.color) {
  // case red:
  //   str[1] = 'r';
  //   break
  // }
  // std::cout << str;
  return str;
}

void print_table(const Card table[]) {}

int main() {
  Deck deck;
  init_deck(deck);
  cout << card_to_string(deck[26]);
}
