/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */
#ifndef __GRAMMAR_H
#define __GRAMMAR_H

#include <unicode/ustring.h>
#include "Set.h"
#include "Rule.h"

namespace CG3 {

	class Grammar {
	public:
		unsigned int last_modified;
		UChar *name;
		unsigned int lines;
		unsigned int sections;
		stdext::hash_map<UChar*, Set*> sets;
		stdext::hash_map<UChar*, unsigned long> delimiters;
		stdext::hash_map<UChar*, unsigned long> preferred_targets;
		stdext::hash_map<unsigned int, Rule*> rules;

		Grammar() {
			last_modified = 0;
			name = 0;
			lines = 0;
			sections = 0;
		}

		void addDelimiter(UChar *to) {
			UChar *delim = new UChar[u_strlen(to)+1];
			u_strcpy(delim, to);
			delimiters[delim] = hash_sdbm_uchar(delim);
		}

		void addPreferredTarget(UChar *to) {
			UChar *pf = new UChar[u_strlen(to)+1];
			u_strcpy(pf, to);
			preferred_targets[pf] = hash_sdbm_uchar(pf);
		}

		Set *allocateSet() {
			return new Set;
		}
		void destroySet(Set *set) {
			delete set;
		}
	};

}

#endif
