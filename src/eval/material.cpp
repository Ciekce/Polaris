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
					S(168, 3), S(71, 57), S(94, 54), S(95, 38), S(106, 26), S(115, 53), S(33, 64), S(127, 10),
					S(20, 18), S(43, 37), S(56, 16), S(49, 14), S(46, 6), S(63, 15), S(48, 29), S(10, 19),
					S(-2, 4), S(-2, 11), S(13, -5), S(27, -13), S(25, -12), S(20, -7), S(-5, 13), S(-3, 6),
					S(-26, -3), S(-20, 1), S(-12, -7), S(-4, -15), S(-5, -17), S(-3, -8), S(-22, 2), S(-25, -3),
					S(-46, -11), S(-29, 4), S(-32, 2), S(-30, 2), S(-26, 0), S(-29, -1), S(-30, 3), S(-44, -10),
					S(-40, 1), S(-5, 14), S(3, 25), S(3, 39), S(1, 31), S(9, 19), S(1, 17), S(-35, -1),
					S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)
				},
				std::array { // knights
					S(-218, -15), S(-75, -36), S(-26, 15), S(20, 3), S(56, -8), S(32, -61), S(-99, -1), S(-216, -44),
					S(-35, -1), S(-19, 3), S(37, -3), S(82, 0), S(72, 6), S(51, 2), S(-11, 2), S(-45, -10),
					S(17, -13), S(67, -1), S(72, 31), S(81, 38), S(91, 32), S(76, 32), S(71, -16), S(12, -7),
					S(13, -2), S(30, 17), S(40, 43), S(53, 47), S(50, 47), S(47, 42), S(36, 16), S(19, -4),
					S(7, 0), S(23, 12), S(23, 41), S(30, 40), S(31, 40), S(25, 40), S(30, 11), S(12, -11),
					S(-33, -6), S(-19, -12), S(-2, 23), S(6, 29), S(12, 25), S(-3, 23), S(-14, -5), S(-23, -11),
					S(-43, -38), S(-23, -4), S(-8, -8), S(0, -1), S(4, 1), S(-8, -2), S(-25, -12), S(-35, -55),
					S(-79, -92), S(-49, -24), S(-30, -14), S(-19, -5), S(-24, -3), S(-32, -20), S(-46, -24), S(-69, -90)
				},
				std::array { // bishops
					S(-79, 30), S(-103, 33), S(-103, 31), S(-86, 23), S(-58, 24), S(-104, 17), S(-111, 36), S(-57, 6),
					S(-19, -6), S(-16, 13), S(-23, 22), S(-25, 20), S(-13, 11), S(-18, 20), S(-20, 14), S(-20, -15),
					S(12, -3), S(60, -9), S(58, 12), S(53, 11), S(53, 15), S(58, 10), S(55, -9), S(13, -9),
					S(6, 0), S(22, 18), S(38, 8), S(53, 31), S(57, 23), S(62, -22), S(22, 14), S(10, -4),
					S(1, -12), S(10, 3), S(24, 25), S(32, 26), S(33, 26), S(26, 17), S(15, 4), S(-7, -10),
					S(2, -18), S(10, -1), S(13, 7), S(15, 7), S(18, -2), S(14, 6), S(4, -3), S(9, -11),
					S(-2, -41), S(7, -26), S(8, -23), S(3, -8), S(8, -14), S(5, -24), S(8, -22), S(-4, -49),
					S(-3, -34), S(-3, -39), S(2, -18), S(-2, -19), S(-8, -16), S(-1, -17), S(-3, -48), S(-9, -33)
				},
				std::array { // rooks
					S(16, 22), S(49, -1), S(41, 2), S(44, 7), S(36, 7), S(38, 11), S(58, -4), S(18, 17),
					S(-2, 22), S(36, 11), S(48, 8), S(52, 11), S(48, 11), S(49, 9), S(34, 12), S(7, 17),
					S(-7, 23), S(44, 5), S(60, -1), S(62, 7), S(64, 1), S(59, 5), S(54, -1), S(5, 18),
					S(-23, 16), S(0, 15), S(10, 13), S(19, 11), S(18, 10), S(7, 17), S(13, 11), S(-9, 6),
					S(-45, 11), S(-36, 17), S(-24, 10), S(-23, 9), S(-23, 6), S(-27, 16), S(-16, 4), S(-30, 6),
					S(-45, 0), S(-31, -4), S(-30, -4), S(-28, -6), S(-28, -5), S(-35, 5), S(-27, -4), S(-36, -4),
					S(-73, -11), S(-41, -20), S(-24, -23), S(-20, -26), S(-22, -24), S(-23, -23), S(-33, -18), S(-65, -15),
					S(-54, -18), S(-31, -22), S(-13, -27), S(0, -38), S(-4, -35), S(-4, -29), S(-16, -25), S(-39, -22)
				},
				std::array { // queens
					S(40, -29), S(34, -8), S(32, 0), S(47, 1), S(50, 7), S(57, -13), S(53, -23), S(29, -19),
					S(-4, 26), S(-14, 51), S(-25, 73), S(-22, 85), S(-25, 91), S(-20, 76), S(-20, 51), S(-19, 33),
					S(0, 29), S(10, 48), S(21, 68), S(11, 85), S(5, 90), S(19, 72), S(11, 55), S(3, 30),
					S(0, 6), S(-11, 48), S(-8, 70), S(0, 85), S(-6, 86), S(-8, 70), S(-2, 45), S(-2, 17),
					S(-4, -20), S(-11, 17), S(-12, 35), S(-15, 54), S(-8, 46), S(-11, 44), S(-5, 13), S(1, -24),
					S(-11, -43), S(-8, -35), S(-5, -18), S(-1, -17), S(-3, -17), S(-4, -15), S(-1, -38), S(-4, -66),
					S(-20, -56), S(-8, -71), S(5, -79), S(6, -70), S(10, -76), S(6, -77), S(-3, -83), S(-19, -68),
					S(-39, -56), S(-16, -79), S(-2, -80), S(-6, -74), S(-2, -77), S(-7, -93), S(-11, -94), S(-29, -91)
				},
				std::array { // kings
					S(107, -143), S(271, -53), S(275, -56), S(225, -49), S(442, -129), S(384, -99), S(253, -56), S(356, -201),
					S(105, -50), S(120, 34), S(116, 18), S(87, 9), S(93, 5), S(79, 25), S(138, 15), S(57, -44),
					S(72, -12), S(182, 1), S(221, -13), S(28, 8), S(12, 13), S(185, -7), S(144, 15), S(46, -6),
					S(18, -27), S(81, -9), S(45, 16), S(78, 8), S(40, 18), S(36, 18), S(86, -5), S(32, -28),
					S(-46, -18), S(-42, 12), S(-48, 25), S(-48, 30), S(-38, 29), S(-14, 16), S(-22, 7), S(-55, -18),
					S(-100, 1), S(-84, 15), S(-115, 26), S(-118, 30), S(-120, 32), S(-101, 23), S(-75, 11), S(-71, -7),
					S(-28, -18), S(-53, 4), S(-87, 10), S(-118, 19), S(-115, 18), S(-83, 12), S(-50, 10), S(-26, -23),
					S(-72, -4), S(-22, -25), S(-44, -16), S(-76, -17), S(-72, -20), S(-59, -13), S(-3, -26), S(-25, -22)
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
