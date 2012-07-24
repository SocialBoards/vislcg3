/*
* Copyright (C) 2007-2012, GrammarSoft ApS
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

#pragma once
#ifndef c6d28b7452ec699b_GRAMMARAPPLICATOR_H
#define c6d28b7452ec699b_GRAMMARAPPLICATOR_H

#include "stdafx.h"
#include "Tag.h"
#include "CohortIterator.h"
#include "Rule.h"
#include "interval_vector.hpp"
#include "exec-stream.h"

namespace CG3 {
	class Window;
	class Grammar;
	class Reading;
	class SingleWindow;
	class Cohort;
	class ContextualTest;
	class Set;
	class Rule;

	class GrammarApplicator {
	public:
		bool always_span;
		bool apply_mappings;
		bool apply_corrections;
		bool no_before_sections;
		bool no_sections;
		bool no_after_sections;
		bool trace;
		bool trace_name_only;
		bool trace_no_removed;
		bool trace_encl;
		bool allow_magic_readings;
		bool no_pass_origin;
		bool unsafe;
		bool ordered;
		bool show_end_tags;
		bool unicode_tags;
		bool owns_grammar;
		bool input_eof;

		bool dep_has_spanned;
		uint32_t dep_delimit;
		bool dep_humanize;
		bool dep_original;
		bool dep_block_loops;
		bool dep_block_crossing;

		uint32_t num_windows;
		uint32_t soft_limit;
		uint32_t hard_limit;
		uint32Vector sections;
		uint32IntervalVector valid_rules;
		uint32_t verbosity_level;
		uint32_t debug_level;
		uint32_t section_max_count;

		GrammarApplicator(UFILE *ux_err);
		virtual ~GrammarApplicator();

		void enableStatistics();
		void disableStatistics();

		void setGrammar(Grammar *res);
		void index();

		virtual int runGrammarOnText(UFILE *input, UFILE *output);

		bool has_dep;
		uint32_t dep_highest_seen;
		Window *gWindow;
		void reflowDependencyWindow(uint32_t max = 0);

		bool has_relations;
		void reflowRelationWindow(uint32_t max = 0);

		Grammar *grammar;

		// Moved these public to help the library API
		Tag *addTag(const UChar *tag, bool vstr = false);
		void initEmptySingleWindow(SingleWindow *cSWindow);
		uint32_t addTagToReading(Reading& reading, uint32_t tag, bool rehash = true);
		void runGrammarOnWindow();
		Taguint32HashMap single_tags;

	protected:
		void printTrace(UFILE *output, uint32_t hit_by);
		void printReading(const Reading *reading, UFILE *output, size_t sub=1);
		void printCohort(Cohort *cohort, UFILE *output);
		virtual void printSingleWindow(SingleWindow *window, UFILE *output);

		void pipeOutReading(const Reading *reading, std::ostream& output);
		void pipeOutCohort(const Cohort *cohort, std::ostream& output);
		void pipeOutSingleWindow(const SingleWindow& window, std::ostream& output);

		void pipeInReading(Reading *reading, std::istream& input, bool force = false);
		void pipeInCohort(Cohort *cohort, std::istream& input);
		void pipeInSingleWindow(SingleWindow& window, std::istream& input);

		UFILE *ux_stderr;

		uint32_t numLines;
		uint32_t numWindows;
		uint32_t numCohorts;
		uint32_t numReadings;

		bool did_index;
		uint32Set dep_deep_seen;

		uint32_t numsections;
		typedef std::map<int32_t,uint32IntervalVector> RSType;
		RSType runsections;

		typedef std::map<uint32_t,exec_stream_t*> externals_t;
		externals_t externals;

		uint32Vector ci_depths;
		stdext::hash_map<uint32_t,CohortIterator> cohortIterators;
		stdext::hash_map<uint32_t,TopologyLeftIter> topologyLeftIters;
		stdext::hash_map<uint32_t,TopologyRightIter> topologyRightIters;
		stdext::hash_map<uint32_t,DepParentIter> depParentIters;
		stdext::hash_map<uint32_t,DepDescendentIter> DepDescendentIters;

		uint32_t match_single, match_comp, match_sub;
		uint32_t begintag, endtag;
		uint32_t par_left_tag, par_right_tag;
		uint32_t par_left_pos, par_right_pos;
		bool did_final_enclosure;

		std::vector<UnicodeString> regexgrps;
		uint32HashMap variables;
		Cohort *target;
		Cohort *mark;
		Cohort *attach_to;
		const Rule *active_rule;

		uint32HashMap unif_tags;
		uint32_t unif_last_wordform;
		uint32_t unif_last_baseform;
		uint32_t unif_last_textual;
		uint32Set unif_sets;
		bool unif_sets_firstrun;

		uint32HashSet index_regexp_yes;
		uint32HashSet index_regexp_no;
		uint32HashSet index_icase_yes;
		uint32HashSet index_icase_no;
		uint32HashSet index_readingSet_yes;
		uint32HashSet index_readingSet_no;
		uint32HashSet index_ruleCohort_no;
		void resetIndexes();
	
		Tag *addTag(const UString& txt, bool vstr = false);
		Tag *makeBaseFromWord(uint32_t tag);
		Tag *makeBaseFromWord(Tag *tag);

		bool updateRuleToCohorts(Cohort& c, const uint32_t& rsit);
		void indexSingleWindow(SingleWindow& current);
		uint32_t runGrammarOnSingleWindow(SingleWindow& current);
		bool updateValidRules(const uint32IntervalVector& rules, uint32IntervalVector& intersects, const uint32_t& hash, Reading& reading);
		uint32_t runRulesOnSingleWindow(SingleWindow& current, const uint32IntervalVector& rules);

		Cohort *runSingleTest(Cohort *cohort, const ContextualTest *test, bool *brk, bool *retval, Cohort **deep = 0, Cohort *origin = 0);
		Cohort *runSingleTest(SingleWindow *sWindow, size_t i, const ContextualTest *test, bool *brk, bool *retval, Cohort **deep = 0, Cohort *origin = 0);
		Cohort *runContextualTest(SingleWindow *sWindow, size_t position, const ContextualTest *test, Cohort **deep = 0, Cohort *origin = 0);
		Cohort *runDependencyTest(SingleWindow *sWindow, Cohort *current, const ContextualTest *test, Cohort **deep = 0, Cohort *origin = 0, const Cohort *self = 0);
		Cohort *runParenthesisTest(SingleWindow *sWindow, const Cohort *current, const ContextualTest *test, Cohort **deep = 0, Cohort *origin = 0);
		Cohort *runRelationTest(SingleWindow *sWindow, Cohort *current, const ContextualTest *test, Cohort **deep = 0, Cohort *origin = 0);

		uint32_t doesRegexpMatchReading(const Reading& reading, const Tag& tag, bool bypass_index = false);
		uint32_t doesTagMatchReading(const Reading& reading, const Tag& tag, bool unif_mode = false, bool bypass_index = false);
		bool doesSetMatchReading_tags(const Reading& reading, const Set& theset, bool unif_mode = false);
		bool doesSetMatchReading(const Reading& reading, const uint32_t set, bool bypass_index = false, bool unif_mode = false);
		inline void doesSetMatchCohortHelper(std::vector<Reading*>& rv, const ReadingList& readings, const Set *theset, const ContextualTest *test = 0, uint32_t options = 0);
		std::vector<Reading*> doesSetMatchCohort(Cohort& cohort, const uint32_t set, const ContextualTest *test = 0, uint32_t options = 0);
		bool doesSetMatchCohortNormal_helper(ReadingList& readings, const Set *theset, const ContextualTest *test);
		bool doesSetMatchCohortNormal(Cohort& cohort, const uint32_t set, const ContextualTest *test = 0, uint64_t options = 0);
		bool doesSetMatchCohortCareful_helper(ReadingList& readings, const Set *theset, const ContextualTest *test);
		bool doesSetMatchCohortCareful(Cohort& cohort, const uint32_t set, const ContextualTest *test = 0, uint64_t options = 0);

		bool statistics;
		ticks gtimer;

		Cohort *delimitAt(SingleWindow& current, Cohort *cohort);
		void reflowReading(Reading& reading);
		Tag *generateVarstringTag(const Tag *tag);
		void delTagFromReading(Reading& reading, uint32_t tag);
		bool unmapReading(Reading& reading, const uint32_t rule);
		TagList getTagList(const Set& theSet, bool unif_mode = false) const;
		void splitMappings(TagList& mappings, Cohort& cohort, Reading& reading, bool mapped = false);
		void mergeMappings(Cohort& cohort);
		bool isChildOf(const Cohort *child, const Cohort *parent);
		bool wouldParentChildLoop(const Cohort *parent, const Cohort *child);
		bool wouldParentChildCross(const Cohort *parent, const Cohort *child);
		bool attachParentChild(Cohort& parent, Cohort& child, bool allowloop = false, bool allowcrossing = false);

		void reflowTextuals_Reading(Reading& r);
		void reflowTextuals_Cohort(Cohort& c);
		void reflowTextuals_SingleWindow(SingleWindow& sw);
		void reflowTextuals();

		Reading *initEmptyCohort(Cohort& cohort);
	};

	inline Reading *get_sub_reading(Reading *tr, int sub_reading) {
		if (sub_reading == 0) {
			return tr;
		}
		if (sub_reading > 0) {
			for (int i=0 ; i<sub_reading && tr ; ++i) {
				tr = tr->next;
			}
			return tr;
		}
		if (sub_reading < 0) {
			int ntr = 0;
			Reading *ttr = tr;
			while (ttr) {
				ttr = ttr->next;
				--ntr;
			}
			for (int i=ntr ; i<sub_reading && tr ; ++i) {
				tr = tr->next;
			}
			return tr;
		}
		return tr;
	}
}

#endif
