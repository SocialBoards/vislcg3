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

inline void GrammarApplicator::reflowSingleWindow(SingleWindow *swindow) {
	swindow->tags.clear();
	swindow->tags_mapped.clear();
	swindow->tags_plain.clear();
	swindow->tags_textual.clear();

	for (uint32_t c=0 ; c < swindow->cohorts.size() ; c++) {
		Cohort *cohort = swindow->cohorts[c];

		std::list<Reading*>::iterator rter;
		for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
			Reading *reading = *rter;

			std::list<uint32_t>::const_iterator tter;
			for (tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
				swindow->tags[*tter] = *tter;
				Tag *tag = 0;
				if (grammar->single_tags.find(*tter) != grammar->single_tags.end()) {
					tag = grammar->single_tags.find(*tter)->second;
				}
				else {
					tag = single_tags.find(*tter)->second;
				}
				assert(tag != 0);
				if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
					swindow->tags_mapped[*tter] = *tter;
				}
				if (tag->type & T_TEXTUAL) {
					swindow->tags_textual[*tter] = *tter;
				}
				if (!tag->features && !tag->type) {
					swindow->tags_plain[*tter] = *tter;
				}
			}
		}
	}
	swindow->rehash();
}

inline void GrammarApplicator::reflowReading(Reading *reading) {
	reading->tags.clear();
	reading->tags_mapped.clear();
	reading->tags_plain.clear();
	reading->tags_textual.clear();

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
		if (!tag->features && !tag->type) {
			reading->tags_plain[*tter] = *tter;
		}
	}
	assert(!reading->tags.empty());
	reading->rehash();
}

int GrammarApplicator::runGrammarOnText(UFILE *input, UFILE *output) {
	if (!input) {
		u_fprintf(ux_stderr, "Error: Input is null - nothing to parse!\n");
		return -1;
	}
	u_frewind(input);
	if (u_feof(input)) {
		u_fprintf(ux_stderr, "Error: Input is empty - nothing to parse!\n");
		return -1;
	}
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		return -1;
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue! Hint: call setGrammar() first.\n");
		return -1;
	}

	free_strings();
	free_keywords();
	int error = init_keywords();
	if (error) {
		u_fprintf(ux_stderr, "Error: init_keywords returned %u!\n", error);
		return error;
	}
	error = init_strings();
	if (error) {
		u_fprintf(ux_stderr, "Error: init_strings returned %u!\n", error);
		return error;
	}

