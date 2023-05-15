/*
 * Polaris, a UCI chess engine
 * Copyright (C) 2023 Ciekce
 *
 * Polaris is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Polaris is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Polaris. If not, see <https://www.gnu.org/licenses/>.
 */

#include "material.h"

namespace polaris::eval
{
	namespace pst
	{
#define S(Mg, Eg) TaperedScore{(Mg), (Eg)}
		namespace
		{
			constexpr auto BonusTables = std::array {
				std::array { // pawns
					S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
					S(78, 197), S(70, 180), S(12, 157), S(110, 83), S(14, 127), S(105, 68), S(-120, 185), S(-163, 230),
					S(29, 71), S(13, 70), S(37, 17), S(62, -47), S(93, -65), S(140, -27), S(80, 24), S(39, 40),
					S(-17, 33), S(-3, 10), S(-14, -9), S(19, -55), S(19, -39), S(31, -30), S(16, 1), S(-11, 5),
					S(-31, 4), S(-28, 3), S(-17, -27), S(13, -52), S(4, -41), S(17, -33), S(5, -17), S(-23, -21),
					S(-41, 0), S(-42, 0), S(-30, -16), S(-36, 10), S(-28, 4), S(-2, -10), S(17, -20), S(-18, -27),
					S(-24, 7), S(-5, 4), S(-22, 7), S(5, -4), S(-10, 30), S(59, -1), S(66, -12), S(-1, -22),
					S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)
				},
				std::array { // knights
					S(-238, -85), S(-129, -41), S(-135, 24), S(-27, -25), S(118, -34), S(-156, -8), S(-51, -68), S(-151, -149),
					S(-114, 2), S(-76, 28), S(72, -20), S(38, 21), S(41, 3), S(78, -21), S(22, -11), S(-14, -41),
					S(-90, 3), S(31, 1), S(33, 44), S(76, 31), S(98, 18), S(176, -2), S(78, -11), S(67, -40),
					S(-6, 14), S(15, 22), S(8, 60), S(80, 54), S(49, 54), S(108, 39), S(38, 23), S(63, -6),
					S(-24, 19), S(-3, 12), S(22, 56), S(27, 57), S(45, 59), S(36, 52), S(77, 13), S(4, 14),
					S(-47, -3), S(-22, 27), S(10, -3), S(14, 39), S(45, 35), S(30, 5), S(38, -12), S(-28, -2),
					S(-43, -44), S(-37, -6), S(-17, 1), S(18, 23), S(18, 11), S(23, -12), S(12, -30), S(-1, -44),
					S(-175, -17), S(-27, -29), S(-39, -11), S(-12, 3), S(8, 3), S(-7, -8), S(-22, -2), S(-25, -82)
				},
				std::array { // bishops
					S(-28, -6), S(-47, -9), S(-172, 18), S(-211, 29), S(-122, 9), S(-157, 10), S(-42, -4), S(-60, -12),
					S(-51, 8), S(-37, 16), S(-62, 10), S(-82, 4), S(4, 1), S(26, -7), S(-17, 11), S(-74, -7),
					S(-21, 4), S(25, 1), S(30, 5), S(31, 0), S(73, -10), S(108, -4), S(71, -2), S(47, -3),
					S(-31, 7), S(19, 8), S(10, 30), S(74, 23), S(41, 29), S(61, 8), S(18, -2), S(-6, 21),
					S(-8, -8), S(3, 13), S(14, 17), S(47, 31), S(49, 13), S(6, 10), S(8, 6), S(11, -12),
					S(-3, -8), S(33, -12), S(18, 21), S(19, 3), S(23, 15), S(40, 7), S(22, -1), S(16, -17),
					S(24, -49), S(25, -11), S(35, -36), S(8, 12), S(21, 11), S(40, -21), S(60, -24), S(14, -53),
					S(-10, -34), S(38, -21), S(-2, 16), S(0, 0), S(30, -10), S(-15, 15), S(13, -26), S(5, -33)
				},
				std::array { // rooks
					S(-13, 34), S(28, 17), S(-53, 45), S(24, 18), S(30, 20), S(7, 23), S(59, 4), S(69, 5),
					S(-29, 33), S(-13, 31), S(16, 25), S(49, 13), S(82, -13), S(95, -4), S(22, 17), S(31, 13),
					S(-55, 26), S(-1, 19), S(-13, 22), S(15, 12), S(15, 2), S(96, -20), S(130, -22), S(49, -14),
					S(-65, 23), S(-3, 0), S(-28, 27), S(-8, 10), S(-5, 3), S(35, 1), S(29, -8), S(1, 1),
					S(-73, 15), S(-53, 14), S(-45, 19), S(-31, 11), S(-1, -9), S(-11, -4), S(43, -21), S(-25, -9),
					S(-55, -5), S(-43, 3), S(-38, -6), S(-30, -10), S(-24, -4), S(19, -26), S(39, -30), S(-13, -36),
					S(-62, -11), S(-37, -13), S(-36, -9), S(-21, -6), S(-8, -19), S(16, -25), S(30, -38), S(-77, -20),
					S(-20, -6), S(-17, -10), S(-19, -2), S(5, -24), S(9, -28), S(14, -10), S(-22, -14), S(-8, -32)
				},
				std::array { // queens
					S(-47, -20), S(-27, 3), S(-24, 36), S(11, 19), S(119, -19), S(59, 17), S(99, -30), S(67, 23),
					S(-50, -23), S(-94, 30), S(-41, 50), S(-52, 85), S(-84, 145), S(64, 14), S(31, 12), S(85, -8),
					S(-9, -35), S(-30, -19), S(-7, -8), S(-24, 74), S(81, 28), S(129, 27), S(130, -26), S(97, 26),
					S(-60, 16), S(-29, 9), S(-45, 17), S(-45, 64), S(-19, 80), S(18, 84), S(6, 107), S(10, 77),
					S(-24, -1), S(-50, 23), S(-22, 1), S(-47, 74), S(-18, 29), S(-19, 49), S(12, 52), S(-7, 62),
					S(-35, -24), S(-13, -17), S(-14, -8), S(-2, -52), S(-1, -21), S(9, -21), S(19, -22), S(9, -20),
					S(-44, -54), S(-15, -42), S(3, -16), S(25, -118), S(25, -83), S(37, -88), S(6, -88), S(16, -94),
					S(-10, -62), S(-30, -41), S(-10, -53), S(7, 7), S(-4, -50), S(-43, -33), S(-30, -48), S(-50, -101)
				},
				std::array { // kings
					S(-6, -128), S(94, -60), S(120, -37), S(2, -1), S(-137, 28), S(-124, 74), S(66, 43), S(46, -26),
					S(131, -37), S(-5, 64), S(-61, 62), S(105, 35), S(-11, 66), S(-80, 106), S(-8, 76), S(-14, 36),
					S(-21, 26), S(31, 53), S(128, 39), S(9, 59), S(12, 69), S(114, 89), S(136, 80), S(-15, 33),
					S(-16, -1), S(-47, 65), S(-27, 72), S(-83, 88), S(-125, 95), S(-125, 95), S(-69, 79), S(-141, 35),
					S(-210, 18), S(-25, 19), S(-86, 70), S(-164, 99), S(-204, 107), S(-149, 80), S(-135, 51), S(-181, 21),
					S(11, -28), S(-25, 12), S(-92, 51), S(-136, 76), S(-109, 75), S(-111, 57), S(-46, 28), S(-65, -1),
					S(96, -73), S(46, -19), S(-28, 24), S(-79, 42), S(-75, 43), S(-43, 27), S(35, -10), S(50, -45),
					S(58, -131), S(124, -98), S(85, -56), S(-81, 3), S(21, -32), S(-42, -13), S(91, -72), S(78, -123)
				}
			};

			std::array<std::array<TaperedScore, 64>, 12> createPsts()
			{
				std::array<std::array<TaperedScore, 64>, 12> psts{};

				for (const auto piece : {
					BasePiece::Pawn, BasePiece::Knight,
					BasePiece::Bishop, BasePiece::Rook,
					BasePiece::Queen, BasePiece::King
				})
				{
					const auto value = pieceValue(piece);
					const auto &bonusTable = BonusTables[static_cast<i32>(piece)];

					auto &blackPst = psts[static_cast<i32>(piece) * 2    ];
					auto &whitePst = psts[static_cast<i32>(piece) * 2 + 1];

					for (i32 square = 0; square < 64; ++square)
					{
						blackPst[square] = -value - bonusTable[square       ];
						whitePst[square] =  value + bonusTable[square ^ 0x38];
					}
				}

				return psts;
			}
		}
#undef S

		const std::array<std::array<TaperedScore, 64>, 12> PieceSquareTables = createPsts();
	}
}
