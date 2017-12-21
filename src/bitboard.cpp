/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2018 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>

#include "bitboard.h"
#include "misc.h"

uint8_t PopCnt16[1 << 16];
int SquareDistance[SQUARE_NB][SQUARE_NB];

Bitboard SquareBB[SQUARE_NB];
Bitboard FileBB[FILE_NB];
Bitboard RankBB[RANK_NB];
Bitboard AdjacentFilesBB[FILE_NB];
Bitboard ForwardRanksBB[COLOR_NB][RANK_NB];
Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
Bitboard LineBB[SQUARE_NB][SQUARE_NB];
Bitboard DistanceRingBB[SQUARE_NB][8];
Bitboard ForwardFileBB[COLOR_NB][SQUARE_NB];
Bitboard PassedPawnMask[COLOR_NB][SQUARE_NB];
Bitboard PawnAttackSpan[COLOR_NB][SQUARE_NB];
Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];

Magic RookMagics[SQUARE_NB];
Magic BishopMagics[SQUARE_NB];

namespace {

  Bitboard AttackTable[HasPext ? 107648 : 87988] = { 0 };

  struct MagicInit {
    Bitboard magic;
    unsigned offset;
  };

  MagicInit BishopMagicInit[SQUARE_NB] = {
    { 0xa7020080601803d8u, 60984 },
    { 0x13802040400801f1u, 66046 },
    { 0x0a0080181001f60cu, 32910 },
    { 0x1840802004238008u, 16369 },
    { 0xc03fe00100000000u, 42115 },
    { 0x24c00bffff400000u,   835 },
    { 0x0808101f40007f04u, 18910 },
    { 0x100808201ec00080u, 25911 },
    { 0xffa2feffbfefb7ffu, 63301 },
    { 0x083e3ee040080801u, 16063 },
    { 0xc0800080181001f8u, 17481 },
    { 0x0440007fe0031000u, 59361 },
    { 0x2010007ffc000000u, 18735 },
    { 0x1079ffe000ff8000u, 61249 },
    { 0x3c0708101f400080u, 68938 },
    { 0x080614080fa00040u, 61791 },
    { 0x7ffe7fff817fcff9u, 21893 },
    { 0x7ffebfffa01027fdu, 62068 },
    { 0x53018080c00f4001u, 19829 },
    { 0x407e0001000ffb8au, 26091 },
    { 0x201fe000fff80010u, 15815 },
    { 0xffdfefffde39ffefu, 16419 },
    { 0xcc8808000fbf8002u, 59777 },
    { 0x7ff7fbfff8203fffu, 16288 },
    { 0x8800013e8300c030u, 33235 },
    { 0x0420009701806018u, 15459 },
    { 0x7ffeff7f7f01f7fdu, 15863 },
    { 0x8700303010c0c006u, 75555 },
    { 0xc800181810606000u, 79445 },
    { 0x20002038001c8010u, 15917 },
    { 0x087ff038000fc001u,  8512 },
    { 0x00080c0c00083007u, 73069 },
    { 0x00000080fc82c040u, 16078 },
    { 0x000000407e416020u, 19168 },
    { 0x00600203f8008020u, 11056 },
    { 0xd003fefe04404080u, 62544 },
    { 0xa00020c018003088u, 80477 },
    { 0x7fbffe700bffe800u, 75049 },
    { 0x107ff00fe4000f90u, 32947 },
    { 0x7f8fffcff1d007f8u, 59172 },
    { 0x0000004100f88080u, 55845 },
    { 0x00000020807c4040u, 61806 },
    { 0x00000041018700c0u, 73601 },
    { 0x0010000080fc4080u, 15546 },
    { 0x1000003c80180030u, 45243 },
    { 0xc10000df80280050u, 20333 },
    { 0xffffffbfeff80fdcu, 33402 },
    { 0x000000101003f812u, 25917 },
    { 0x0800001f40808200u, 32875 },
    { 0x084000101f3fd208u,  4639 },
    { 0x080000000f808081u, 17077 },
    { 0x0004000008003f80u, 62324 },
    { 0x08000001001fe040u, 18159 },
    { 0x72dd000040900a00u, 61436 },
    { 0xfffffeffbfeff81du, 57073 },
    { 0xcd8000200febf209u, 61025 },
    { 0x100000101ec10082u, 81259 },
    { 0x7fbaffffefe0c02fu, 64083 },
    { 0x7f83fffffff07f7fu, 56114 },
    { 0xfff1fffffff7ffc1u, 57058 },
    { 0x0878040000ffe01fu, 58912 },
    { 0x945e388000801012u, 22194 },
    { 0x0840800080200fdau, 70880 },
    { 0x100000c05f582008u, 11140 },
  };

