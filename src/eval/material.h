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

#pragma once

#include "../types.h"

#include <array>

#include "../core.h"

namespace polaris::eval
{
#define S(Mg, Eg) TaperedScore{(Mg), (Eg)}

	namespace values
	{
		// keep below 2000
		constexpr auto Pawn = S(84, 100);
		constexpr auto Knight = S(361, 312);
		constexpr auto Bishop = S(379, 342);
		constexpr auto Rook = S(483, 610);
		constexpr auto Queen = S(1045, 1175);

		constexpr auto King = S(0, 0);

		constexpr auto BaseValues = std::array {
			Pawn,
			Knight,
			Bishop,
			Rook,
			Queen,
			King,
			TaperedScore{}
		};

		constexpr auto Values = std::array {
			Pawn,
			Pawn,
			Knight,
			Knight,
			Bishop,
			Bishop,
			Rook,
			Rook,
			Queen,
			Queen,
			King,
			King,
			TaperedScore{}
		};
	}
#undef S

	constexpr auto pieceValue(BasePiece piece)
	{
		return values::BaseValues[static_cast<i32>(piece)];
	}

	constexpr auto pieceValue(Piece piece)
	{
		return values::Values[static_cast<i32>(piece)];
	}
}
