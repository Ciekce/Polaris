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

#include "nnue.h"

#ifdef _MSC_VER
#define PS_MSVC
#pragma push_macro("_MSC_VER")
#undef _MSC_VER
#endif

#define INCBIN_PREFIX g_
#include "../3rdparty/incbin.h"

#ifdef PS_MSVC
#pragma pop_macro("_MSC_VER")
#undef PS_MSVC
#endif

//TODO (maybe) this path only works for cmake
INCBIN(nnue, "../src/eval/gemstone.nnue");

namespace polaris::eval::nnue
{
	// technically this might not be aligned as expected
	// but as incbin aligns for the worst case possible it's fine in practice
	// in theory though we're violating the alignment requirements of NnueParams /shrug
	const NnueParams &g_nnue = *reinterpret_cast<const NnueParams *>(g_nnueData);
}