  MagicInit RookMagicInit[SQUARE_NB] = {
    { 0x80280013ff84ffffu, 10890 },
    { 0x5ffbfefdfef67fffu, 50579 },
    { 0xffeffaffeffdffffu, 62020 },
    { 0x003000900300008au, 67322 },
    { 0x0050028010500023u, 80251 },
    { 0x0020012120a00020u, 58503 },
    { 0x0030006000c00030u, 51175 },
    { 0x0058005806b00002u, 83130 },
    { 0x7fbff7fbfbeafffcu, 50430 },
    { 0x0000140081050002u, 21613 },
    { 0x0000180043800048u, 72625 },
    { 0x7fffe800021fffb8u, 80755 },
    { 0xffffcffe7fcfffafu, 69753 },
    { 0x00001800c0180060u, 26973 },
    { 0x4f8018005fd00018u, 84972 },
    { 0x0000180030620018u, 31958 },
    { 0x00300018010c0003u, 69272 },
    { 0x0003000c0085ffffu, 48372 },
    { 0xfffdfff7fbfefff7u, 65477 },
    { 0x7fc1ffdffc001fffu, 43972 },
    { 0xfffeffdffdffdfffu, 57154 },
    { 0x7c108007befff81fu, 53521 },
    { 0x20408007bfe00810u, 30534 },
    { 0x0400800558604100u, 16548 },
    { 0x0040200010080008u, 46407 },
    { 0x0010020008040004u, 11841 },
    { 0xfffdfefff7fbfff7u, 21112 },
    { 0xfebf7dfff8fefff9u, 44214 },
    { 0xc00000ffe001ffe0u, 57925 },
    { 0x4af01f00078007c3u, 29574 },
    { 0xbffbfafffb683f7fu, 17309 },
    { 0x0807f67ffa102040u, 40143 },
    { 0x200008e800300030u, 64659 },
    { 0x0000008780180018u, 70469 },
    { 0x0000010300180018u, 62917 },
    { 0x4000008180180018u, 60997 },
    { 0x008080310005fffau, 18554 },
    { 0x4000188100060006u, 14385 },
    { 0xffffff7fffbfbfffu,     0 },
    { 0x0000802000200040u, 38091 },
    { 0x20000202ec002800u, 25122 },
    { 0xfffff9ff7cfff3ffu, 60083 },
    { 0x000000404b801800u, 72209 },
    { 0x2000002fe03fd000u, 67875 },
    { 0xffffff6ffe7fcffdu, 56290 },
    { 0xbff7efffbfc00fffu, 43807 },
    { 0x000000100800a804u, 73365 },
    { 0x6054000a58005805u, 76398 },
    { 0x0829000101150028u, 20024 },
    { 0x00000085008a0014u,  9513 },
    { 0x8000002b00408028u, 24324 },
    { 0x4000002040790028u, 22996 },
    { 0x7800002010288028u, 23213 },
    { 0x0000001800e08018u, 56002 },
    { 0xa3a80003f3a40048u, 22809 },
    { 0x2003d80000500028u, 44545 },
    { 0xfffff37eefefdfbeu, 36072 },
    { 0x40000280090013c1u,  4750 },
    { 0xbf7ffeffbffaf71fu,  6014 },
    { 0xfffdffff777b7d6eu, 36054 },
    { 0x48300007e8080c02u, 78538 },
    { 0xafe0000fff780402u, 28745 },
    { 0xee73fffbffbb77feu,  8555 },
    { 0x0002000308482882u,  1009 },
  };

  Bitboard relevant_occupancies(Direction directions[], Square s);
  void init_magics(MagicInit init[], Magic magics[], Direction directions[], unsigned shift);

  // popcount16() counts the non-zero bits using SWAR-Popcount algorithm

  unsigned popcount16(unsigned u) {
    u -= (u >> 1) & 0x5555U;
    u = ((u >> 2) & 0x3333U) + (u & 0x3333U);
    u = ((u >> 4) + u) & 0x0F0FU;
    return (u * 0x0101U) >> 8;
  }
}


/// Bitboards::pretty() returns an ASCII representation of a bitboard suitable
/// to be printed to standard output. Useful for debugging.

const std::string Bitboards::pretty(Bitboard b) {

  std::string s = "+---+---+---+---+---+---+---+---+\n";

  for (Rank r = RANK_8; r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= FILE_H; ++f)
          s += b & make_square(f, r) ? "| X " : "|   ";

      s += "|\n+---+---+---+---+---+---+---+---+\n";
  }

  return s;
}


/// Bitboards::init() initializes various bitboard tables. It is called at
/// startup and relies on global objects to be already zero-initialized.

