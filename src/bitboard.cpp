/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2026 The Stockfish developers (see AUTHORS file)

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

#include "bitboard.h"

#include <algorithm>
#include <bitset>
#include <initializer_list>

#include "misc.h"

namespace Stockfish {

uint8_t PopCnt16[1 << 16];
uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

Bitboard LineBB[SQUARE_NB][SQUARE_NB];
Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
Bitboard RayPassBB[SQUARE_NB][SQUARE_NB];

alignas(64) Magic Magics[SQUARE_NB][2];

// Returns an ASCII representation of a bitboard suitable
// to be printed to standard output. Useful for debugging.
std::string Bitboards::pretty(Bitboard b) {

    std::string s = "+---+---+---+---+---+---+---+---+\n";

    for (Rank r = RANK_8;; --r)
    {
        for (File f = FILE_A; f <= FILE_H; ++f)
            s += b & make_square(f, r) ? "| X " : "|   ";

        s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";

        if (r == RANK_1)
            break;
    }
    s += "  a   b   c   d   e   f   g   h\n";

    return s;
}

namespace {

struct KnownMagic {
    uint64_t magic;
    unsigned offset;
};

// Fixed shift white magics found by Volker Annuss.
// From: http://www.talkchess.com/forum/viewtopic.php?p=727500&t=64790
// clang-format off
constexpr KnownMagic KnownRookMagics[SQUARE_NB] = {
    {0x00280077ffebfffeULL, 26304},
    {0x2004010201097fffULL, 35520},
    {0x0010020010053fffULL, 38592},
    {0x0040040008004002ULL,  8026},
    {0x7fd00441ffffd003ULL, 22196},
    {0x4020008887dffffeULL, 80870},
    {0x004000888847ffffULL, 76747},
    {0x006800fbff75fffdULL, 30400},
    {0x000028010113ffffULL, 11115},
    {0x0020040201fcffffULL, 18205},
    {0x007fe80042ffffe8ULL, 53577},
    {0x00001800217fffe8ULL, 62724},
    {0x00001800073fffe8ULL, 34282},
    {0x00001800e05fffe8ULL, 29196},
    {0x00001800602fffe8ULL, 23806},
    {0x000030002fffffa0ULL, 49481},
    {0x00300018010bffffULL,  2410},
    {0x0003000c0085fffbULL, 36498},
    {0x0004000802010008ULL, 24478},
    {0x0004002020020004ULL, 10074},
    {0x0001002002002001ULL, 79315},
    {0x0001001000801040ULL, 51779},
    {0x0000004040008001ULL, 13586},
    {0x0000006800cdfff4ULL, 19323},
    {0x0040200010080010ULL, 70612},
    {0x0000080010040010ULL, 83652},
    {0x0004010008020008ULL, 63110},
    {0x0000040020200200ULL, 34496},
    {0x0002008010100100ULL, 84966},
    {0x0000008020010020ULL, 54341},
    {0x0000008020200040ULL, 60421},
    {0x0000820020004020ULL, 86402},
    {0x00fffd1800300030ULL, 50245},
    {0x007fff7fbfd40020ULL, 76622},
    {0x003fffbd00180018ULL, 84676},
    {0x001fffde80180018ULL, 78757},
    {0x000fffe0bfe80018ULL, 37346},
    {0x0001000080202001ULL,   370},
    {0x0003fffbff980180ULL, 42182},
    {0x0001fffdff9000e0ULL, 45385},
    {0x00fffefeebffd800ULL, 61659},
    {0x007ffff7ffc01400ULL, 12790},
    {0x003fffbfe4ffe800ULL, 16762},
    {0x001ffff01fc03000ULL,     0},
    {0x000fffe7f8bfe800ULL, 38380},
    {0x0007ffdfdf3ff808ULL, 11098},
    {0x0003fff85fffa804ULL, 21803},
    {0x0001fffd75ffa802ULL, 39189},
    {0x00ffffd7ffebffd8ULL, 58628},
    {0x007fff75ff7fbfd8ULL, 44116},
    {0x003fff863fbf7fd8ULL, 78357},
    {0x001fffbfdfd7ffd8ULL, 44481},
    {0x000ffff810280028ULL, 64134},
    {0x0007ffd7f7feffd8ULL, 41759},
    {0x0003fffc0c480048ULL,  1394},
    {0x0001ffffafd7ffd8ULL, 40910},
    {0x00ffffe4ffdfa3baULL, 66516},
    {0x007fffef7ff3d3daULL,  3897},
    {0x003fffbfdfeff7faULL,  3930},
    {0x001fffeff7fbfc22ULL, 72934},
    {0x0000020408001001ULL, 72662},
    {0x0007fffeffff77fdULL, 56325},
    {0x0003ffffbf7dfeecULL, 66501},
    {0x0001ffff9dffa333ULL, 14826},
};

constexpr KnownMagic KnownBishopMagics[SQUARE_NB] = {
    {0x007fbfbfbfbfbfffULL,  5378},
    {0x0000a060401007fcULL,  4093},
    {0x0001004008020000ULL,  4314},
    {0x0000806004000000ULL,  6587},
    {0x0000100400000000ULL,  6491},
    {0x000021c100b20000ULL,  6330},
    {0x0000040041008000ULL,  5609},
    {0x00000fb0203fff80ULL, 22236},
    {0x0000040100401004ULL,  6106},
    {0x0000020080200802ULL,  5625},
    {0x0000004010202000ULL, 16785},
    {0x0000008060040000ULL, 16817},
    {0x0000004402000000ULL,  6842},
    {0x0000000801008000ULL,  7003},
    {0x000007efe0bfff80ULL,  4197},
    {0x0000000820820020ULL,  7356},
    {0x0000400080808080ULL,  4602},
    {0x00021f0100400808ULL,  4538},
    {0x00018000c06f3fffULL, 29531},
    {0x0000258200801000ULL, 45393},
    {0x0000240080840000ULL, 12420},
    {0x000018000c03fff8ULL, 15763},
    {0x00000a5840208020ULL,  5050},
    {0x0000020008208020ULL,  4346},
    {0x0000804000810100ULL,  6074},
    {0x0001011900802008ULL,  7866},
    {0x0000804000810100ULL, 32139},
    {0x000100403c0403ffULL, 57673},
    {0x00078402a8802000ULL, 55365},
    {0x0000101000804400ULL, 15818},
    {0x0000080800104100ULL,  5562},
    {0x00004004c0082008ULL,  6390},
    {0x0001010120008020ULL,  7930},
    {0x000080809a004010ULL, 13329},
    {0x0007fefe08810010ULL,  7170},
    {0x0003ff0f833fc080ULL, 27267},
    {0x007fe08019003042ULL, 53787},
    {0x003fffefea003000ULL,  5097},
    {0x0000101010002080ULL,  6643},
    {0x0000802005080804ULL,  6138},
    {0x0000808080a80040ULL,  7418},
    {0x0000104100200040ULL,  7898},
    {0x0003ffdf7f833fc0ULL, 42012},
    {0x0000008840450020ULL, 57350},
    {0x00007ffc80180030ULL, 22813},
    {0x007fffdd80140028ULL, 56693},
    {0x00020080200a0004ULL,  5818},
    {0x0000101010100020ULL,  7098},
    {0x0007ffdfc1805000ULL,  4451},
    {0x0003ffefe0c02200ULL,  4709},
    {0x0000000820806000ULL,  4794},
    {0x0000000008403000ULL, 13364},
    {0x0000000100202000ULL,  4570},
    {0x0000004040802000ULL,  4282},
    {0x0004010040100400ULL, 14964},
    {0x00006020601803f4ULL,  4026},
    {0x0003ffdfdfc28048ULL,  4826},
    {0x0000000820820020ULL,  7354},
    {0x0000000008208060ULL,  4848},
    {0x0000000000808020ULL, 15946},
    {0x0000000001002020ULL, 14932},
    {0x0000000401002008ULL, 16588},
    {0x0000004040404040ULL,  6905},
    {0x007fff9fdf7ff813ULL, 16076},
};
// clang-format on

[[maybe_unused]] constexpr Bitboard constexpr_pext(Bitboard b, Bitboard m) {
    Bitboard result = 0, bit = 0;
    while (m)
    {
        Bitboard last = m & -m;
        result |= bool(b & last) << bit++;
        m ^= last;
    }
    return result;
}

// Computes all rook and bishop attacks at startup or optionally, compile time. Magic
// bitboards are used to look up attacks of sliding pieces. As a reference see
// https://www.chessprogramming.org/Magic_Bitboards.
constexpr void
init_magics(PieceType pt, Magic::AttackData* table, Magic magics[][2], bool tableAlreadyInit) {
#if !defined(USE_COMPTIME_ATTACKS)
    tableAlreadyInit = false;
#endif

    for (Square s = SQ_A1; s <= SQ_H8; ++s)
    {
        // Board edges are not considered in the relevant occupancies
        Bitboard edges = ((Rank1BB | Rank8BB) & ~rank_bb(s)) | ((FileABB | FileHBB) & ~file_bb(s));

        // Given a square 's', the mask is the bitboard of sliding attacks from
        // 's' computed on an empty board. The index must be big enough to contain
        // all the attacks for each possible subset of the mask and so is 2 power
        // the number of 1s of the mask. Hence we deduce the size of the shift to
        // apply to the 64 or 32 bits word to get the index.
        Magic&   m       = magics[s][pt - BISHOP];
        Bitboard attacks = Bitboards::sliding_attack(pt, s, 0);
        m.mask           = attacks & ~edges;
#ifdef USE_PEXT
        m.pseudoAttacks = attacks;
        m.attacks       = table;

        // Use Carry-Rippler trick to enumerate all subsets of masks[s] and
        // store the corresponding sliding attack bitboard in reference[].
        Bitboard b           = 0;
        Bitboard prevSliding = -1;
        do
        {
            if (!tableAlreadyInit)
            {
                Bitboard sliding = Bitboards::sliding_attack(pt, s, b);
                *table = sliding != prevSliding ? constexpr_pext(sliding, attacks) : *(table - 1);
                prevSliding = sliding;
            }
            table++;
            b = (b - m.mask) & m.mask;
        } while (b);
#else
        const KnownMagic& knownMagic = (pt == ROOK) ? KnownRookMagics[s] : KnownBishopMagics[s];
        m.magic                      = knownMagic.magic;
        m.attacks                    = &table[knownMagic.offset];
        if (!tableAlreadyInit)
        {
            int      shift = 64 - (pt == ROOK ? 12 : 9);
            Bitboard b     = 0;
            do
            {
                unsigned index   = ((b & m.mask) * m.magic) >> shift;
                m.attacks[index] = Bitboards::sliding_attack(pt, s, b);
                b                = (b - m.mask) & m.mask;
            } while (b);
        }
#endif
    }
}

#if defined(USE_COMPTIME_ATTACKS)
    #ifdef USE_PEXT
constexpr auto RookTable = []() {
    std::array<uint16_t, 0x19000> result{};
    Magic                         magics[64][2] = {};
    init_magics(ROOK, result.data(), magics, false);
    return result;
}();
constexpr auto BishopTable = []() {
    std::array<uint16_t, 0x1480> result{};
    Magic                        magics[64][2] = {};
    init_magics(BISHOP, result.data(), magics, false);
    return result;
}();
    #else
constexpr auto AttackTable = []() {
    std::array<Bitboard, 88772> result{};
    Magic                       magics[64][2] = {};
    init_magics(ROOK, result.data(), magics, false);
    init_magics(BISHOP, result.data(), magics, false);
    return result;
}();
    #endif
#else
    #ifdef USE_PEXT
std::array<MagicMask, 0x19000> RookTable{};
std::array<MagicMask, 0x1480>  BishopTable{};
    #else
std::array<Bitboard, 88772> AttackTable{};
    #endif
#endif  // !USE_COMPTIME_ATTACKS
}


// Initializes various bitboard tables. It is called at
// startup and relies on global objects to be already zero-initialized.
void Bitboards::init() {

    for (unsigned i = 0; i < (1 << 16); ++i)
        PopCnt16[i] = uint8_t(std::bitset<16>(i).count());

    for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1)
        for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2)
            SquareDistance[s1][s2] = std::max(distance<File>(s1, s2), distance<Rank>(s1, s2));

#ifdef USE_PEXT
    init_magics(ROOK, const_cast<Magic::AttackData*>(RookTable.data()), Magics, true);
    init_magics(BISHOP, const_cast<Magic::AttackData*>(BishopTable.data()), Magics, true);
#else
    init_magics(ROOK, const_cast<Magic::AttackData*>(AttackTable.data()), Magics, true);
    init_magics(BISHOP, const_cast<Magic::AttackData*>(AttackTable.data()), Magics, true);
#endif

    for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1)
    {
        for (PieceType pt : {BISHOP, ROOK})
            for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2)
            {
                if (PseudoAttacks[pt][s1] & s2)
                {
                    LineBB[s1][s2] = (attacks_bb(pt, s1, 0) & attacks_bb(pt, s2, 0)) | s1 | s2;
                    BetweenBB[s1][s2] =
                      (attacks_bb(pt, s1, square_bb(s2)) & attacks_bb(pt, s2, square_bb(s1)));
                    RayPassBB[s1][s2] =
                      attacks_bb(pt, s1, 0) & (attacks_bb(pt, s2, square_bb(s1)) | s2);
                }
                BetweenBB[s1][s2] |= s2;
            }
    }
}

}  // namespace Stockfish