#define BUFFER_SIZE (131072)
	UChar _line[BUFFER_SIZE];
	UChar *line = _line;
	UChar _cleaned[BUFFER_SIZE];
	UChar *cleaned = _cleaned;

	begintag = addTag(stringbits[S_BEGINTAG]);
	endtag = addTag(stringbits[S_ENDTAG]);

	uint32_t lines = 0;
	Window *cWindow = new Window();
	SingleWindow *cSWindow = 0;
	Cohort *cCohort = 0;
	Reading *cReading = 0;

	SingleWindow *lSWindow = 0;
	Cohort *lCohort = 0;
	Reading *lReading = 0;

	cWindow->num_windows = num_windows;

	while (!u_feof(input)) {
		u_fgets(line, BUFFER_SIZE-1, input);
		u_strcpy(cleaned, line);
		ux_packWhitespace(cleaned);

		if (cleaned[0] == '"' && cleaned[1] == '<') {
			ux_trim(cleaned);
			if (cCohort && doesTagMatchSet(cCohort->wordform, grammar->delimiters->hash)) {
				if (cCohort->readings.empty()) {
					cReading = new Reading();
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					cReading->tags[cCohort->wordform] = cCohort->wordform;
					cReading->tags_list.push_back(cCohort->wordform);
					cReading->noprint = true;
					reflowReading(cReading);
					cCohort->readings.push_back(cReading);
					lReading = cReading;
				}
				std::list<Reading*>::iterator iter;
				for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
					(*iter)->tags_list.push_back(endtag);
					(*iter)->tags[endtag] = endtag;
					(*iter)->rehash();
				}

				cSWindow->cohorts.push_back(cCohort);
				cWindow->next.push_back(cSWindow);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
			}
			if (!cSWindow) {
				cReading = new Reading();
				cReading->baseform = begintag;
				cReading->wordform = begintag;
				cReading->tags[begintag] = begintag;
				cReading->tags_list.push_back(begintag);
				cReading->rehash();

				cCohort = new Cohort();
				cCohort->wordform = begintag;
				cCohort->readings.push_back(cReading);

				cSWindow = new SingleWindow();
				cSWindow->cohorts.push_back(cCohort);

				lSWindow = cSWindow;
				lReading = cReading;
				lCohort = cCohort;
				cCohort = 0;
			}
			if (cCohort && cSWindow) {
				// ToDo: Add max window length
				cSWindow->cohorts.push_back(cCohort);
				lCohort = cCohort;
				if (cCohort->readings.empty()) {
					cReading = new Reading();
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					cReading->tags[cCohort->wordform] = cCohort->wordform;
					cReading->tags_list.push_back(cCohort->wordform);
					cReading->noprint = true;
					reflowReading(cReading);
					cCohort->readings.push_back(cReading);
					lReading = cReading;
				}
			}
			if (cWindow->next.size() > num_windows) {
				cWindow->shuffleWindowsDown();
				runGrammarOnWindow(cWindow);
				printSingleWindow(cWindow->current, output);
			}
			cCohort = new Cohort();
			cCohort->wordform = addTag(cleaned);
			lCohort = cCohort;
			lReading = 0;
		}
		else if (cleaned[0] == ' ' && cleaned[1] == '"' && cCohort) {
			cReading = new Reading();
			cReading->wordform = cCohort->wordform;
			cReading->tags[cReading->wordform] = cReading->wordform;

			ux_trim(cleaned);
			UChar *space = cleaned;
			UChar *base = space;

			while (space && (space = u_strchr(space, ' ')) != 0) {
				space[0] = 0;
				space++;
				if (u_strlen(base)) {
					uint32_t tag = addTag(base);
					if (!cReading->baseform && single_tags[tag]->type & T_BASEFORM) {
						cReading->baseform = tag;
					}
					cReading->tags_list.push_back(tag);
				}
				base = space;
			}
			if (u_strlen(base)) {
				uint32_t tag = addTag(base);
				if (!cReading->baseform && single_tags[tag]->type & T_BASEFORM) {
					cReading->baseform = tag;
				}
				cReading->tags_list.push_back(tag);
			}
			reflowReading(cReading);
			if (!cReading->tags_mapped.empty()) {
				cReading->mapped = true;
			}
			cCohort->readings.push_back(cReading);
			lReading = cReading;
		}
		else {
			if (cleaned[0] == ' ' && cleaned[1] == '"') {
				u_fprintf(ux_stderr, "Warning: Line %u looked like a reading but there was no containing cohort - treated as plain text.\n", lines);
			}
			ux_trim(cleaned);
			if (u_strlen(cleaned) > 0) {
				if (lReading) {
					lReading->text = ux_append(lReading->text, line);
				}
				else if (lCohort) {
					lCohort->text = ux_append(lCohort->text, line);
				}
				else if (lSWindow) {
					lSWindow->text = ux_append(lSWindow->text, line);
				}
				else {
					u_fprintf(output, "%S", line);
				}
			}
		}
		lines++;
	}

	if (cCohort && cSWindow) {
		cSWindow->cohorts.push_back(cCohort);
		lCohort = cCohort;
		if (cCohort->readings.empty()) {
			cReading = new Reading();
			cReading->wordform = cCohort->wordform;
			cReading->baseform = cCohort->wordform;
			cReading->tags[cCohort->wordform] = cCohort->wordform;
			cReading->noprint = true;
			cReading->rehash();
			cCohort->readings.push_back(cReading);
			lReading = cReading;
		}
		cWindow->next.push_back(cSWindow);
	}
	while (!cWindow->next.empty()) {
		cWindow->shuffleWindowsDown();
		runGrammarOnWindow(cWindow);
		printSingleWindow(cWindow->current, output);
	}
	u_fflush(output);
	std::cerr << "Cache " << (cache_hits+cache_miss) << " : " << cache_hits << " / " << cache_miss << std::endl;
	std::cerr << "Match " << (match_sub+match_comp+match_single) << " : " << match_sub << " / " << match_comp << " / " << match_single << std::endl;
	return 0;
}

