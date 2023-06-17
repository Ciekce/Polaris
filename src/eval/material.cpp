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
					S(68, 114), S(62, 103), S(21, 92), S(81, 38), S(33, 54), S(98, 39), S(-83, 112), S(-79, 123),
					S(3, 48), S(11, 39), S(31, 10), S(35, -24), S(45, -31), S(78, -10), S(21, 16), S(6, 27),
					S(-6, 19), S(-1, 4), S(-7, -5), S(13, -32), S(11, -21), S(28, -18), S(4, 2), S(-8, 5),
					S(-14, 4), S(-14, 0), S(-9, -17), S(-2, -25), S(3, -19), S(9, -22), S(1, -9), S(-14, -11),
					S(-22, -3), S(-26, 0), S(-20, -5), S(-17, -13), S(-14, 1), S(-1, -4), S(1, -6), S(-11, -13),
					S(-13, 6), S(-9, 9), S(-19, 13), S(-5, -9), S(-11, 21), S(28, 6), S(29, -4), S(-1, -17),
					S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)
				},
				std::array { // knights
					S(-179, -39), S(-109, -6), S(-91, 16), S(-10, -13), S(89, -23), S(-133, 13), S(-10, -46), S(-115, -81),
					S(-25, -15), S(-41, 3), S(33, -9), S(23, 23), S(42, 11), S(65, -21), S(-13, -4), S(6, -20),
					S(-43, -6), S(4, 2), S(23, 35), S(48, 34), S(75, 16), S(102, 18), S(56, -6), S(62, -33),
					S(7, -2), S(12, 14), S(23, 37), S(44, 44), S(30, 45), S(53, 41), S(31, 21), S(34, 8),
					S(-13, 2), S(0, 4), S(16, 36), S(23, 40), S(34, 35), S(27, 42), S(51, 11), S(6, 13),
					S(-21, -53), S(-2, -6), S(8, 5), S(13, 25), S(23, 16), S(20, 2), S(19, -11), S(-7, 2),
					S(-37, -27), S(-32, -6), S(-4, -27), S(10, 2), S(6, 15), S(8, -22), S(-11, -12), S(1, -29),
					S(-119, -19), S(-16, -27), S(-29, -6), S(-6, 1), S(5, -9), S(-3, -14), S(-12, -12), S(-55, -29)
				},
				std::array { // bishops
					S(-46, -5), S(-27, 5), S(-110, 15), S(-76, 16), S(-82, 14), S(-84, 4), S(-4, -5), S(-4, -3),
					S(-39, 15), S(-17, 4), S(-21, 9), S(-24, 3), S(-16, 11), S(27, 0), S(-9, 4), S(-12, 0),
					S(-8, 4), S(6, 6), S(38, 2), S(16, 6), S(43, -2), S(43, 18), S(48, 5), S(12, 12),
					S(-24, 3), S(16, 9), S(5, 15), S(50, 10), S(29, 18), S(29, 11), S(13, 19), S(-2, 15),
					S(-4, -11), S(2, 4), S(9, 15), S(28, 15), S(35, 5), S(0, 15), S(1, 2), S(15, -17),
					S(-2, -18), S(11, -7), S(9, 1), S(6, 1), S(4, 16), S(16, -6), S(13, -9), S(14, -16),
					S(-1, -17), S(11, -28), S(15, -15), S(-1, -8), S(5, -14), S(5, -23), S(24, -7), S(13, -38),
					S(20, -32), S(17, -17), S(-7, 16), S(-2, -5), S(-6, -7), S(-14, 0), S(-5, -3), S(-3, -28)
				},
				std::array { // rooks
					S(26, 15), S(14, 20), S(-8, 23), S(-5, 24), S(52, 7), S(37, 11), S(48, 5), S(85, 1),
					S(-7, 21), S(-12, 23), S(10, 20), S(22, 20), S(28, 13), S(69, -10), S(28, 3), S(38, 3),
					S(-29, 19), S(4, 10), S(11, 11), S(17, 8), S(30, 0), S(82, -6), S(98, -17), S(49, -8),
					S(-24, 12), S(-15, 7), S(-5, 16), S(10, 5), S(0, 11), S(19, 5), S(12, 0), S(1, 4),
					S(-44, 6), S(-34, 10), S(-36, 11), S(-22, 7), S(-26, 8), S(-10, 3), S(12, -3), S(-10, -9),
					S(-44, -12), S(-33, -5), S(-33, -5), S(-27, -7), S(-27, -5), S(-3, -15), S(17, -14), S(-11, -24),
					S(-56, -14), S(-30, -16), S(-32, -9), S(-25, -14), S(-21, -12), S(-6, -19), S(1, -25), S(-70, -8),
					S(-23, -7), S(-24, -8), S(-17, -10), S(-8, -19), S(-12, -16), S(-5, -8), S(-12, -11), S(-13, -28)
				},
				std::array { // queens
					S(-19, -14), S(-13, 19), S(16, 21), S(25, 17), S(58, 29), S(68, 22), S(75, 7), S(63, 4),
					S(-30, 6), S(-63, 34), S(-34, 37), S(-49, 55), S(-33, 80), S(47, 45), S(15, 44), S(26, 50),
					S(-24, -14), S(-24, -3), S(-9, 4), S(-26, 59), S(-2, 74), S(82, 41), S(80, 35), S(34, 79),
					S(-25, -23), S(-24, 3), S(-30, 23), S(-25, 38), S(-19, 73), S(5, 75), S(8, 82), S(6, 82),
					S(-20, -30), S(-13, -3), S(-13, -1), S(-24, 43), S(-17, 33), S(-1, 33), S(12, 24), S(4, 23),
					S(-23, -38), S(0, -64), S(-3, -17), S(-8, -21), S(-10, -12), S(-3, -5), S(14, -26), S(-2, -9),
					S(-28, -50), S(0, -73), S(6, -81), S(-1, -42), S(3, -56), S(10, -88), S(16, -97), S(-9, -56),
					S(1, -67), S(-15, -53), S(-5, -59), S(-1, 0), S(-7, -55), S(-13, -76), S(12, -86), S(-25, -74)
				},
				std::array { // kings
					S(-13, -73), S(87, -34), S(90, -25), S(59, -16), S(-62, 1), S(-45, 40), S(52, 11), S(3, -16),
					S(93, -18), S(65, 29), S(78, 25), S(45, 23), S(30, 33), S(52, 35), S(-20, 52), S(-72, 22),
					S(53, 20), S(70, 32), S(87, 31), S(5, 33), S(36, 29), S(73, 51), S(98, 41), S(-50, 23),
					S(-30, 11), S(27, 32), S(10, 45), S(3, 51), S(-43, 55), S(5, 46), S(13, 37), S(-83, 21),
					S(-23, -17), S(52, 1), S(-2, 35), S(-67, 59), S(-34, 52), S(-39, 39), S(-8, 20), S(-70, 1),
					S(-37, -16), S(-32, 12), S(-50, 31), S(-51, 43), S(-45, 41), S(-46, 28), S(-19, 11), S(-30, -9),
					S(8, -23), S(1, -9), S(-31, 13), S(-84, 28), S(-58, 25), S(-50, 18), S(3, -7), S(21, -33),
					S(3, -79), S(57, -58), S(19, -25), S(-72, -3), S(-11, -29), S(-57, -7), S(34, -42), S(29, -86)
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
