/*
* Copyright (C) 2007, GrammarSoft ApS
* Developed by Tino Didriksen <tino@didriksen.cc>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <tino@didriksen.cc>
*
* This file is part of VISL CG-3
*
* VISL CG-3 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* VISL CG-3 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __RULE_H
#define __RULE_H

#include "stdafx.h"
#include "Strings.h"
#include "ContextualTest.h"
#include "Tag.h"
#include "ContextualTest.h"

namespace CG3 {

	class Grammar;

	class Rule {
	public:
		UChar *name;
		uint32_t wordform;
		uint32_t target;
		uint32_t line;
		uint32_t varname, varvalue;
		uint32_t jumpstart, jumpend;
		int32_t section;
		// ToDo: Add proper "quality" quantifier based on num_fail, num_match, total_time
		double weight, quality;
		KEYWORDS type;
		TagList maplist;
		uint32List sublist;
		
		mutable std::list<ContextualTest*> tests;
		mutable uint32_t num_fail, num_match;
		mutable double total_time;
		mutable ContextualTest *dep_target;
		mutable std::list<ContextualTest*> dep_tests;

		Rule();
		~Rule();
		void setName(const UChar *to);
		
		void resetStatistics();

		ContextualTest *allocateContextualTest();
		void addContextualTest(ContextualTest *to, std::list<ContextualTest*> *thelist);

		static bool cmp_quality(const Rule *a, const Rule *b);

		static inline size_t cmp_hash(const Rule* r) {
			return hash_sdbm_uint32_t(r->line);
		}
		static inline bool cmp_compare(const Rule* a, const Rule* b) {
			return a->line < b->line;
		}
	};

	struct compare_Rule {
		static const size_t bucket_size = 4;
		static const size_t min_buckets = 8;

		inline size_t operator() (const Rule* r) const {
			return Rule::cmp_hash(r);
		}

		inline bool operator() (const Rule* a, const Rule* b) const {
			return Rule::cmp_compare(a, b);
		}
	};

}

#ifdef __GNUC__
namespace __gnu_cxx {
	template<> struct hash< CG3::Rule* > {
		size_t operator()( const CG3::Rule *x ) const {
			return CG3::Rule::cmp_hash(x);
		}
	};
}
#endif

#endif