uint32_t GrammarApplicator::runRulesOnWindow(Window *window, const std::vector<Rule*> *rules, const uint32_t start, const uint32_t end) {
	SingleWindow *current = window->current;

	uint32_t retval = RV_NOTHING;
	bool section_did_good = false;
	bool delimited = false;

	if (!rules->empty()) {
		for (uint32_t j=start;j<end;) {
			const Rule *rule = rules->at(j);
			KEYWORDS type = rule->type;

			if (!apply_mappings && (rule->type == K_MAP || rule->type == K_ADD || rule->type == K_REPLACE)) {
				continue;
			}
			if (!apply_corrections && (rule->type == K_SUBSTITUTE || rule->type == K_APPEND)) {
				continue;
			}

			for (uint32_t c=0 ; c < current->cohorts.size() ; c++) {
				if (c == 0) {
					continue;
				}
				Cohort *cohort = current->cohorts[c];
				if (cohort->readings.empty()) {
					continue;
				}

				const Set *set = grammar->sets_by_contents.find(rule->target)->second;
				if ((type == K_SELECT || type == K_REMOVE || type == K_IFF) && cohort->readings.size() == 1) {
					if (!set->has_mappings || cohort->readings.front()->tags_mapped.size() <= 1) {
						continue;
					}
				}
				if (type == K_DELIMIT && c == current->cohorts.size()-1) {
					continue;
				}
				if (rule->wordform && rule->wordform != cohort->wordform) {
					rule->num_fail++;
					continue;
				}

				size_t num_active = 0;
				size_t num_iff = 0;
				bool all_active = false;
				std::list<Reading*>::iterator rter;

				for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
					Reading *reading = *rter;
					reading->matched_target = false;
					reading->matched_tests = false;
					reading->current_mapping_tag = 0;
					if (!reading->hash) {
						reading->rehash();
					}
					if (reading->mapped && (rule->type == K_MAP || rule->type == K_ADD || rule->type == K_REPLACE)) {
						continue;
					}
					last_mapping_tag = 0;
					if (rule->target && doesSetMatchReading(reading, rule->target, set->has_mappings)) {
						reading->current_mapping_tag = last_mapping_tag;
						reading->matched_target = true;
						bool good = true;
						if (!rule->tests.empty()) {
							bool test_good = false;
							std::list<ContextualTest*>::iterator iter;
							for (iter = rule->tests.begin() ; iter != rule->tests.end() ; iter++) {
								ContextualTest *test = *iter;
								test_good = runContextualTest(window, current, c, test);
								if (!test_good) {
									good = test_good;
									if (!statistics) {
										/*
										if (test != rule->tests.front()) {
										rule->tests.remove(test);
										rule->tests.push_front(test);
										}
										//*/
										break;
									}
								}
							}
						}
						if (good) {
							reading->matched_tests = true;
							num_active++;
						}
						num_iff++;
					}
					else {
						rule->num_fail++;
					}
				}

				if (num_active == 0 && (num_iff == 0 || rule->type != K_IFF)) {
					continue;
				}
				if (num_active == cohort->readings.size()) {
					all_active = true;
				}
				if (all_active && rule->type == K_SELECT) {
					continue;
				}
				if (all_active && rule->type == K_REMOVE) {
					continue;
				}

				std::list<Reading*> removed;
				std::list<Reading*> selected;
				removed.clear();
				selected.clear();
				
				for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
					Reading *reading = *rter;
					bool good = reading->matched_tests;

					if (rule->type == K_IFF) {
						if (!good && reading->matched_target) {
							type = K_REMOVE;
							good = true;
						}
					}

					if (type == K_REMOVE) {
						if (good && reading->current_mapping_tag && reading->tags_mapped.size() > 1) {
							reading->tags_list.remove(reading->current_mapping_tag);
							reflowReading(reading);
						}
						else {
							if (good) {
								removed.push_back(reading);
								reading->deleted = true;
								reading->hit_by.push_back(j);
							}
							else {
								selected.push_back(reading);
							}
						}
						if (good) {
							section_did_good = true;
						}
					}
					else if (type == K_SELECT) {
						if (good && reading->current_mapping_tag && reading->tags_mapped.size() > 1) {
							std::map<uint32_t, uint32_t>::iterator iter_maps;
							for (iter_maps = reading->tags_mapped.begin() ; iter_maps != reading->tags_mapped.end() ; iter_maps++) {
								reading->tags_list.remove(iter_maps->second);
							}
							reading->tags_list.push_back(reading->current_mapping_tag);
							reflowReading(reading);
						}
						if (good) {
							selected.push_back(reading);
							reading->selected = true;
							reading->hit_by.push_back(j);
						}
						else {
							removed.push_back(reading);
							reading->deleted = true;
							reading->hit_by.push_back(j);
						}
						if (good) {
							section_did_good = true;
						}
					}
					if (good) {
						if (type == K_REMVARIABLE) {
							variables[rule->varname] = 0;
						}
						else if (type == K_SETVARIABLE) {
							variables[rule->varname] = rule->varvalue;
						}
						else if (type == K_DELIMIT) {
							SingleWindow *nwin = new SingleWindow();
							uint32_t nc = c+1;

							Reading *cReading = new Reading();
							cReading->baseform = begintag;
							cReading->wordform = begintag;
							cReading->tags[begintag] = begintag;
							cReading->tags_list.push_back(begintag);
							cReading->rehash();

							Cohort *cCohort = new Cohort();
							cCohort->wordform = begintag;
							cCohort->readings.push_back(cReading);

							nwin->cohorts.push_back(cCohort);

							for ( ; nc < current->cohorts.size() ; nc++) {
								nwin->cohorts.push_back(current->cohorts.at(nc));
							}
							c = (uint32_t)current->cohorts.size()-c;
							for (nc = 0 ; nc < c-1 ; nc++) {
								current->cohorts.pop_back();
							}
							window->next.push_front(nwin);

							cohort = current->cohorts.back();
							for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
								Reading *reading = *rter;
								reading->tags_list.push_back(endtag);
								reading->tags[endtag] = endtag;
								reading->rehash();
							}
							delimited = true;
							break;
						}
						else if (rule->type == K_ADD || rule->type == K_MAP) {
							reading->mapped_by.push_back(j);
							reading->noprint = false;
							std::list<uint32_t>::const_iterator tter;
							for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
								reading->tags_list.push_back(*tter);
							}
							reflowReading(reading);
							if (rule->type == K_MAP) {
								reading->mapped = true;
							}
						}
						else if (rule->type == K_REPLACE) {
							reading->mapped_by.push_back(j);
							reading->noprint = false;
							std::list<uint32_t>::const_iterator tter;
							reading->tags_list.clear();
							reading->tags_list.push_back(reading->wordform);
							reading->tags_list.push_back(reading->baseform);
							for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
								reading->tags_list.push_back(*tter);
							}
							reflowReading(reading);
							if (!reading->tags_mapped.empty()) {
								reading->mapped = true;
							}
						}
						else if (rule->type == K_SUBSTITUTE) {
							reading->mapped_by.push_back(j);
							reading->noprint = false;
							std::list<uint32_t>::const_iterator tter;
							for (tter = rule->sublist.begin() ; tter != rule->sublist.end() ; tter++) {
								reading->tags_list.remove(*tter);
							}
							for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
								reading->tags_list.push_back(*tter);
							}
							reflowReading(reading);
							if (!reading->tags_mapped.empty()) {
								reading->mapped = true;
							}
						}
					}
					// ToDo: Implement APPEND
				}
				
				if (!removed.empty()) {
					cohort->deleted.insert(cohort->deleted.end(), removed.begin(), removed.end());
					removed.clear();
				}
				if (!selected.empty()) {
					cohort->readings.clear();
					cohort->readings.insert(cohort->readings.begin(), selected.begin(), selected.end());
					selected.clear();
				}
				
				if (delimited) {
					break;
				}
			}
			if (j == end) {
				if (single_run) {
					section_did_good = false;
				}
			}
			j++;
			if (delimited) {
				break;
			}
		}
	}
	if (section_did_good) {
		retval |= RV_SOMETHING;
	}
	if (delimited) {
		retval |= RV_DELIMITED;
	}
	return retval;
}

