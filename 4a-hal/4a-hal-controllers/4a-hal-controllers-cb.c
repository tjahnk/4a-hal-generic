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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <wrap-json.h>

#include "../4a-hal-utilities/4a-hal-utilities-data.h"
#include "../4a-hal-utilities/4a-hal-utilities-verbs-loader.h"

#include "4a-hal-controllers-cb.h"

/*******************************************************************************
 *		HAL controllers sections parsing functions		       *
 ******************************************************************************/

int HalCtlsHalMixerConfig(afb_dynapi *apiHandle, CtlSectionT *section, json_object *MixerJ)
{
	unsigned int streamCount, idx;
	char *currentStreamName;

	CtlConfigT *ctrlConfig;
	struct SpecificHalData *currentHalData;

	json_object *streamsArray, *currentStream;

	struct HalUtlApiVerb *CtlHalDynApiStreamVerbs;

	if(! apiHandle || ! section)
		return -1;

	if(MixerJ) {
		ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
		if(! ctrlConfig)
			return -2;

		currentHalData = (struct SpecificHalData *) ctrlConfig->external;
		if(! currentHalData)
			return -3;

		if(json_object_is_type(MixerJ, json_type_object))
			currentHalData->ctlHalSpecificData->halMixerJ = MixerJ;
		else
			return -4;

		if(wrap_json_unpack(MixerJ, "{s:s}", "mixerapi", &currentHalData->ctlHalSpecificData->mixerApiName))
			return -5;

		if(wrap_json_unpack(MixerJ, "{s:s}", "uid", &currentHalData->ctlHalSpecificData->halSoftMixerVerb))
			return -6;

		if(! json_object_object_get_ex(MixerJ, "streams", &streamsArray))
			return -7;

		switch(json_object_get_type(streamsArray)) {
			case json_type_object:
				streamCount = 1;
				break;
			case json_type_array:
				streamCount = json_object_array_length(streamsArray);
				break;
			default:
				return -8;
		}

		currentHalData->ctlHalSpecificData->ctlHalStreamsData.count = streamCount;
		currentHalData->ctlHalSpecificData->ctlHalStreamsData.data =
			(struct CtlHalStreamData *) calloc(streamCount, sizeof (struct CtlHalStreamData *));

		CtlHalDynApiStreamVerbs = alloca((streamCount + 1) * sizeof(struct HalUtlApiVerb));
		memset(CtlHalDynApiStreamVerbs, '\0', (streamCount + 1) * sizeof(struct HalUtlApiVerb));
		CtlHalDynApiStreamVerbs[streamCount + 1].verb = NULL;

		for(idx = 0; idx < streamCount; idx++) {
			if(streamCount > 1)
				currentStream = json_object_array_get_idx(streamsArray, (int) idx);
			else
				currentStream = streamsArray;

			if(wrap_json_unpack(currentStream, "{s:s}", "uid", &currentStreamName))
				return -10-idx;

			currentHalData->ctlHalSpecificData->ctlHalStreamsData.data[idx].name = strdup(currentStreamName);
			currentHalData->ctlHalSpecificData->ctlHalStreamsData.data[idx].cardId = NULL;

			CtlHalDynApiStreamVerbs[idx].verb = currentStreamName;
			CtlHalDynApiStreamVerbs[idx].callback = HalCtlsActionOnStream;
			CtlHalDynApiStreamVerbs[idx].info = "Peform action on this stream";
		}

		if(HalUtlLoadVerbs(apiHandle, CtlHalDynApiStreamVerbs))
			return -9;
	}

	return 0;
}

// TODO JAI : to implement
int HalCtlsHalMapConfig(afb_dynapi *apiHandle, CtlSectionT *section, json_object *StreamControlsJ)
{
	AFB_DYNAPI_WARNING(apiHandle, "JAI :%s not implemented yet", __func__);

	return 0;
}

/*******************************************************************************
 *		HAL controllers verbs functions				       *
 ******************************************************************************/

