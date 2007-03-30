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

#include "GrammarApplicator.h"
#include "Window.h"
#include "SingleWindow.h"
#include "uextras.h"

using namespace CG3;
using namespace CG3::Strings;

void GrammarApplicator::attachParentChild(Cohort *parent, Cohort *child) {
	parent->dep_self = parent->global_number;

	child->dep_self = child->global_number;
	gWindow->cohort_map.find(child->dep_parent)->second->remChild(child->dep_self);
	child->dep_parent = parent->global_number;

	parent->addChild(child->global_number);

	std::set<uint32_t>::const_iterator tter;
	for (tter = parent->dep_children.begin() ; tter != parent->dep_children.end() ; tter++) {
		std::set<uint32_t>::const_iterator ster;
		for (ster = parent->dep_children.begin() ; ster != parent->dep_children.end() ; ster++) {
			gWindow->cohort_map.find(*tter)->second->addSibling(*ster);
		}
		gWindow->cohort_map.find(*tter)->second->remSibling(*tter);
	}

	parent->dep_done = true;
	child->dep_done = true;
}

void GrammarApplicator::reflowDependencyWindow() {
	bool did_dep = false;
	if (gWindow->dep_window.find(0) == gWindow->dep_window.end()) {
		// This has to be done in 2 steps or it will segfault on Linux for some reason...
		Cohort *tmp = gWindow->dep_window.begin()->second->parent->cohorts.at(0);
		gWindow->dep_window[0] = tmp;
	}

	std::map<uint32_t, Cohort*>::iterator dIter;
	for (dIter = gWindow->dep_window.begin() ; dIter != gWindow->dep_window.end() ; dIter++) {
		Cohort *cohort = dIter->second;
		if (cohort->dep_done) {
			continue;
		}

		if (cohort->dep_self) {
			did_dep = true;
			if (gWindow->dep_map.find(cohort->dep_self) == gWindow->dep_map.end()) {
				gWindow->dep_map[cohort->dep_self] = cohort->global_number;
				cohort->dep_self = cohort->global_number;
			}
		}
	}

	if (did_dep) {
		gWindow->dep_map[0] = 0;
		for (dIter = gWindow->dep_window.begin() ; dIter != gWindow->dep_window.end() ; dIter++) {
			Cohort *cohort = dIter->second;

			if (cohort->dep_self == cohort->global_number) {
				if (!cohort->dep_done && gWindow->dep_map.find(cohort->dep_parent) == gWindow->dep_map.end()) {
					u_fprintf(
						ux_stderr,
						"Warning: Parent %u of dep %u in cohort %u of window %u does not exist - ignoring.\n",
						cohort->dep_parent, cohort->dep_self, cohort->local_number, cohort->parent->number
						);
					// ToDo: If parent is not found, it should be totally ignored, not just set to itself
					cohort->dep_parent = cohort->dep_self;
				}
				else {
					if (!cohort->dep_done) {
						uint32_t dep_real = gWindow->dep_map.find(cohort->dep_parent)->second;
						cohort->dep_parent = dep_real;
					}
					gWindow->cohort_map.find(cohort->dep_parent)->second->addChild(cohort->dep_self);
					cohort->dep_done = true;
				}
			}
		}

		for (dIter = gWindow->dep_window.begin() ; dIter != gWindow->dep_window.end() ; dIter++) {
			Cohort *cohort = dIter->second;

			std::set<uint32_t>::const_iterator tter;
			for (tter = cohort->dep_children.begin() ; tter != cohort->dep_children.end() ; tter++) {
				std::set<uint32_t>::const_iterator ster;
				for (ster = cohort->dep_children.begin() ; ster != cohort->dep_children.end() ; ster++) {
					gWindow->cohort_map.find(*tter)->second->addSibling(*ster);
				}
				gWindow->cohort_map.find(*tter)->second->remSibling(*tter);
			}
		}
	}
}

void GrammarApplicator::reflowReading(Reading *reading) {
	reading->tags.clear();
	reading->tags_mapped.clear();
	reading->tags_plain.clear();
	reading->tags_textual.clear();
	reading->tags_numerical.clear();

	std::list<uint32_t>::const_iterator tter;
	for (tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
		reading->tags[*tter] = *tter;
		Tag *tag = 0;
		if (grammar->single_tags.find(*tter) != grammar->single_tags.end()) {
			tag = grammar->single_tags.find(*tter)->second;
		}
		else {
			tag = single_tags.find(*tter)->second;
		}
		assert(tag != 0);
		if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
			reading->tags_mapped[*tter] = *tter;
		}
		if (tag->type & (T_TEXTUAL|T_WORDFORM|T_BASEFORM)) {
			reading->tags_textual[*tter] = *tter;
		}
		if (tag->type & T_NUMERICAL) {
			reading->tags_numerical[*tter] = *tter;
		}
		if (!reading->baseform && tag->type & T_BASEFORM) {
			reading->baseform = tag->hash;
		}
		if (!reading->wordform && tag->type & T_WORDFORM) {
			reading->wordform = tag->hash;
		}
		if (grammar->has_dep && tag->type & T_DEPENDENCY && !reading->parent->dep_self && !reading->parent->dep_parent) {
			reading->parent->dep_self = tag->dep_self;
			reading->parent->dep_parent = tag->dep_parent;
			has_dep = true;
			if (reading->parent->dep_self <= dep_highest_seen) {
				reflowDependencyWindow();
				gWindow->dep_map.clear();
				gWindow->dep_window.clear();
				dep_highest_seen = 0;
			}
			else {
				dep_highest_seen = reading->parent->dep_self;
			}
		}
		if (!tag->type) {
			reading->tags_plain[*tter] = *tter;
		}
	}
	assert(!reading->tags.empty());
	reading->rehash();
}