int GrammarApplicator::runGrammarOnWindow(Window *window) {
label_runGrammarOnWindow_begin:
	SingleWindow *current = window->current;

	if (!grammar->before_sections.empty()) {
		uint32_t rv = runRulesOnWindow(window, &grammar->before_sections, 0, (uint32_t)grammar->before_sections.size());
		if (rv & RV_DELIMITED) {
			goto label_runGrammarOnWindow_begin;
		}
	}

	// ToDo: Make old cohort -> rules order available via switch
	if (!grammar->rules.empty()) {
		reflowSingleWindow(current);

		for (uint32_t i=0;i<grammar->sections.size()-1;) {
			uint32_t rv = runRulesOnWindow(window, &grammar->rules, 0, grammar->sections[i+1]);
			if (rv & RV_DELIMITED) {
				goto label_runGrammarOnWindow_begin;
			}
			if (!(rv & RV_SOMETHING)) {
				i++;
			}
		}
	}

	if (!grammar->after_sections.empty()) {
		uint32_t rv = runRulesOnWindow(window, &grammar->after_sections, 0, (uint32_t)grammar->after_sections.size());
		if (rv & RV_DELIMITED) {
			goto label_runGrammarOnWindow_begin;
		}
	}

	return 0;
}

bool GrammarApplicator::runContextualTest(const Window *window, const SingleWindow *sWindow, const uint32_t position, const ContextualTest *test) {
	bool retval = true;
	int pos = position + test->offset;
	const Cohort *cohort = 0;
	// ToDo: (NOT *) and (*C) tests can be cached
	if (test->absolute) {
		if (test->offset < 0) {
			pos = ((int)sWindow->cohorts.size()-1) - test->offset;
		}
		else {
			pos = test->offset;
		}
	}
	if (pos >= 0 && (uint32_t)pos < sWindow->cohorts.size()) {
		cohort = sWindow->cohorts.at(pos);
	}
	else if (test->span_windows && pos < 0) {
		sWindow = window->previousFrom(sWindow);
		if (sWindow) {
			pos = (int)sWindow->cohorts.size()+pos;
			cohort = sWindow->cohorts.at(pos);
		}
	}
	else if (test->span_windows && (uint32_t)pos >= sWindow->cohorts.size()) {
		sWindow = window->nextFrom(sWindow);
	}

	if (!cohort) {
		retval = false;
	}
	else {
		bool foundfirst = false;
		if (test->offset < 0 && pos >= 0 && (test->scanall || test->scanfirst)) {
			for (int i=pos;i>=0;i--) {
				cohort = sWindow->cohorts.at(i);
				retval = doesSetMatchCohortNormal(cohort, test->target);
				foundfirst = retval;
				if (test->careful) {
					if (retval) {
						retval = doesSetMatchCohortCareful(cohort, test->target);
					}
					else {
						retval = false;
					}
				}
				if (test->negative) {
					retval = !retval;
				}
				if (retval && test->linked) {
					retval = runContextualTest(window, sWindow, i, test->linked);
				}
				if (foundfirst && test->scanfirst) {
					break;
				}
				if (test->barrier) {
					bool barrier = doesSetMatchCohortNormal(cohort, test->barrier);
					if (barrier) {
						break;
					}
				}
			}
		}
		else if (test->offset > 0 && (uint32_t)pos <= sWindow->cohorts.size() && (test->scanall || test->scanfirst)) {
			for (uint32_t i=pos;i<sWindow->cohorts.size();i++) {
				cohort = sWindow->cohorts.at(i);
				retval = doesSetMatchCohortNormal(cohort, test->target);
				foundfirst = retval;
				if (test->careful) {
					if (retval) {
						retval = doesSetMatchCohortCareful(cohort, test->target);
					}
					else {
						retval = false;
					}
				}
				if (test->negative) {
					retval = !retval;
				}
				if (retval && test->linked) {
					retval = runContextualTest(window, sWindow, i, test->linked);
				}
				if (foundfirst && test->scanfirst) {
					break;
				}
				if (test->barrier) {
					bool barrier = doesSetMatchCohortNormal(cohort, test->barrier);
					if (barrier) {
						break;
					}
				}
			}
		}
		else {
			if (test->careful) {
				retval = doesSetMatchCohortCareful(cohort, test->target);
			}
			else {
				retval = doesSetMatchCohortNormal(cohort, test->target);
			}
			if (test->negative) {
				retval = !retval;
			}
			if (retval && test->linked) {
				retval = runContextualTest(window, sWindow, pos, test->linked);
			}
		}
	}
	if (!cohort && test->negative) {
		retval = !retval;
	}
	if (retval) {
		test->num_match++;
	} else {
		test->num_fail++;
	}
	return retval;
}
