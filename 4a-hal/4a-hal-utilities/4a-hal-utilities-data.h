/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Jonathan Aillet <jonathan.aillet@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _HAL_UTILITIES_DATA_INCLUDE_
#define _HAL_UTILITIES_DATA_INCLUDE_

#include <stdio.h>

#define AFB_BINDING_VERSION dyn
#include <afb/afb-binding.h>

#include <ctl-config.h>

// Enum for sharing hal (controller or external) status
enum HalStatus {
	HAL_STATUS_UNAVAILABLE=0,
	HAL_STATUS_AVAILABLE=1,
};

struct CtlHalStreamData {
	char *name;
	char *cardId;
};

struct CtlHalStreamsDataT {
	struct CtlHalStreamData *data;
	unsigned int count;
};

struct CtlHalSpecificData {
	char *mixerApiName;
	json_object *halMixerJ;

	struct CtlHalStreamsDataT ctlHalStreamsData;
	// TODO JAI : add structure to hold halmap section data

	afb_dynapi *apiHandle;
	CtlConfigT *ctrlConfig;
};

// Structure to store specific hal (controller or external) data
struct SpecificHalData {
	char *apiName;
	enum HalStatus status;
	char *sndCard;
	char *info;
	unsigned int internal;

	char *author;
	char *version;
	char *date;
	// Can be beefed up if needed

	struct CtlHalSpecificData *ctlHalSpecificData;		// Can be NULL if external api

	struct SpecificHalData *next;
};

// Structure to store hal manager data
struct HalMgrData {
	char *apiName;
	char *info;

	afb_dynapi *apiHandle;

	struct SpecificHalData *first;
};

// Exported verbs for 'struct SpecificHalData' handling
struct SpecificHalData *HalUtlAddHalApiToHalList(struct HalMgrData *HalMgrGlobalData);
uint8_t HalUtlRemoveSelectedHalFromList(struct HalMgrData *HalMgrGlobalData, struct SpecificHalData *ApiToRemove);
uint64_t HalUtlRemoveAllHalFromList(struct HalMgrData *HalMgrGlobalData);
uint64_t HalUtlGetNumberOfHalInList(struct HalMgrData *HalMgrGlobalData);
struct SpecificHalData *HalUtlSearchHalDataByApiName(struct HalMgrData *HalMgrGlobalData, char *apiName);

// Exported verbs for 'struct HalMgrData' handling
uint8_t HalUtlInitializeHalMgrData(afb_dynapi *apiHandle, struct HalMgrData *HalMgrGlobalData, char *apiName, char *info);
void HalUtlRemoveHalMgrData(struct HalMgrData *HalMgrGlobalData);

#endif /* _HAL_UTILITIES_DATA_INCLUDE_ */