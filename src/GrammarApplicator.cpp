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

GrammarApplicator::GrammarApplicator() {
	num_windows = 2;
	grammar = 0;
}

GrammarApplicator::~GrammarApplicator() {
	grammar = 0;
}

void GrammarApplicator::setGrammar(const Grammar *res) {
	grammar = res;
}

uint32_t GrammarApplicator::addTag(const UChar *txt) {
	Tag *tag = new Tag();
	tag->parseTag(txt);
	uint32_t hash = tag->rehash();
	if (single_tags.find(hash) == single_tags.end()) {
		single_tags[hash] = tag;
	} else {
		delete tag;
	}
	return hash;
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

	uint32_t begintag = addTag(stringbits[S_BEGINTAG]);
	uint32_t endtag = addTag(stringbits[S_ENDTAG]);

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
					cReading->rehash();
					cReading->noprint = true;
					cCohort->readings.push_back(cReading);
					lReading = cReading;
				}
				std::vector<Reading*>::iterator iter;
				for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
					(*iter)->tags[endtag] = endtag;
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
				cSWindow->cohorts.push_back(cCohort);
				lCohort = cCohort;
			}
			if (cWindow->next.size() > num_windows) {
				cWindow->shuffleWindowsDown();
				printSingleWindow(cWindow->current, output);
				u_fflush(output);
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
					if (single_tags[tag]->type & T_MAPPING) {
						cReading->mapped = true;
					}
					cReading->tags[tag] = tag;
				}
				base = space;
			}
			if (u_strlen(base)) {
				uint32_t tag = addTag(base);
				if (!cReading->baseform && single_tags[tag]->type & T_BASEFORM) {
					cReading->baseform = tag;
				}
				if (single_tags[tag]->type & T_MAPPING) {
					cReading->mapped = true;
				}
				cReading->tags[tag] = tag;
			}
			cCohort->readings.push_back(cReading);
			lReading = cReading;
		}
		else {
			if (cleaned[0] == ' ' && cleaned[1] == '"') {
				u_fprintf(ux_stderr, "Warning: Line %u looked like a reading but there was no containing cohort - treated as plain text.\n", lines);
			}
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
		lines++;
	}

	while (!cWindow->next.empty()) {
		cWindow->shuffleWindowsDown();
		printSingleWindow(cWindow->current, output);
		u_fflush(output);
	}
	return 0;
}

void GrammarApplicator::printSingleWindow(SingleWindow *window, UFILE *output) {
	if (window->text) {
		u_fprintf(output, "%S", window->text);
	}

	std::vector<Cohort*>::iterator cter;
	for (cter = window->cohorts.begin() ; cter != window->cohorts.end() ; cter++) {
		if (cter == window->cohorts.begin()) {
			continue;
		}
		Cohort *cohort = *cter;
		single_tags[cohort->wordform]->print(output);
		u_fprintf(output, "\n");
		if (cohort->text) {
			u_fprintf(output, "%S", cohort->text);
		}

		std::vector<Reading*>::iterator rter;
		for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
			Reading *reading = *rter;
			if (reading->noprint) {
				continue;
			}
			if (reading->deleted) {
				u_fprintf(output, ";");
			}
			u_fprintf(output, "\t");
			single_tags[reading->baseform]->print(output);
			u_fprintf(output, " ");

			stdext::hash_map<uint32_t, uint32_t>::iterator tter;
			for (tter = reading->tags.begin() ; tter != reading->tags.end() ; tter++) {
				Tag *tag = single_tags[tter->second];
				if (!(tag->type & T_BASEFORM) && !(tag->type & T_WORDFORM)) {
					tag->print(output);
					u_fprintf(output, " ");
				}
			}
			u_fprintf(output, "\n");
			if (reading->text) {
				u_fprintf(output, "%S", reading->text);
			}
		}
	}
}
