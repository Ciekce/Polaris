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

#include "uci.h"
#include "bench.h"
#include "eval/material.h"

using namespace polaris;

int main(int argc, const char *argv[])
{
	/*
	std::cout << "constexpr auto MvvLva = std::array {";

	for (i32 moving = 0; moving < 6; ++moving)
	{
		if (moving > 0)
			std::cout << ",";
		std::cout << "\n\tstd::array {\n\t\t";

		const auto srcValue = eval::pieceValue(static_cast<BasePiece>(moving)).midgame();

		for (i32 captured = 0; captured <= 6; ++captured)
		{
			if (captured > 0)
				std::cout << ", ";

			const auto dstValue = eval::pieceValue(static_cast<BasePiece>(captured)).midgame();
			std::cout << (captured == static_cast<i32>(BasePiece::King)
				? 0 : ((dstValue - srcValue) * 2000 + dstValue));
		}

		std::cout << "\n\t}";
	}

	std::cout << "\n};";

	return 0;
	 */

	if (argc > 1 && std::string{argv[1]} == "bench")
	{
		search::Searcher searcher{16};
		bench::run(searcher);

		return 0;
	}

	return uci::run();
}
