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
#include "stdafx.h"
#include <unicode/ustring.h>
#include "CompositeTag.h"

using namespace CG3;

CompositeTag::CompositeTag() {
	hash = 0;
}

CompositeTag::~CompositeTag() {
	tags_map.clear();
	tags.clear();
}

void CompositeTag::addTag(uint32_t tag) {
	tags[tag] = tag;
	tags_map[tag] = tag;
}
void CompositeTag::removeTag(uint32_t tag) {
	tags.erase(tag);
	tags_map.erase(tag);
}

uint32_t CompositeTag::rehash() {
	uint32_t retval = 0;
	std::map<uint32_t, uint32_t>::iterator iter;
	for (iter = tags_map.begin() ; iter != tags_map.end() ; iter++) {
		retval = hash_sdbm_uint32_t(iter->second, retval);
	}
	hash = retval;
	return retval;
}