void Bitboards::init() {

  for (unsigned i = 0; i < (1 << 16); ++i)
      PopCnt16[i] = (uint8_t) popcount16(i);

  for (Square s = SQ_A1; s <= SQ_H8; ++s)
      SquareBB[s] = make_bitboard(s);

  for (File f = FILE_A; f <= FILE_H; ++f)
      FileBB[f] = f > FILE_A ? FileBB[f - 1] << 1 : FileABB;

  for (Rank r = RANK_1; r <= RANK_8; ++r)
      RankBB[r] = r > RANK_1 ? RankBB[r - 1] << 8 : Rank1BB;

  for (File f = FILE_A; f <= FILE_H; ++f)
      AdjacentFilesBB[f] = (f > FILE_A ? FileBB[f - 1] : 0) | (f < FILE_H ? FileBB[f + 1] : 0);

  for (Rank r = RANK_1; r < RANK_8; ++r)
      ForwardRanksBB[WHITE][r] = ~(ForwardRanksBB[BLACK][r + 1] = ForwardRanksBB[BLACK][r] | RankBB[r]);

  for (Color c = WHITE; c <= BLACK; ++c)
      for (Square s = SQ_A1; s <= SQ_H8; ++s)
      {
          ForwardFileBB [c][s] = ForwardRanksBB[c][rank_of(s)] & FileBB[file_of(s)];
          PawnAttackSpan[c][s] = ForwardRanksBB[c][rank_of(s)] & AdjacentFilesBB[file_of(s)];
          PassedPawnMask[c][s] = ForwardFileBB [c][s] | PawnAttackSpan[c][s];
      }

  for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1)
      for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2)
          if (s1 != s2)
          {
              SquareDistance[s1][s2] = std::max(distance<File>(s1, s2), distance<Rank>(s1, s2));
              DistanceRingBB[s1][SquareDistance[s1][s2] - 1] |= s2;
          }

  int steps[][5] = { {}, { 7, 9 }, { 6, 10, 15, 17 }, {}, {}, {}, { 1, 7, 8, 9 } };

  for (Color c = WHITE; c <= BLACK; ++c)
      for (PieceType pt : { PAWN, KNIGHT, KING })
          for (Square s = SQ_A1; s <= SQ_H8; ++s)
              for (int i = 0; steps[pt][i]; ++i)
              {
                  Square to = s + Direction(c == WHITE ? steps[pt][i] : -steps[pt][i]);

                  if (is_ok(to) && distance(s, to) < 3)
                  {
                      if (pt == PAWN)
                          PawnAttacks[c][s] |= to;
                      else
                          PseudoAttacks[pt][s] |= to;
                  }
              }

  Direction RookDirections[] = { NORTH,  EAST,  SOUTH,  WEST };
  Direction BishopDirections[] = { NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST };

  if (HasPext)
  {
      unsigned offset = 0;

      for (Square s = SQ_A1; s <= SQ_H8; ++s)
      {
          RookMagicInit[s].offset = offset;
          offset += 1 << popcount(relevant_occupancies(RookDirections, s));
      }

      for (Square s = SQ_A1; s <= SQ_H8; ++s)
      {
          BishopMagicInit[s].offset = offset;
          offset += 1 << popcount(relevant_occupancies(BishopDirections, s));
      }
  }

  init_magics(RookMagicInit, RookMagics, RookDirections, 12);
  init_magics(BishopMagicInit, BishopMagics, BishopDirections, 9);

  for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1)
  {
      PseudoAttacks[QUEEN][s1]  = PseudoAttacks[BISHOP][s1] = attacks_bb<BISHOP>(s1, 0);
      PseudoAttacks[QUEEN][s1] |= PseudoAttacks[  ROOK][s1] = attacks_bb<  ROOK>(s1, 0);

      for (PieceType pt : { BISHOP, ROOK })
          for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2)
          {
              if (!(PseudoAttacks[pt][s1] & s2))
                  continue;

              LineBB[s1][s2] = (attacks_bb(pt, s1, 0) & attacks_bb(pt, s2, 0)) | s1 | s2;
              BetweenBB[s1][s2] = attacks_bb(pt, s1, SquareBB[s2]) & attacks_bb(pt, s2, SquareBB[s1]);
          }
  }
}


namespace {

  Bitboard sliding_attack(Direction directions[], Square sq, Bitboard occupied) {

    Bitboard attack = 0;

    for (int i = 0; i < 4; ++i)
        for (Square s = sq + directions[i];
             is_ok(s) && distance(s, s - directions[i]) == 1;
             s += directions[i])
        {
            attack |= s;

            if (occupied & s)
                break;
        }

    return attack;
  }

  Bitboard relevant_occupancies(Direction directions[], Square s) {

    Bitboard edges = ((Rank1BB | Rank8BB) & ~rank_bb(s)) | ((FileABB | FileHBB) & ~file_bb(s));
    return sliding_attack(directions, s, 0) & ~edges;
  }

  // init_magics() computes all rook and bishop attacks at startup. Magic
  // bitboards are used to look up attacks of sliding pieces. As a reference see
  // chessprogramming.wikispaces.com/Magic+Bitboards. In particular, we use
  // precomputed fixed shift magics.

  void init_magics(MagicInit init[], Magic magics[], Direction directions[], unsigned shift) {

    for (Square s = SQ_A1; s <= SQ_H8; ++s)
    {
        Magic& m = magics[s];

        Bitboard relevant = relevant_occupancies(directions, s);

        m.magic = init[s].magic;
        m.mask = HasPext ? relevant : ~relevant;
        m.attacks = AttackTable + init[s].offset;

        Bitboard b = 0;
        do {
            unsigned idx = HasPext ? pext(b, m.mask) : ((b | m.mask) * m.magic) >> (64 - shift);
            Bitboard attack = sliding_attack(directions, s, b);
            assert(!m.attacks[idx] || m.attacks[idx] == attack);
            m.attacks[idx] = attack;
            b = (b - relevant) & relevant;
        } while (b);
    }
  }

}
