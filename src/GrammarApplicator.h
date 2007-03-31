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
#ifndef __GRAMMARAPPLICATOR_H
#define __GRAMMARAPPLICATOR_H

#include "stdafx.h"
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "Index.h"

namespace CG3 {
	class GrammarApplicator {
	public:
		// ToDo: Implement --sections
		// ToDo: Implement --unsafe
		// ToDo: Make use of Preferred-Targets
		bool always_span;
		bool apply_mappings;
		bool apply_corrections;
		bool trace;
		bool single_run;

		bool dep_reenum;
		bool dep_delimit;
		bool dep_humanize;

		uint32_t num_windows;
		uint32_t soft_limit;
		uint32_t hard_limit;

		GrammarApplicator();
		~GrammarApplicator();

		void enableStatistics();
		void disableStatistics();

		void setGrammar(const Grammar *res);

		int runGrammarOnText(UFILE *input, UFILE *output);

	private:
		static const uint32_t RV_NOTHING = 1;
		static const uint32_t RV_SOMETHING = 2;
		static const uint32_t RV_DELIMITED = 4;

		uint32_t cache_hits, cache_miss, match_single, match_comp, match_sub;
		uint32_t begintag, endtag;
		uint32_t last_mapping_tag;

		const Grammar *grammar;

		stdext::hash_map<uint32_t, uint32_t> variables;
		stdext::hash_map<uint32_t, uint32_t> metas;

		stdext::hash_map<uint32_t, Tag*> single_tags;
		stdext::hash_map<uint32_t, Index*> index_tags_regexp;

		stdext::hash_map<uint32_t, Index*> index_reading_yes;
		stdext::hash_map<uint32_t, Index*> index_reading_no;
	
		uint32_t addTag(const UChar *tag);

		int runGrammarOnWindow(Window *window);
		uint32_t runRulesOnWindow(SingleWindow *current, const std::vector<Rule*> *rules, const uint32_t start, const uint32_t end);

		Cohort *runSingleTest(SingleWindow *sWindow, uint32_t i, const ContextualTest *test, bool *brk, bool *retval);
		Cohort *runContextualTest(SingleWindow *sWindow, const uint32_t position, const ContextualTest *test);

		bool doesTagMatchSet(const uint32_t tag, const uint32_t set);
		bool doesTagMatchReading(const Reading *reading, const uint32_t tag, bool bypass_index = false);
		bool doesSetMatchReading(const Reading *reading, const uint32_t set, bool bypass_index = false);
		bool doesSetMatchCohortNormal(const Cohort *cohort, const uint32_t set);
		bool doesSetMatchCohortCareful(const Cohort *cohort, const uint32_t set);
		Cohort *doesSetMatchDependency(SingleWindow *sWindow, const Cohort *current, const ContextualTest *test);

		void printReading(Reading *reading, UFILE *output);
		void printSingleWindow(SingleWindow *window, UFILE *output);

		bool has_dep;
		uint32_t dep_highest_seen;
		Window *gWindow;
		bool statistics;
		PACC::Timer *timer;

		inline bool __index_matches(const stdext::hash_map<uint32_t, Index*> *me, const uint32_t value, const uint32_t set);
		void reflowReading(Reading *reading);
		void reflowDependencyWindow();
		void attachParentChild(Cohort *parent, Cohort *child);
	};
}

#endif