void HalCtlsActionOnStream(afb_request *request)
{
	int verbToCallSize;

	char *apiToCall, *halSoftMixerVerb, *verbToCall;

	afb_dynapi *apiHandle;
	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData;

	json_object *requestJson, *returnJ, *toReturnJ;

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		afb_request_fail(request, "api_handle", "Can't get current hal controller api handle");
		return;
	}

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		afb_request_fail(request, "hal_controller_config", "Can't get current hal controller config");
		return;
	}

	currentCtlHalData = ctrlConfig->external;
	if(! currentCtlHalData) {
		afb_request_fail(request, "hal_controller_data", "Can't get current hal controller data");
		return;
	}

	requestJson = afb_request_json(request);
	if(! requestJson) {
		afb_request_fail(request, "request_json", "Can't get request json");
		return;
	}

	apiToCall = currentCtlHalData->ctlHalSpecificData->mixerApiName;
	if(! apiToCall) {
		afb_request_fail(request, "mixer_api", "Can't get mixer api");
		return;
	}

	halSoftMixerVerb = currentCtlHalData->ctlHalSpecificData->halSoftMixerVerb;
	if(! halSoftMixerVerb) {
		afb_request_fail(request, "hal_softmixer_verb", "Can't get hal mixer verb prefix");
		return;
	}

	// TODO JAI: check status of hal before doing anything

	// TODO JAI : remove verb to call prefix, each hal should have its own api in softmixer, and each streams should be created as verb by mixer
	verbToCallSize = (int) strlen(halSoftMixerVerb) + (int) strlen(request->verb) + 2;
	verbToCall = (char *) alloca(verbToCallSize * sizeof(char));
	verbToCall[0] = '\0';
	verbToCall[verbToCallSize - 1] = '\0';

	strcat(verbToCall, halSoftMixerVerb);
	strcat(verbToCall, "/");
	strcat(verbToCall, request->verb);

	if(afb_dynapi_call_sync(apiHandle, apiToCall, verbToCall, json_object_get(requestJson), &returnJ)) {
		afb_request_fail_f(request, "action", "Action %s seemingly not correctly transfered to %s",
							verbToCall,
							apiToCall);

	}
	else if(json_object_object_get_ex(returnJ, "response", &toReturnJ)){
		afb_request_success_f(request, toReturnJ, "Action %s correctly transfered to %s, and successfull",
								verbToCall,
								apiToCall);
	}
	else {
		afb_request_fail_f(request, "invalid_response", "Action %s correctly transfered to %s, but response is not valid",
							verbToCall,
							apiToCall);
	}
}

void HalCtlsListVerbs(afb_request *request)
{
	unsigned int idx;

	afb_dynapi *apiHandle;
	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData;

	json_object *requestJson, *requestAnswer, *streamsArray, *currentStream;

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		afb_request_fail(request, "api_handle", "Can't get current hal controller api handle");
		return;
	}

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		afb_request_fail(request, "hal_controller_config", "Can't get current hal controller config");
		return;
	}

	currentCtlHalData = ctrlConfig->external;
	if(! currentCtlHalData) {
		afb_request_fail(request, "hal_controller_data", "Can't get current hal controller data");
		return;
	}

	if(! currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.count) {
		afb_request_fail(request, "no_data", "Won't be able to respond, no streams ofound");
		return;
	}

	requestJson = afb_request_json(request);
	if(! requestJson) {
		afb_request_fail(request, "request_json", "Can't get request json");
		return;
	}

	streamsArray = json_object_new_array();
	if(! streamsArray) {
		afb_request_fail(request, "json_answer", "Can't generate par of json answer");
		return;
	}

	for(idx = 0; idx < currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.count; idx++) {
		wrap_json_pack(&currentStream,
			       "{s:s s:s}",
			       "name", currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.data[idx].name,
			       "cardId", currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.data[idx].cardId ?
					 currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.data[idx].cardId :
					 "");
		json_object_array_add(streamsArray, currentStream);
	}

	// TODO JAI : Check if there is some halmap config controls and add them to the reponse

	wrap_json_pack(&requestAnswer, "{s:o}", "streams", streamsArray);

	afb_request_success(request, requestAnswer, "Requested data");
}

void HalCtlsInitMixer(afb_request *request)
{
	char *apiToCall;

	afb_dynapi *apiHandle;
	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData;

	json_object *returnJ, *toReturnJ;

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		afb_request_fail(request, "api_handle", "Can't get hal manager api handle");
		return;
	}

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		afb_request_fail(request, "hal_controller_config", "Can't get current hal controller config");
		return;
	}

	currentCtlHalData = ctrlConfig->external;
	if(! currentCtlHalData) {
		afb_request_fail(request, "hal_controller_data", "Can't get current hal controller data");
		return;
	}

	apiToCall = currentCtlHalData->ctlHalSpecificData->mixerApiName;
	if(! apiToCall) {
		afb_request_fail(request, "mixer_api", "Can't get mixer api");
		return;
	}

	// TODO JAI: test hal status (card is detected)

	if(afb_dynapi_call_sync(apiHandle, apiToCall, "mixer_new", json_object_get(currentCtlHalData->ctlHalSpecificData->halMixerJ), &returnJ)) {
		afb_request_fail_f(request,
				   "mixer_new_call",
				   "Seems that mix_new call to api %s didn't end well",
				   apiToCall);
	}
	else if(json_object_object_get_ex(returnJ, "response", &toReturnJ)) {
		// TODO JAI : get streams cardId from mixer response

		afb_request_success_f(request,
				      toReturnJ,
				      "Seems that mix_new call to api %s succeed",
				      apiToCall);
	}
	else {
		afb_request_fail_f(request,
				   "invalid_response",
				   "Seems that mix_new call to api %s succeed, but response is not valid",
				   apiToCall);
	}
}